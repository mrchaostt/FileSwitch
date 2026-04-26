#include "transfer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QHostInfo>
#include <QNetworkInterface>

FileTransfer::FileTransfer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_sendSocket(nullptr)
    , m_currentSendFile(nullptr)
    , m_pendingChunk()
    , m_pendingOffset(0)
    , m_sendQueue()
    , m_currentFileIndex(0)
    , m_currentFileOffset(0)
    , m_totalBytes(0)
    , m_sentBytes(0)
    , m_downloadDir(QDir::homePath() + "/Downloads/FileSwitch")
    , m_receiveSocket(nullptr)
    , m_receiveFile(nullptr)
    , m_receiveFilename()
    , m_receiveFilesize(0)
    , m_receiveOffset(0)
    , m_receivingHeader(true)
    , m_pendingRequest()
    , m_currentReceiveFileIndex(0)
    , m_currentReceiveFileActualSize(0)
    , m_transferAccepted(false)
{
    connect(m_server, &QTcpServer::newConnection, this, &FileTransfer::acceptConnection);
}

FileTransfer::~FileTransfer() {
    stopListening();
}

void FileTransfer::startListening() {
    m_server->listen(QHostAddress::AnyIPv4, TRANSFER_PORT);
}

void FileTransfer::stopListening() {
    m_server->close();
    if (m_sendSocket) {
        m_sendSocket->disconnect();
        m_sendSocket->deleteLater();
        m_sendSocket = nullptr;
    }
    if (m_receiveSocket) {
        m_receiveSocket->disconnect();
        m_receiveSocket->deleteLater();
        m_receiveSocket = nullptr;
    }
}

void FileTransfer::acceptTransfer() {
    m_transferAccepted = true;
    if (!m_pendingRequest.files.isEmpty() && m_currentReceiveFileIndex < m_pendingRequest.files.size()) {
        openNextReceiveFile();
    }
}

void FileTransfer::rejectTransfer() {
    m_transferAccepted = false;
    if (m_receiveFile) {
        m_receiveFile->close();
        delete m_receiveFile;
        m_receiveFile = nullptr;
    }
    if (m_receiveSocket) {
        m_receiveSocket->disconnectFromHost();
    }
    resetReceiveState();
    emit transferRejected();
}

void FileTransfer::resetReceiveState() {
    m_receivingHeader = true;
    m_receiveOffset = 0;
    m_receiveFilesize = 0;
    m_receiveFilename.clear();
    m_currentReceiveFileIndex = 0;
    m_currentReceiveFileActualSize = 0;
    m_totalBytes = 0;
    m_pendingRequest = TransferRequest();
    m_transferAccepted = false;
}

void FileTransfer::openNextReceiveFile() {
    if (!m_transferAccepted) return;
    if (m_currentReceiveFileIndex >= m_pendingRequest.files.size()) return;

    if (m_receiveFile) {
        m_receiveFile->close();
        delete m_receiveFile;
        m_receiveFile = nullptr;
    }

    const TransferFile &file = m_pendingRequest.files.at(m_currentReceiveFileIndex);
    m_receiveFilename = file.name;
    m_receiveFilesize = file.size;
    m_receiveOffset = 0;
    m_currentReceiveFileActualSize = 0;

    m_receiveFile = new QFile(m_downloadDir + "/" + m_receiveFilename);
    if (!m_receiveFile->open(QFile::WriteOnly)) {
        emit transferError("Cannot open file for writing: " + m_receiveFilename);
        delete m_receiveFile;
        m_receiveFile = nullptr;
        return;
    }
}

void FileTransfer::acceptConnection() {
    // Clean up any existing connection
    if (m_receiveSocket) {
        m_receiveSocket->disconnect();
        m_receiveSocket->deleteLater();
    }

    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    m_receiveSocket = socket;
    resetReceiveState();

    connect(m_receiveSocket, &QTcpSocket::readyRead, this, &FileTransfer::readIncomingData);
    connect(m_receiveSocket, &QTcpSocket::disconnected, this, [this]() {
        resetReceiveState();
    });
    connect(m_receiveSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit transferError(m_receiveSocket->errorString());
        resetReceiveState();
    });
}

void FileTransfer::readIncomingData() {
    if (!m_receiveSocket) return;

    while (m_receiveSocket->bytesAvailable() > 0) {
        if (m_receivingHeader) {
            // Need at least 4 bytes for header size
            if (m_receiveSocket->bytesAvailable() < 4) return;

            // Read 4-byte header size (big-endian)
            QDataStream stream(m_receiveSocket);
            stream.setVersion(QDataStream::Qt_5_0);
            quint32 headerSize;
            stream >> headerSize;

            // Wait for full header
            if (m_receiveSocket->bytesAvailable() < static_cast<qint64>(headerSize)) return;

            // Read header JSON
            QByteArray headerData = m_receiveSocket->read(headerSize);
            QJsonDocument doc = QJsonDocument::fromJson(headerData);
            if (!doc.isObject()) {
                emit transferError("Invalid header format");
                return;
            }

            QJsonObject obj = doc.object();
            handleTransferRequest(obj);
            m_receivingHeader = false;

            // Don't proceed to file reading until user accepts
            if (!m_transferAccepted) {
                return;
            }
        }

        // Receive file data - only if accepted
        if (m_transferAccepted && m_receiveFile && m_receiveSocket->bytesAvailable() > 0) {
            // If current file is already complete, rotate to next file first.
            qint64 remaining = m_receiveFilesize - m_receiveOffset;
            if (remaining <= 0) {
                m_receiveFile->close();
                delete m_receiveFile;
                m_receiveFile = nullptr;
                m_currentReceiveFileIndex++;

                if (m_currentReceiveFileIndex < m_pendingRequest.files.size()) {
                    openNextReceiveFile();
                } else {
                    emit transferCompleted();
                    resetReceiveState();
                }
                continue;
            }

            qint64 toRead = qMin(m_receiveSocket->bytesAvailable(), remaining);
            QByteArray data = m_receiveSocket->read(toRead);
            if (!data.isEmpty()) {
                m_receiveFile->write(data);
                m_receiveOffset += data.size();
                m_currentReceiveFileActualSize += data.size();

                // Calculate total progress across all files
                qint64 totalProgress = 0;
                for (int i = 0; i < m_currentReceiveFileIndex; ++i) {
                    totalProgress += m_pendingRequest.files.at(i).size;
                }
                totalProgress += m_receiveOffset;
                emit transferProgress(totalProgress, m_totalBytes);

                // Check if current file is complete
                if (m_receiveOffset >= m_receiveFilesize) {
                    m_receiveFile->close();
                    delete m_receiveFile;
                    m_receiveFile = nullptr;
                    m_currentReceiveFileIndex++;

                    // Check if more files to receive
                    if (m_currentReceiveFileIndex < m_pendingRequest.files.size()) {
                        openNextReceiveFile();
                        m_receiveOffset = 0;
                    } else {
                        emit transferCompleted();
                        resetReceiveState();
                    }
                }
            }
        }
    }
}

void FileTransfer::handleTransferRequest(const QJsonObject &obj) {
    TransferRequest request;
    request.senderIp = obj["sender_ip"].toString();
    request.senderHostname = obj["sender_hostname"].toString();

    QJsonArray filesArray = obj["files"].toArray();
    for (const QJsonValue &fileVal : filesArray) {
        QJsonObject fileObj = fileVal.toObject();
        TransferFile file;
        file.name = fileObj["name"].toString();
        file.size = fileObj["size"].toVariant().toLongLong();
        request.files.append(file);
    }

    m_pendingRequest = request;

    // Calculate total bytes
    m_totalBytes = 0;
    for (const TransferFile &f : request.files) {
        m_totalBytes += f.size;
    }

    // Create download directory if needed
    QDir dir;
    dir.mkpath(m_downloadDir);

    // Reset transfer state but wait for user acceptance
    m_currentReceiveFileIndex = 0;
    m_receiveOffset = 0;
    m_receivingHeader = false;
    m_transferAccepted = false;

    emit transferRequestReceived(request);
}

void FileTransfer::sendFiles(const QString &targetIp, const QList<TransferFile> &files) {
    if (files.isEmpty()) return;

    m_sendSocket = new QTcpSocket(this);
    m_sendQueue = files;
    m_currentFileIndex = 0;
    m_currentFileOffset = 0;
    m_sentBytes = 0;
    m_totalBytes = 0;

    for (const TransferFile &f : files) {
        m_totalBytes += f.size;
    }

    connect(m_sendSocket, &QTcpSocket::connected, this, [this, targetIp]() {
        // Build header JSON
        QJsonObject obj;
        obj["type"] = "transfer_request";
        obj["sender_ip"] = getLocalIP();
        obj["sender_hostname"] = QHostInfo::localHostName();

        QJsonArray filesArray;
        for (const TransferFile &f : m_sendQueue) {
            QJsonObject fileObj;
            fileObj["name"] = f.name;
            fileObj["size"] = f.size;
            filesArray.append(fileObj);
        }
        obj["files"] = filesArray;

        QByteArray headerData = QJsonDocument(obj).toJson();

        // Send 4-byte big-endian header size + header JSON
        QByteArray header;
        QDataStream stream(&header, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_0);
        stream << static_cast<quint32>(headerData.size());
        header.append(headerData);

        m_sendSocket->write(header);
        m_sendSocket->flush();

        // Start sending first file
        m_currentFileIndex = 0;
        openNextSendFile();
        sendFileData();  // Explicitly trigger first chunk send
    });

    connect(m_sendSocket, &QTcpSocket::bytesWritten, this, &FileTransfer::onBytesWritten);
    connect(m_sendSocket, &QTcpSocket::disconnected, this, [this]() {
        cleanupSendState();
    });

    m_sendSocket->connectToHost(targetIp, TRANSFER_PORT);
}

void FileTransfer::openNextSendFile() {
    if (m_currentFileIndex >= m_sendQueue.size()) return;

    if (m_currentSendFile) {
        m_currentSendFile->close();
        delete m_currentSendFile;
        m_currentSendFile = nullptr;
    }

    const TransferFile &file = m_sendQueue.at(m_currentFileIndex);
    m_currentSendFile = new QFile(file.path);
    if (!m_currentSendFile->open(QFile::ReadOnly)) {
        emit transferError("Cannot open file for reading: " + file.path);
        delete m_currentSendFile;
        m_currentSendFile = nullptr;
        return;
    }
    m_currentFileOffset = 0;
}

void FileTransfer::onBytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);
    // Don't increment offset here - sendFileData already tracks what it writes
    // This callback just triggers the next chunk to be sent

    // Continue sending current file
    sendFileData();
}

void FileTransfer::sendFileData() {
    if (!m_sendSocket || m_sendSocket->state() != QAbstractSocket::ConnectedState) return;

    // If we have pending data from a partial write, try to send it first
    if (!m_pendingChunk.isEmpty()) {
        qint64 written = m_sendSocket->write(m_pendingChunk.constData() + m_pendingOffset, m_pendingChunk.size() - m_pendingOffset);
        if (written > 0) {
            m_pendingOffset += written;
            m_currentFileOffset += written;
            if (m_pendingOffset >= m_pendingChunk.size()) {
                // Pending chunk fully sent
                m_pendingChunk.clear();
                m_pendingOffset = 0;
            }
        } else if (written < 0) {
            // Write failed
            emit transferError("Socket write error");
            cleanupSendState();
            return;
        }
        // Continue to check if we can send more
    }

    // Check if we need to open next file
    if (m_pendingChunk.isEmpty() && (!m_currentSendFile || m_currentSendFile->atEnd())) {
        if (m_currentSendFile) {
            m_currentSendFile->close();
            delete m_currentSendFile;
            m_currentSendFile = nullptr;
        }

        m_currentFileIndex++;
        m_currentFileOffset = 0;

        if (m_currentFileIndex >= m_sendQueue.size()) {
            // All files sent
            emit transferCompleted();
            cleanupSendState();
            return;
        }

        openNextSendFile();
    }

    // Send chunk - only if no pending data
    if (m_pendingChunk.isEmpty() && m_currentSendFile && !m_currentSendFile->atEnd()) {
        QByteArray chunk = m_currentSendFile->read(CHUNK_SIZE);
        if (!chunk.isEmpty()) {
            qint64 written = m_sendSocket->write(chunk);
            if (written > 0) {
                if (written < chunk.size()) {
                    // Partial write - save remaining data
                    m_pendingChunk = chunk;
                    m_pendingOffset = written;
                }
                m_currentFileOffset += written;

                // Progress only counts actual file data, not header
                qint64 fileDataSent = 0;
                for (int i = 0; i < m_currentFileIndex; ++i) {
                    fileDataSent += m_sendQueue.at(i).size;
                }
                fileDataSent += m_currentFileOffset;

                emit transferProgress(fileDataSent, m_totalBytes);
            } else if (written < 0) {
                emit transferError("Socket write error");
                cleanupSendState();
            }
        }
    }
}

void FileTransfer::cleanupSendState() {
    if (m_currentSendFile) {
        m_currentSendFile->close();
        delete m_currentSendFile;
        m_currentSendFile = nullptr;
    }
    if (m_sendSocket) {
        m_sendSocket->deleteLater();
        m_sendSocket = nullptr;
    }
    m_pendingChunk.clear();
    m_pendingOffset = 0;
    m_currentFileIndex = 0;
    m_sendQueue.clear();
}

QString FileTransfer::downloadDirectory() const {
    QMutexLocker locker(&m_mutex);
    return m_downloadDir;
}

void FileTransfer::setDownloadDirectory(const QString &dir) {
    QMutexLocker locker(&m_mutex);
    m_downloadDir = dir;
}

QString FileTransfer::getLocalIP() const {
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost) {
            return addr.toString();
        }
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

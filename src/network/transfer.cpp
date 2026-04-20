#include "transfer.h"
#include <QJsonDocument>
#include <QJsonObject>
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
}

void FileTransfer::acceptConnection() {
    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    m_receiveSocket = socket;
    m_receivingHeader = true;
    m_receiveOffset = 0;
    m_receiveFilesize = 0;

    connect(m_receiveSocket, &QTcpSocket::readyRead, this, &FileTransfer::readIncomingData);
    connect(m_receiveSocket, &QTcpSocket::disconnected, this, [this]() {
        if (m_receiveFile) {
            m_receiveFile->close();
            delete m_receiveFile;
            m_receiveFile = nullptr;
        }
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
            stream.setVersion(QDataStream::Qt_5_15);
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
        }

        // Receive file data
        if (m_receiveFile && m_receiveSocket->bytesAvailable() > 0) {
            QByteArray data = m_receiveSocket->readAll();
            m_receiveFile->write(data);
            m_receiveOffset += data.size();

            emit transferProgress(m_receiveOffset, m_receiveFilesize);

            // Check if current file is complete
            if (m_receiveOffset >= m_receiveFilesize) {
                m_receiveFile->close();
                delete m_receiveFile;
                m_receiveFile = nullptr;

                // Check if entire transfer is complete
                if (m_receiveOffset >= m_totalBytes) {
                    emit transferCompleted();
                    m_totalBytes = 0;
                    m_receiveOffset = 0;
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
    emit transferRequestReceived(request);

    // Create download directory if needed
    QDir dir;
    dir.mkpath(m_downloadDir);

    // Set up first file
    if (!request.files.isEmpty()) {
        m_receiveFilename = request.files.first().name;
        m_receiveFilesize = request.files.first().size;
        m_totalBytes = 0;
        for (const TransferFile &f : request.files) {
            m_totalBytes += f.size;
        }
        m_receiveOffset = 0;

        m_receiveFile = new QFile(m_downloadDir + "/" + m_receiveFilename);
        if (!m_receiveFile->open(QFile::WriteOnly)) {
            emit transferError("Cannot open file for writing: " + m_receiveFilename);
            delete m_receiveFile;
            m_receiveFile = nullptr;
        }
    }
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

    connect(m_sendSocket, &QTcpSocket::connected, this, [this]() {
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
        stream.setVersion(QDataStream::Qt_5_15);
        stream << static_cast<quint32>(headerData.size());
        header.append(headerData);

        m_sendSocket->write(header);
        m_sendSocket->flush();

        // Open first file and start sending
        m_currentSendFile = new QFile(m_sendQueue.first().path);
        if (m_currentSendFile->open(QFile::ReadOnly)) {
            sendNextChunk();
        } else {
            emit transferError("Cannot open file for reading: " + m_sendQueue.first().path);
            delete m_currentSendFile;
            m_currentSendFile = nullptr;
        }
    });

    connect(m_sendSocket, &QTcpSocket::bytesWritten, this, &FileTransfer::bytesWritten);
    connect(m_sendSocket, &QTcpSocket::disconnected, this, [this]() {
        if (m_currentSendFile) {
            m_currentSendFile->close();
            delete m_currentSendFile;
            m_currentSendFile = nullptr;
        }
        if (m_sendSocket) {
            m_sendSocket->deleteLater();
            m_sendSocket = nullptr;
        }
    });

    m_sendSocket->connectToHost(targetIp, TRANSFER_PORT);
}

void FileTransfer::sendNextChunk() {
    if (!m_sendSocket || m_currentFileIndex >= m_sendQueue.size()) return;

    const TransferFile &file = m_sendQueue.at(m_currentFileIndex);

    if (m_currentFileOffset == 0) {
        m_currentSendFile = new QFile(file.path);
        if (!m_currentSendFile->open(QFile::ReadOnly)) {
            emit transferError("Cannot open file for reading: " + file.path);
            delete m_currentSendFile;
            m_currentSendFile = nullptr;
            return;
        }
    }

    if (!m_currentSendFile) return;

    QByteArray chunk = m_currentSendFile->read(CHUNK_SIZE);
    if (chunk.isEmpty()) {
        m_currentSendFile->close();
        delete m_currentSendFile;
        m_currentSendFile = nullptr;
        m_currentFileIndex++;
        m_currentFileOffset = 0;
        sendNextChunk();
        return;
    }

    m_sendSocket->write(chunk);
    m_currentFileOffset += chunk.size();
}

void FileTransfer::bytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);
    m_sentBytes += bytes;
    emit transferProgress(m_sentBytes, m_totalBytes);

    if (m_currentFileIndex < m_sendQueue.size()) {
        sendNextChunk();
    } else {
        emit transferCompleted();
    }
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
    return QHostAddress::LocalHost.toString();
}
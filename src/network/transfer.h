#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QQueue>
#include <QMutex>

struct TransferFile {
    QString name;
    QString path;
    qint64 size;
};
Q_DECLARE_METATYPE(TransferFile)

struct TransferRequest {
    QString senderIp;
    QString senderHostname;
    QList<TransferFile> files;
};
Q_DECLARE_METATYPE(TransferRequest)

class FileTransfer : public QObject {
    Q_OBJECT
public:
    static constexpr quint16 TRANSFER_PORT = 45679;
    static constexpr qint64 CHUNK_SIZE = 65536; // 64KB

    explicit FileTransfer(QObject *parent = nullptr);
    ~FileTransfer();

    void startListening();
    void stopListening();
    void sendFiles(const QString &targetIp, const QList<TransferFile> &files);

    QString downloadDirectory() const;
    void setDownloadDirectory(const QString &dir);
    void acceptTransfer();
    void rejectTransfer();

signals:
    void transferRequestReceived(const TransferRequest &request);
    void transferProgress(qint64 bytes, qint64 total);
    void transferCompleted();
    void transferError(const QString &error);
    void transferRejected();

private slots:
    void acceptConnection();
    void readIncomingData();
    void onBytesWritten(qint64 bytes);

private:
    void sendFileData();
    void openNextSendFile();
    void cleanupSendState();
    void resetReceiveState();
    void openNextReceiveFile();
    void handleTransferRequest(const QJsonObject &obj);
    QString getLocalIP() const;

    QTcpServer *m_server;
    QTcpSocket *m_sendSocket;
    QFile *m_currentSendFile;
    QByteArray m_pendingChunk;
    qint64 m_pendingOffset;
    QList<TransferFile> m_sendQueue;
    int m_currentFileIndex;
    qint64 m_currentFileOffset;
    qint64 m_totalBytes;
    qint64 m_sentBytes;
    QString m_downloadDir;
    mutable QMutex m_mutex;

    // Receiver side
    QTcpSocket *m_receiveSocket;
    QFile *m_receiveFile;
    QString m_receiveFilename;
    qint64 m_receiveFilesize;
    qint64 m_receiveOffset;
    bool m_receivingHeader;
    TransferRequest m_pendingRequest;
    int m_currentReceiveFileIndex;
    qint64 m_currentReceiveFileActualSize;
    bool m_transferAccepted;
};
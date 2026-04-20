#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QTimer>

struct DeviceInfo {
    QString hostname;
    QString ip;
    quint16 port;
};

class Discovery : public QObject {
    Q_OBJECT
public:
    static constexpr quint16 DISCOVERY_PORT = 45678;

    explicit Discovery(QObject *parent = nullptr);
    ~Discovery();

    void startBroadcasting(const QString &hostname);
    void stopBroadcasting();
    void scanForDevices();

    QList<DeviceInfo> getDevices() const;

signals:
    void deviceFound(const DeviceInfo &device);
    void deviceLeft(const QString &ip);

private slots:
    void readPendingDatagrams();
    void sendBroadcast();

private:
    void parseDeviceResponse(const QByteArray &data, const QHostAddress &sender);

    QUdpSocket *m_socket;
    QUdpSocket *m_broadcastSocket;
    QTimer *m_broadcastTimer;
    QString m_hostname;
    QList<DeviceInfo> m_devices;
    mutable QMutex m_mutex;
};
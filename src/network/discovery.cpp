#include "discovery.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

Discovery::Discovery(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_broadcastSocket(new QUdpSocket(this))
    , m_broadcastTimer(new QTimer(this))
{
    const bool bound = m_socket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT, QUdpSocket::ShareAddress);
    if (bound) {
        qInfo() << "Discovery UDP socket bound on port" << DISCOVERY_PORT;
    } else {
        qWarning() << "Discovery UDP bind failed on port" << DISCOVERY_PORT << ":" << m_socket->errorString();
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &Discovery::readPendingDatagrams);
    connect(m_broadcastTimer, &QTimer::timeout, this, &Discovery::sendBroadcast);
}

Discovery::~Discovery() {
    stopBroadcasting();
}

void Discovery::startBroadcasting(const QString &hostname) {
    m_hostname = hostname;
    qInfo() << "Discovery broadcasting started for host" << hostname;
    m_broadcastTimer->start(5000);
    sendBroadcast();
}

void Discovery::stopBroadcasting() {
    qInfo() << "Discovery broadcasting stopped";
    m_broadcastTimer->stop();
    if (m_socket) {
        m_socket->close();
    }
    if (m_broadcastSocket) {
        m_broadcastSocket->close();
    }
}

void Discovery::scanForDevices() {
    QJsonObject obj;
    obj["type"] = "scan";
    QByteArray data = QJsonDocument(obj).toJson();
    const qint64 written = m_socket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
    if (written < 0) {
        qWarning() << "Discovery scan datagram failed:" << m_socket->errorString();
    } else {
        qDebug() << "Discovery scan datagram sent, bytes" << written;
    }
}

void Discovery::sendBroadcast() {
    QJsonObject obj;
    obj["type"] = "announce";
    obj["hostname"] = m_hostname;
    obj["port"] = 45679;
    QByteArray data = QJsonDocument(obj).toJson();
    const qint64 written = m_broadcastSocket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
    if (written < 0) {
        qWarning() << "Discovery announce datagram failed:" << m_broadcastSocket->errorString();
    } else {
        qDebug() << "Discovery announce datagram sent, bytes" << written;
    }
}

void Discovery::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        qint64 size = m_socket->pendingDatagramSize();
        QByteArray data(size, '\0');
        QHostAddress sender;
        quint16 port = 0;
        qint64 read = m_socket->readDatagram(data.data(), size, &sender, &port);
        if (read > 0) {
            data.resize(read);
            parseDeviceResponse(data, sender);
        }
    }
}

void Discovery::parseDeviceResponse(const QByteArray &data, const QHostAddress &sender) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Discovery ignored invalid JSON from" << sender.toString();
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "announce" || type == "scan_response") {
        QString senderIp = sender.toString();

        // Filter out local device
        if (!m_localIp.isEmpty() && senderIp == m_localIp) {
            return;
        }

        DeviceInfo info;
        info.hostname = obj["hostname"].toString();
        info.ip = senderIp;
        info.port = obj["port"].toInt(45679);

        QMutexLocker locker(&m_mutex);

        bool found = false;
        for (auto &d : m_devices) {
            if (d.ip == info.ip) {
                d = info;
                found = true;
                break;
            }
        }
        if (!found) {
            m_devices.append(info);
            qInfo() << "Discovery found new device:" << info.hostname << info.ip << info.port;
        }

        emit deviceFound(info);
    }
}

QList<DeviceInfo> Discovery::getDevices() const {
    QMutexLocker locker(&m_mutex);
    return m_devices;
}

#include "discovery.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>

Discovery::Discovery(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_broadcastSocket(new QUdpSocket(this))
    , m_broadcastTimer(new QTimer(this))
{
    m_socket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT, QUdpSocket::ShareAddress);
    connect(m_socket, &QUdpSocket::readyRead, this, &Discovery::readPendingDatagrams);
    connect(m_broadcastTimer, &QTimer::timeout, this, &Discovery::sendBroadcast);
}

Discovery::~Discovery() {
    stopBroadcasting();
}

void Discovery::startBroadcasting(const QString &hostname) {
    m_hostname = hostname;
    m_broadcastTimer->start(5000);
    sendBroadcast();
}

void Discovery::stopBroadcasting() {
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
    m_socket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
}

void Discovery::sendBroadcast() {
    QJsonObject obj;
    obj["type"] = "announce";
    obj["hostname"] = m_hostname;
    obj["port"] = 45679;
    QByteArray data = QJsonDocument(obj).toJson();
    m_broadcastSocket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
}

void Discovery::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        parseDeviceResponse(datagram.data(), datagram.senderAddress());
    }
}

void Discovery::parseDeviceResponse(const QByteArray &data, const QHostAddress &sender) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "announce" || type == "scan_response") {
        DeviceInfo info;
        info.hostname = obj["hostname"].toString();
        info.ip = sender.toString();
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
        }

        emit deviceFound(info);
    }
}

QList<DeviceInfo> Discovery::getDevices() const {
    QMutexLocker locker(&m_mutex);
    return m_devices;
}
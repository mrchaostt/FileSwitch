#include "widgets/devicewidget.h"
#include <QVBoxLayout>
#include <QLabel>

DeviceWidget::DeviceWidget(Discovery *discovery, QWidget *parent)
    : QWidget(parent)
    , m_discovery(discovery)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("设备列表:");
    label->setStyleSheet("font-weight: bold");
    layout->addWidget(label);

    m_deviceList = new QListWidget(this);
    layout->addWidget(m_deviceList);

    m_scanBtn = new QPushButton("扫描设备", this);
    layout->addWidget(m_scanBtn);

    connect(m_scanBtn, &QPushButton::clicked, this, &DeviceWidget::onScanClicked);
    connect(m_deviceList, &QListWidget::itemDoubleClicked, this, &DeviceWidget::onDeviceDoubleClicked);
    connect(m_discovery, &Discovery::deviceFound, this, &DeviceWidget::onDeviceFound);
}

QString DeviceWidget::selectedDeviceIp() const
{
    QListWidgetItem *item = m_deviceList->currentItem();
    if (!item) return QString();
    return item->data(Qt::UserRole).toString();
}

QString DeviceWidget::selectedDeviceHostname() const
{
    QListWidgetItem *item = m_deviceList->currentItem();
    if (!item) return QString();
    return item->text().section(" ", 0, 0);
}

void DeviceWidget::onScanClicked()
{
    m_discovery->scanForDevices();
}

void DeviceWidget::onDeviceDoubleClicked(QListWidgetItem *item)
{
    QString ip = item->data(Qt::UserRole).toString();
    QString hostname = item->text().section(" ", 0, 0);
    emit deviceSelected(ip, hostname);
}

void DeviceWidget::onDeviceFound(const DeviceInfo &info)
{
    QString displayText = QString("%1 (%2)").arg(info.hostname, info.ip);
    QListWidgetItem *item = new QListWidgetItem(displayText, m_deviceList);
    item->setData(Qt::UserRole, info.ip);
}
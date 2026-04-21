#include "devicewidget.h"
#include <QVBoxLayout>
#include <QLabel>

// Claude Design System Constants
#define COLOR_IVORY QColor(250, 249, 245)
#define COLOR_WARM_SAND QColor(232, 230, 220)
#define COLOR_BORDER_CREAM QColor(240, 238, 230)
#define COLOR_CHARCOAL_WARM QColor(77, 76, 72)
#define COLOR_OLIVE_GRAY QColor(94, 93, 89)
#define COLOR_STONE_GRAY QColor(135, 134, 127)
#define COLOR_RING_WARM QColor(209, 207, 197)
#define COLOR_NEAR_BLACK QColor(20, 20, 19)
#define COLOR_TERRACOTTA QColor(201, 100, 66)
#define FONT_SERIF "Georgia, 'Times New Roman', serif"
#define FONT_SANS "system-ui, -apple-system, Arial, sans-serif"

DeviceWidget::DeviceWidget(Discovery *discovery, QWidget *parent)
    : QWidget(parent)
    , m_discovery(discovery)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel *label = new QLabel("设备列表");
    label->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 500;"
        " color: %2;"
        " letter-spacing: 0.12px;"
        " text-transform: uppercase;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name()));
    layout->addWidget(label);

    m_deviceList = new QListWidget(this);
    m_deviceList->setStyleSheet(QString(
        "QListWidget {"
        " background-color: %1;"
        " border: 1px solid %2;"
        " border-radius: 8px;"
        " padding: 6px;"
        " font-family: %3;"
        " font-size: 14px;"
        " color: %4;"
        "}"
        "QListWidget::item {"
        " background-color: transparent;"
        " padding: 10px 14px;"
        " border-radius: 6px;"
        " margin-bottom: 4px;"
        "}"
        "QListWidget::item:selected {"
        " background-color: %5;"
        " color: %6;"
        "}"
        "QListWidget::item:hover:!selected {"
        " background-color: %7;"
        "}"
    ).arg(COLOR_IVORY.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_NEAR_BLACK.name())
     .arg(COLOR_TERRACOTTA.name())
     .arg(COLOR_IVORY.name())
     .arg(COLOR_WARM_SAND.name()));
    m_deviceList->setMinimumHeight(80);
    layout->addWidget(m_deviceList);

    m_scanBtn = new QPushButton("扫描设备");
    m_scanBtn->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: 1px solid %3;"
        " border-radius: 6px;"
        " font-family: %4;"
        " font-size: 12px;"
        " font-weight: 500;"
        " padding: 6px 14px;"
        "}"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(COLOR_WARM_SAND.name())
     .arg(COLOR_CHARCOAL_WARM.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_RING_WARM.name())
     .arg(COLOR_BORDER_CREAM.name()));
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
    QString displayText = QString("%1  •  %2").arg(info.hostname, info.ip);
    QListWidgetItem *item = new QListWidgetItem(displayText, m_deviceList);
    item->setData(Qt::UserRole, info.ip);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}
#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "../network/discovery.h"

class DeviceWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceWidget(Discovery *discovery, QWidget *parent = nullptr);

    QString selectedDeviceIp() const;
    QString selectedDeviceHostname() const;

signals:
    void deviceSelected(const QString &ip, const QString &hostname);

private slots:
    void onScanClicked();
    void onDeviceDoubleClicked(QListWidgetItem *item);
    void onDeviceFound(const DeviceInfo &info);

private:
    Discovery *m_discovery;
    QPushButton *m_scanBtn;
    QListWidget *m_deviceList;
};
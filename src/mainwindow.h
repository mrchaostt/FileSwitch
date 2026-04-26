#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QString>
#include "network/discovery.h"
#include "network/transfer.h"
#include "widgets/devicewidget.h"
#include "widgets/filequeuewidget.h"
#include "widgets/progresswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onSendClicked();
    void onTransferProgress(qint64 bytes, qint64 total);
    void onTransferCompleted();
    void onTransferError(const QString &error);
    void onTransferRequestReceived(const TransferRequest &request);
    void onChangeDownloadDir();

private:
    QString getLocalIP();

    QTabWidget *m_tabWidget;
    Discovery *m_discovery;
    FileTransfer *m_transfer;
    DeviceWidget *m_deviceWidget;
    FileQueueWidget *m_fileQueueWidget;
    ProgressWidget *m_progressWidget;
    QLabel *m_localIpLabel;
    QLabel *m_recvDirLabel;
    QString m_localIp;
};
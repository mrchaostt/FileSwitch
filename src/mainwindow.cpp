#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QAbstractSocket>
#include <QTabWidget>
#include <QListWidget>
#include "widgets/acceptdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileSwitch");

    // Create instances
    m_discovery = new Discovery(this);
    m_transfer = new FileTransfer(this);
    m_localIp = getLocalIP();

    // Load settings
    QSettings settings;
    QString downloadDir = settings.value("downloadDir", QDir::home().filePath("Downloads/FileSwitch")).toString();
    m_transfer->setDownloadDirectory(downloadDir);

    // Start services
    m_discovery->startBroadcasting("FileSwitch");
    m_transfer->startListening();

    m_tabWidget = new QTabWidget(this);

    // ========== Sender Tab ==========
    QWidget *senderWidget = new QWidget;
    QVBoxLayout *senderLayout = new QVBoxLayout(senderWidget);

    // Local IP label at top
    m_localIpLabel = new QLabel(QString("本机IP: %1").arg(m_localIp));
    senderLayout->addWidget(m_localIpLabel);

    // Device widget (scans for devices)
    m_deviceWidget = new DeviceWidget(m_discovery, this);
    senderLayout->addWidget(m_deviceWidget);

    // File queue widget
    m_fileQueueWidget = new FileQueueWidget(this);
    senderLayout->addWidget(m_fileQueueWidget);

    // Send button
    QPushButton *sendBtn = new QPushButton("发送文件");
    senderLayout->addWidget(sendBtn);

    // Progress widget at bottom
    m_progressWidget = new ProgressWidget(this);
    senderLayout->addWidget(m_progressWidget);

    senderWidget->setLayout(senderLayout);

    // ========== Receiver Tab ==========
    QWidget *receiverWidget = new QWidget;
    QVBoxLayout *receiverLayout = new QVBoxLayout(receiverWidget);

    // Local IP label
    QLabel *recvIpLabel = new QLabel(QString("本机IP: %1").arg(m_localIp));
    receiverLayout->addWidget(recvIpLabel);

    // Status label
    QLabel *statusLabel = new QLabel("等待传输请求...");
    statusLabel->setObjectName("recvStatusLabel");
    receiverLayout->addWidget(statusLabel);

    // Download directory display and change button
    QLabel *dirLabel = new QLabel(QString("下载目录: %1").arg(downloadDir));
    dirLabel->setObjectName("recvDirLabel");
    QPushButton *changeBtn = new QPushButton("更改");

    QHBoxLayout *dirLayout = new QHBoxLayout;
    dirLayout->addWidget(dirLabel);
    dirLayout->addWidget(changeBtn);

    receiverLayout->addLayout(dirLayout);

    // Progress widget at bottom
    ProgressWidget *recvProgressWidget = new ProgressWidget(this);
    recvProgressWidget->setObjectName("recvProgressWidget");
    receiverLayout->addWidget(recvProgressWidget);

    receiverWidget->setLayout(receiverLayout);

    // ========== Final Setup ==========
    m_tabWidget->addTab(senderWidget, "发送");
    m_tabWidget->addTab(receiverWidget, "接收");

    setCentralWidget(m_tabWidget);
    resize(600, 500);

    // Connect signals
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(changeBtn, &QPushButton::clicked, this, &MainWindow::onChangeDownloadDir);
    connect(m_transfer, &FileTransfer::transferProgress, this, &MainWindow::onTransferProgress);
    connect(m_transfer, &FileTransfer::transferCompleted, this, &MainWindow::onTransferCompleted);
    connect(m_transfer, &FileTransfer::transferError, this, &MainWindow::onTransferError);
    connect(m_transfer, &FileTransfer::transferRequestReceived, this, &MainWindow::onTransferRequestReceived);
}

void MainWindow::onSendClicked()
{
    QString ip = m_deviceWidget->selectedDeviceIp();
    QList<TransferFile> files = m_fileQueueWidget->getFiles();

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择一个设备");
        return;
    }

    if (files.isEmpty()) {
        QMessageBox::warning(this, "警告", "请添加要发送的文件");
        return;
    }

    m_progressWidget->setStatus("正在连接...");
    m_transfer->sendFiles(ip, files);
}

void MainWindow::onTransferProgress(qint64 bytes, qint64 total)
{
    m_progressWidget->setProgress(bytes, total);
}

void MainWindow::onTransferCompleted()
{
    m_progressWidget->setStatus("传输完成");
    QMessageBox::information(this, "传输完成", "文件传输已完成");
}

void MainWindow::onTransferError(const QString &error)
{
    m_progressWidget->setStatus("传输错误");
    QMessageBox::critical(this, "传输错误", error);
}

void MainWindow::onTransferRequestReceived(const TransferRequest &request)
{
    AcceptDialog dialog(request, m_transfer->downloadDirectory(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_progressWidget->setStatus("正在接收...");
    } else {
        m_progressWidget->setStatus("接收被拒绝");
    }
}

void MainWindow::onChangeDownloadDir()
{
    QSettings settings;
    QString currentDir = m_transfer->downloadDirectory();
    QString newDir = QFileDialog::getExistingDirectory(this, "选择下载目录", currentDir);

    if (!newDir.isEmpty()) {
        m_transfer->setDownloadDirectory(newDir);
        settings.setValue("downloadDir", newDir);

        // Update label
        QWidget *receiverWidget = m_tabWidget->widget(1);
        QLayout *layout = receiverWidget->layout();
        if (QLayoutItem *item = layout->itemAt(2)) { // dirLayout is at index 2
            if (QHBoxLayout *hLayout = item->layout()) {
                if (QLabel *label = hLayout->findChild<QLabel *>("recvDirLabel")) {
                    label->setText(QString("下载目录: %1").arg(newDir));
                }
            }
        }
    }
}

QString MainWindow::getLocalIP()
{
    QString ip;
    QHostAddress addr;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                !addr.isLoopback()) {
                ip = addr.toString();
                break;
            }
        }
        if (!ip.isEmpty()) break;
    }
    return ip;
}
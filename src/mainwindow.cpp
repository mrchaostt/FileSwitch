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

// Claude Design System Constants
#define COLOR_PARCHMENT QColor(245, 244, 237)    // #f5f4ed - page background
#define COLOR_IVORY QColor(250, 249, 245)        // #faf9f5 - card surface
#define COLOR_TERRACOTTA QColor(201, 100, 66)    // #c96442 - brand CTA
#define COLOR_NEAR_BLACK QColor(20, 20, 19)       // #141413 - primary text
#define COLOR_WARM_SAND QColor(232, 230, 220)     // #e8e6dc - button bg
#define COLOR_CHARCOAL_WARM QColor(77, 76, 72)   // #4d4c48 - button text
#define COLOR_OLIVE_GRAY QColor(94, 93, 89)      // #5e5d59 - secondary text
#define COLOR_STONE_GRAY QColor(135, 134, 127)    // #87867f - tertiary text
#define COLOR_BORDER_CREAM QColor(240, 238, 230)  // #f0eee6 - borders
#define COLOR_WARM_SILVER QColor(176, 174, 165)   // #b0aea5 - text on dark
#define COLOR_DARK_SURFACE QColor(48, 48, 46)     // #30302e - dark surface
#define COLOR_WHITE QColor(255, 255, 255)          // #ffffff
#define COLOR_RING_WARM QColor(209, 207, 197)     // #d1cfc5 - ring shadow
#define FONT_SERIF "Georgia, 'Times New Roman', serif"
#define FONT_SANS "system-ui, -apple-system, Arial, sans-serif"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileSwitch");
    setMinimumSize(480, 400);
    setStyleSheet(QString("QMainWindow { background-color: %1; }").arg(COLOR_PARCHMENT.name()));

    // Create instances
    m_discovery = new Discovery(this);
    m_transfer = new FileTransfer(this);
    m_localIp = getLocalIP();

    // Load settings
    QSettings settings;
    QString downloadDir = settings.value("downloadDir", QDir::home().filePath("Downloads/FileSwitch")).toString();
    m_transfer->setDownloadDirectory(downloadDir);

    // Set local IP for filtering (exclude self from device list)
    m_discovery->setLocalIp(m_localIp);

    // Start services
    m_discovery->startBroadcasting("FileSwitch");
    m_transfer->startListening();

    // ========== Central Widget ==========
    QWidget *centralWidget = new QWidget;
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(16, 12, 16, 12);
    centralLayout->setSpacing(12);

    // App Title - Anthropic Serif style
    QLabel *titleLabel = new QLabel("FileSwitch");
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 28px;"
        " font-weight: 500;"
        " color: %2;"
        " line-height: 1.10;"
        "}"
    ).arg(FONT_SERIF).arg(COLOR_NEAR_BLACK.name()));
    centralLayout->addWidget(titleLabel);

    // Subtitle
    QLabel *subtitleLabel = new QLabel("局域网文件传输");
    subtitleLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 14px;"
        " font-weight: 400;"
        " color: %2;"
        " line-height: 1.60;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_OLIVE_GRAY.name()));
    centralLayout->addWidget(subtitleLabel);

    m_tabWidget = new QTabWidget(this);

    // ========== Sender Tab ==========
    QWidget *senderWidget = new QWidget;
    senderWidget->setStyleSheet(QString(
        "QWidget { background-color: %1; border-radius: 10px; }"
    ).arg(COLOR_IVORY.name()));
    QVBoxLayout *senderLayout = new QVBoxLayout(senderWidget);
    senderLayout->setContentsMargins(16, 16, 16, 16);
    senderLayout->setSpacing(12);

    // Local IP label - card style
    m_localIpLabel = new QLabel(QString("本机 IP: %1").arg(m_localIp));
    m_localIpLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        " background-color: %3;"
        " padding: 6px 12px;"
        " border-radius: 6px;"
        " border: 1px solid %4;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_OLIVE_GRAY.name())
     .arg(COLOR_WARM_SAND.name()).arg(COLOR_BORDER_CREAM.name()));
    senderLayout->addWidget(m_localIpLabel);

    // Device widget
    m_deviceWidget = new DeviceWidget(m_discovery, this);
    senderLayout->addWidget(m_deviceWidget, 1);

    // File queue widget
    m_fileQueueWidget = new FileQueueWidget(this);
    senderLayout->addWidget(m_fileQueueWidget, 1);

    // Send button - Terracotta Brand CTA
    QPushButton *sendBtn = new QPushButton("发送文件");
    sendBtn->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: none;"
        " border-radius: 8px;"
        " font-family: %3;"
        " font-size: 14px;"
        " font-weight: 500;"
        " padding: 8px 20px;"
        "}"
        "QPushButton:hover { background-color: #b85a3a; }"
        "QPushButton:pressed { background-color: #a34f32; }"
    ).arg(COLOR_TERRACOTTA.name()).arg(COLOR_WHITE.name()).arg(FONT_SANS));
    senderLayout->addWidget(sendBtn);

    // Progress widget
    m_progressWidget = new ProgressWidget(this);
    senderLayout->addWidget(m_progressWidget);

    senderWidget->setLayout(senderLayout);

    // ========== Receiver Tab ==========
    QWidget *receiverWidget = new QWidget;
    receiverWidget->setStyleSheet(QString(
        "QWidget { background-color: %1; border-radius: 10px; }"
    ).arg(COLOR_IVORY.name()));
    QVBoxLayout *receiverLayout = new QVBoxLayout(receiverWidget);
    receiverLayout->setContentsMargins(16, 16, 16, 16);
    receiverLayout->setSpacing(12);

    // Local IP label
    QLabel *recvIpLabel = new QLabel(QString("本机 IP: %1").arg(m_localIp));
    recvIpLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        " background-color: %3;"
        " padding: 6px 12px;"
        " border-radius: 6px;"
        " border: 1px solid %4;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_OLIVE_GRAY.name())
     .arg(COLOR_WARM_SAND.name()).arg(COLOR_BORDER_CREAM.name()));
    receiverLayout->addWidget(recvIpLabel);

    // Status label - Serif style headline
    QLabel *statusLabel = new QLabel("等待传输请求...");
    statusLabel->setObjectName("recvStatusLabel");
    statusLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 20px;"
        " font-weight: 500;"
        " color: %2;"
        " line-height: 1.20;"
        "}"
    ).arg(FONT_SERIF).arg(COLOR_NEAR_BLACK.name()));
    receiverLayout->addWidget(statusLabel);

    // Spacer
    receiverLayout->addStretch();

    // Download directory
    QLabel *dirLabel = new QLabel(QString("下载目录: %1").arg(downloadDir));
    dirLabel->setObjectName("recvDirLabel");
    dirLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        " line-height: 1.43;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name()));
    dirLabel->setWordWrap(true);

    QPushButton *changeBtn = new QPushButton("更改");
    changeBtn->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: 1px solid %3;"
        " border-radius: 6px;"
        " font-family: %4;"
        " font-size: 12px;"
        " font-weight: 500;"
        " padding: 6px 12px;"
        "}"
        "QPushButton:hover { background-color: %5; }"
    ).arg(COLOR_WARM_SAND.name())
     .arg(COLOR_CHARCOAL_WARM.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_RING_WARM.name()));

    QHBoxLayout *dirLayout = new QHBoxLayout;
    dirLayout->addWidget(dirLabel, 1);
    dirLayout->addWidget(changeBtn);

    receiverLayout->addLayout(dirLayout);

    // Progress widget
    ProgressWidget *recvProgressWidget = new ProgressWidget(this);
    recvProgressWidget->setObjectName("recvProgressWidget");
    receiverLayout->addWidget(recvProgressWidget);

    receiverWidget->setLayout(receiverLayout);

    // ========== Tab Styling ==========
    m_tabWidget->addTab(senderWidget, "发送");
    m_tabWidget->addTab(receiverWidget, "接收");
    m_tabWidget->setStyleSheet(QString(
        "QTabWidget { background-color: transparent; border: none; }"
        "QTabWidget::pane { background-color: transparent; border: none; }"
        "QTabBar { background-color: transparent; border: none; }"
        "QTabBar::tab {"
        " font-family: %1;"
        " font-size: 14px;"
        " font-weight: 400;"
        " color: %2;"
        " background-color: transparent;"
        " padding: 8px 16px;"
        " margin-right: 12px;"
        " border: none;"
        "}"
        "QTabBar::tab:selected { color: %3; border-bottom: 2px solid %3; }"
        "QTabBar::tab:hover:!selected { color: %4; }"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name())
     .arg(COLOR_TERRACOTTA.name()).arg(COLOR_OLIVE_GRAY.name()));

    centralLayout->addWidget(m_tabWidget);

    setCentralWidget(centralWidget);
    resize(640, 560);

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
        if (QLayoutItem *item = layout->itemAt(2)) {
            if (QHBoxLayout *hLayout = qobject_cast<QHBoxLayout *>(item->layout())) {
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
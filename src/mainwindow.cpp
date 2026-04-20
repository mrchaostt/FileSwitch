#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include "network/transfer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileSwitch");

    m_tabWidget = new QTabWidget(this);

    // Sender tab
    QWidget *senderWidget = new QWidget;
    QVBoxLayout *senderLayout = new QVBoxLayout(senderWidget);
    senderLayout->addWidget(new QLabel("Sender Mode - placeholder"));
    senderWidget->setLayout(senderLayout);

    // Receiver tab
    QWidget *receiverWidget = new QWidget;
    QVBoxLayout *receiverLayout = new QVBoxLayout(receiverWidget);

    // Download directory display and change button
    QSettings settings;
    QString downloadDir = settings.value("downloadDir", QDir::home().filePath("Downloads/FileSwitch")).toString();

    m_transfer = new FileTransfer(this);
    m_transfer->setDownloadDirectory(downloadDir);

    QLabel *dirLabel = new QLabel(QString("下载目录: %1").arg(downloadDir));
    QPushButton *changeBtn = new QPushButton("更改");

    QHBoxLayout *dirLayout = new QHBoxLayout;
    dirLayout->addWidget(dirLabel);
    dirLayout->addWidget(changeBtn);

    receiverLayout->addLayout(dirLayout);
    receiverLayout->addWidget(new QLabel("Receiver Mode - placeholder"));
    receiverWidget->setLayout(receiverLayout);

    m_tabWidget->addTab(senderWidget, "发送");
    m_tabWidget->addTab(receiverWidget, "接收");

    setCentralWidget(m_tabWidget);
    resize(600, 500);

    connect(changeBtn, &QPushButton::clicked, this, &MainWindow::onChangeDownloadDir);
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
        if (QLayoutItem *item = layout->itemAt(0)) {
            if (QHBoxLayout *hLayout = item->layout()) {
                if (QLabel *label = qobject_cast<QLabel *>(hLayout->itemAt(0)->widget())) {
                    label->setText(QString("下载目录: %1").arg(newDir));
                }
            }
        }
    }
}
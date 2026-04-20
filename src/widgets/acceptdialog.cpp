#include "acceptdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

AcceptDialog::AcceptDialog(const TransferRequest &request,
                           const QString &downloadDir,
                           QWidget *parent)
    : QDialog(parent)
    , m_accepted(false)
{
    setWindowTitle("文件传输请求");
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Sender info
    mainLayout->addWidget(new QLabel(QString("收到来自: %1 (%2)")
        .arg(request.senderHostname).arg(request.senderIp), this));

    // File list label
    mainLayout->addWidget(new QLabel("文件列表:", this));

    // File list widget
    QListWidget *fileListWidget = new QListWidget(this);
    qint64 totalSize = 0;
    for (const TransferFile &file : request.files) {
        QString itemText = QString("%1 (%2)").arg(file.name).arg(formatSize(file.size));
        fileListWidget->addItem(itemText);
        totalSize += file.size;
    }
    mainLayout->addWidget(fileListWidget);

    // Summary
    mainLayout->addWidget(new QLabel(QString("共 %1 个文件, 总计 %2")
        .arg(request.files.size()).arg(formatSize(totalSize)), this));

    // Download directory
    mainLayout->addWidget(new QLabel(QString("保存到: %1").arg(downloadDir), this));

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *rejectButton = new QPushButton("拒绝", this);
    QPushButton *acceptButton = new QPushButton("接受", this);
    buttonLayout->addWidget(rejectButton);
    buttonLayout->addWidget(acceptButton);

    mainLayout->addLayout(buttonLayout);

    connect(rejectButton, &QPushButton::clicked, this, &AcceptDialog::onRejectClicked);
    connect(acceptButton, &QPushButton::clicked, this, &AcceptDialog::onAcceptClicked);
}

void AcceptDialog::onAcceptClicked()
{
    m_accepted = true;
    accept();
}

void AcceptDialog::onRejectClicked()
{
    m_accepted = false;
    reject();
}

QString AcceptDialog::formatSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString::number(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
}
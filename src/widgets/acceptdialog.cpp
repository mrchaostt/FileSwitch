#include "acceptdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

// Claude Design System Constants
#define COLOR_PARCHMENT QColor(245, 244, 237)
#define COLOR_IVORY QColor(250, 249, 245)
#define COLOR_WARM_SAND QColor(232, 230, 220)
#define COLOR_BORDER_CREAM QColor(240, 238, 230)
#define COLOR_CHARCOAL_WARM QColor(77, 76, 72)
#define COLOR_OLIVE_GRAY QColor(94, 93, 89)
#define COLOR_STONE_GRAY QColor(135, 134, 127)
#define COLOR_NEAR_BLACK QColor(20, 20, 19)
#define COLOR_WHITE QColor(255, 255, 255)
#define COLOR_TERRACOTTA QColor(201, 100, 66)
#define COLOR_ERROR_CRIMSON QColor(181, 51, 51)
#define COLOR_WARM_SILVER QColor(176, 174, 165)
#define COLOR_DARK_SURFACE QColor(48, 48, 46)
#define FONT_SERIF "Georgia, 'Times New Roman', serif"
#define FONT_SANS "system-ui, -apple-system, Arial, sans-serif"

AcceptDialog::AcceptDialog(const TransferRequest &request,
                           const QString &downloadDir,
                           QWidget *parent)
    : QDialog(parent)
    , m_accepted(false)
{
    setWindowTitle("文件传输请求");
    setMinimumWidth(420);
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(COLOR_PARCHMENT.name()));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Title - Serif style
    QLabel *titleLabel = new QLabel("收到文件传输请求", this);
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 22px;"
        " font-weight: 500;"
        " color: %2;"
        " line-height: 1.20;"
        "}"
    ).arg(FONT_SERIF).arg(COLOR_NEAR_BLACK.name()));
    mainLayout->addWidget(titleLabel);

    // Sender info - Terracotta card style
    QFrame *senderFrame = new QFrame(this);
    senderFrame->setStyleSheet(QString(
        "QFrame {"
        " background-color: %1;"
        " border-radius: 8px;"
        " padding: 12px 16px;"
        "}"
    ).arg(COLOR_TERRACOTTA.name()));
    QHBoxLayout *senderLayout = new QHBoxLayout(senderFrame);
    senderLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *senderLabel = new QLabel(
        QString("来自: %1 (%2)").arg(request.senderHostname, request.senderIp), senderFrame);
    senderLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 14px;"
        " font-weight: 400;"
        " color: %2;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_WHITE.name()));
    senderLayout->addWidget(senderLabel);
    mainLayout->addWidget(senderFrame);

    // File list label
    QLabel *filesLabel = new QLabel("文件列表", this);
    filesLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 11px;"
        " font-weight: 500;"
        " color: %2;"
        " letter-spacing: 0.12px;"
        " text-transform: uppercase;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name()));
    mainLayout->addWidget(filesLabel);

    // File list widget
    QListWidget *fileListWidget = new QListWidget(this);
    fileListWidget->setStyleSheet(QString(
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
        " padding: 8px 12px;"
        " border-radius: 6px;"
        " margin-bottom: 4px;"
        "}"
    ).arg(COLOR_IVORY.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_NEAR_BLACK.name()));
    qint64 totalSize = 0;
    for (const TransferFile &file : request.files) {
        QString itemText = QString("%1  •  %2").arg(file.name).arg(formatSize(file.size));
        fileListWidget->addItem(itemText);
        totalSize += file.size;
    }
    mainLayout->addWidget(fileListWidget);

    // Summary
    QLabel *summaryLabel = new QLabel(
        QString("共 %1 个文件，总计 %2").arg(request.files.size()).arg(formatSize(totalSize)), this);
    summaryLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_OLIVE_GRAY.name()));
    mainLayout->addWidget(summaryLabel);

    // Download directory
    QLabel *dirLabel = new QLabel(QString("保存到: %1").arg(downloadDir), this);
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
    mainLayout->addWidget(dirLabel);

    // Spacer
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    QPushButton *rejectButton = new QPushButton("拒绝", this);
    rejectButton->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: none;"
        " border-radius: 8px;"
        " font-family: %3;"
        " font-size: 14px;"
        " font-weight: 500;"
        " padding: 10px 20px;"
        "}"
        "QPushButton:hover { background-color: #cf3d3d; }"
        "QPushButton:pressed { background-color: #a32d2d; }"
    ).arg(COLOR_ERROR_CRIMSON.name())
     .arg(COLOR_WHITE.name())
     .arg(FONT_SANS));

    QPushButton *acceptButton = new QPushButton("接受", this);
    acceptButton->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: none;"
        " border-radius: 8px;"
        " font-family: %3;"
        " font-size: 14px;"
        " font-weight: 500;"
        " padding: 10px 20px;"
        "}"
        "QPushButton:hover { background-color: #b85a3a; }"
        "QPushButton:pressed { background-color: #a34f32; }"
    ).arg(COLOR_TERRACOTTA.name())
     .arg(COLOR_WHITE.name())
     .arg(FONT_SANS));

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
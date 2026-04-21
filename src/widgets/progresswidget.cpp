#include "progresswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

// Claude Design System Constants
#define COLOR_IVORY QColor(250, 249, 245)
#define COLOR_WARM_SAND QColor(232, 230, 220)
#define COLOR_BORDER_CREAM QColor(240, 238, 230)
#define COLOR_OLIVE_GRAY QColor(94, 93, 89)
#define COLOR_STONE_GRAY QColor(135, 134, 127)
#define COLOR_TERRACOTTA QColor(201, 100, 66)
#define COLOR_NEAR_BLACK QColor(20, 20, 19)
#define FONT_SANS "system-ui, -apple-system, Arial, sans-serif"

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_progressBar(new QProgressBar(this))
    , m_statusLabel(new QLabel("就绪", this))
    , m_speedLabel(new QLabel(this))
{
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(6);

    m_statusLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_OLIVE_GRAY.name()));

    m_speedLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 400;"
        " color: %2;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name()));

    m_progressBar->setStyleSheet(QString(
        "QProgressBar {"
        " background-color: %1;"
        " border: 1px solid %2;"
        " border-radius: 5px;"
        " height: 8px;"
        "}"
        "QProgressBar::chunk {"
        " background-color: %3;"
        " border-radius: 4px;"
        "}"
    ).arg(COLOR_WARM_SAND.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(COLOR_TERRACOTTA.name()));

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(m_statusLabel);
    topLayout->addWidget(m_speedLabel);
    topLayout->addStretch();

    mainLayout->addWidget(m_progressBar);
    mainLayout->addLayout(topLayout);

    setLayout(mainLayout);
}

void ProgressWidget::setProgress(qint64 current, qint64 total)
{
    if (total > 0) {
        int percent = static_cast<int>((current * 100) / total);
        m_progressBar->setValue(percent);
    }
    m_speedLabel->setText(formatSize(current) + " / " + formatSize(total));
}

void ProgressWidget::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void ProgressWidget::reset()
{
    m_progressBar->setValue(0);
    m_statusLabel->setText("就绪");
    m_speedLabel->setText("");
}

QString ProgressWidget::formatSize(qint64 bytes) const
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
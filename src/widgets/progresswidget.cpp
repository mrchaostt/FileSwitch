#include "progresswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
    , m_progressBar(new QProgressBar(this))
    , m_statusLabel(new QLabel("就绪", this))
    , m_speedLabel(new QLabel(this))
{
    m_progressBar->setRange(0, 100);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(m_statusLabel);
    topLayout->addWidget(m_speedLabel);

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
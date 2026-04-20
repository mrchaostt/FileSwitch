#pragma once
#include <QWidget>
#include <QProgressBar>
#include <QLabel>

class ProgressWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProgressWidget(QWidget *parent = nullptr);

    void setProgress(qint64 current, qint64 total);
    void setStatus(const QString &status);
    void reset();

private:
    QString formatSize(qint64 bytes) const;

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_speedLabel;
};
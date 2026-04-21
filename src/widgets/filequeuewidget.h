#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "../network/transfer.h"

class FileQueueWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileQueueWidget(QWidget *parent = nullptr);

    QList<TransferFile> getFiles() const;
    void clearQueue();

signals:
    void filesReady();

private slots:
    void onAddFilesClicked();
    void onAddFolderClicked();
    void onRemoveClicked();
    void onClearClicked();

private:
    void addFilesToQueue(const QStringList &paths);
    QString formatSize(qint64 bytes) const;

    QListWidget *m_fileList;
    QPushButton *m_addFilesBtn;
    QPushButton *m_addFolderBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_clearBtn;
};
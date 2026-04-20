#pragma once
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include "network/transfer.h"

class AcceptDialog : public QDialog {
    Q_OBJECT
public:
    explicit AcceptDialog(const TransferRequest &request,
                           const QString &downloadDir,
                           QWidget *parent = nullptr);

    bool isAccepted() const { return m_accepted; }

private slots:
    void onAcceptClicked();
    void onRejectClicked();

private:
    QString formatSize(qint64 bytes) const;

    bool m_accepted;
};
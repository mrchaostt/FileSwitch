#pragma once
#include <QMainWindow>
#include <QTabWidget>

class FileTransfer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onChangeDownloadDir();

private:
    QTabWidget *m_tabWidget;
    FileTransfer *m_transfer;
};
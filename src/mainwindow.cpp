#include "mainwindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>

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
    receiverLayout->addWidget(new QLabel("Receiver Mode - placeholder"));
    receiverWidget->setLayout(receiverLayout);

    m_tabWidget->addTab(senderWidget, "发送");
    m_tabWidget->addTab(receiverWidget, "接收");

    setCentralWidget(m_tabWidget);
    resize(600, 500);
}
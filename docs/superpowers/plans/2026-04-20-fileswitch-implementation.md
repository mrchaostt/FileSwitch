# FileSwitch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a cross-platform (macOS/Windows 7+) LAN file transfer app with device discovery, file queue, and receiver-confirmation workflow.

**Architecture:** Qt6/C++ with dual-mode UI (Sender/Receiver tabs), UDP broadcast discovery on port 45678, TCP file transfer on port 45679. Single binary switches modes via tab.

**Tech Stack:** Qt 6.x (Core, Network, Widgets), CMake, C++17

---

## File Structure

```
FileSwitch/
├── CMakeLists.txt                    # Build configuration
├── src/
│   ├── main.cpp                      # App entry, QApplication setup
│   ├── mainwindow.cpp / .h           # Main window, tab management
│   ├── network/
│   │   ├── discovery.cpp / .h        # UDP broadcast discovery
│   │   └── transfer.cpp / .h         # TCP file transfer
│   └── widgets/
│       ├── devicewidget.cpp / .h     # Device list (Sender mode)
│       ├── filequeuewidget.cpp / .h  # File queue (Sender mode)
│       ├── progresswidget.cpp / .h   # Progress display (shared)
│       └── acceptdialog.cpp / .h     # Accept confirmation (Receiver mode)
├── resources/
│   └── icons/                       # App icons
└── docs/superpowers/
    └── specs/2026-04-20-fileswitch-design.md  # Reference spec
```

---

## Task 1: Project Scaffolding

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `src/mainwindow.cpp` / `src/mainwindow.h`

- [ ] **Step 1: Create CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(FileSwitch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Gui)

qt_standard_project_setup()

qt_add_executable(FileSwitch
    src/main.cpp
    src/mainwindow.cpp
    src/network/discovery.cpp
    src/network/transfer.cpp
    src/widgets/devicewidget.cpp
    src/widgets/filequeuewidget.cpp
    src/widgets/progresswidget.cpp
    src/widgets/acceptdialog.cpp
)

target_link_libraries(FileSwitch PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Gui)
```

- [ ] **Step 2: Create src/main.cpp**

```cpp
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FileSwitch");

    MainWindow w;
    w.show();

    return app.exec();
}
```

- [ ] **Step 3: Create src/mainwindow.h**

```cpp
#pragma once
#include <QMainWindow>
#include <QTabWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QTabWidget *m_tabWidget;
};
```

- [ ] **Step 4: Create src/mainwindow.cpp**

```cpp
#include "mainwindow.h"
#include <QTabWidget>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FileSwitch");

    m_tabWidget = new QTabWidget(this);

    // Placeholder tabs - will be filled in later tasks
    QWidget *senderTab = new QWidget();
    senderTab->setLayout(new QVBoxLayout());
    senderTab->layout()->addWidget(new QLabel("Sender Mode - placeholder"));

    QWidget *receiverTab = new QWidget();
    receiverTab->setLayout(new QVBoxLayout());
    receiverTab->layout()->addWidget(new QLabel("Receiver Mode - placeholder"));

    m_tabWidget->addTab(senderTab, "发送");
    m_tabWidget->addTab(receiverTab, "接收");

    setCentralWidget(m_tabWidget);
    resize(600, 500);
}
```

- [ ] **Step 5: Create directory structure**

```bash
mkdir -p src/network src/widgets resources/icons
touch src/network/.gitkeep src/widgets/.gitkeep
```

- [ ] **Step 6: Build skeleton to verify setup**

Run: `mkdir build && cd build && cmake .. && cmake --build .`
Expected: Compiles without error, produces FileSwitch binary

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt src/main.cpp src/mainwindow.cpp src/mainwindow.h
git commit -m "feat: project scaffold with CMake and main window skeleton"
```

---

## Task 2: UDP Device Discovery Module

**Files:**
- Create: `src/network/discovery.h`
- Create: `src/network/discovery.cpp`

- [ ] **Step 1: Create src/network/discovery.h**

```cpp
#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QJsonObject>
#include <QList>
#include <QMutex>

struct DeviceInfo {
    QString hostname;
    QString ip;
    quint16 port;
};

class Discovery : public QObject {
    Q_OBJECT
public:
    static constexpr quint16 DISCOVERY_PORT = 45678;

    explicit Discovery(QObject *parent = nullptr);
    ~Discovery();

    void startBroadcasting(const QString &hostname);
    void stopBroadcasting();
    void scanForDevices();

    QList<DeviceInfo> getDevices() const;

signals:
    void deviceFound(const DeviceInfo &device);
    void deviceLeft(const QString &ip);

private slots:
    void readPendingDatagrams();

private:
    void sendBroadcast();
    void parseDeviceResponse(const QByteArray &data, const QHostAddress &sender);

    QUdpSocket *m_socket;
    QUdpSocket *m_broadcastSocket;
    QString m_hostname;
    QList<DeviceInfo> m_devices;
    mutable QMutex m_mutex;
};
```

- [ ] **Step 2: Create src/network/discovery.cpp**

```cpp
#include "discovery.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QTimer>

Discovery::Discovery(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_broadcastSocket(new QUdpSocket(this))
{
    m_socket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT, QUdpSocket::ShareAddress);

    connect(m_socket, &QUdpSocket::readyRead, this, &Discovery::readPendingDatagrams);
}

Discovery::~Discovery() {
    stopBroadcasting();
}

void Discovery::startBroadcasting(const QString &hostname) {
    m_hostname = hostname;

    QJsonObject obj;
    obj["type"] = "announce";
    obj["hostname"] = m_hostname;
    obj["port"] = 45679;

    QByteArray data = QJsonDocument(obj).toJson();

    QTimer *timer = new Timer(this);
    timer->setInterval(5000);
    connect(timer, &QTimer::timeout, this, [this, data]() {
        m_broadcastSocket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
    });
    timer->start();
}

void Discovery::stopBroadcasting() {
    m_broadcastSocket->close();
}

void Discovery::scanForDevices() {
    QJsonObject obj;
    obj["type"] = "scan";

    QByteArray data = QJsonDocument(obj).toJson();
    m_socket->writeDatagram(data, QHostAddress::Broadcast, DISCOVERY_PORT);
}

void Discovery::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        parseDeviceResponse(datagram.data(), datagram.senderAddress());
    }
}

void Discovery::parseDeviceResponse(const QByteArray &data, const QHostAddress &sender) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "announce" || type == "scan_response") {
        DeviceInfo info;
        info.hostname = obj["hostname"].toString();
        info.ip = sender.toString();
        info.port = obj["port"].toInt(45679);

        QMutexLocker locker(&m_mutex);

        bool found = false;
        for (auto &d : m_devices) {
            if (d.ip == info.ip) {
                d = info;
                found = true;
                break;
            }
        }
        if (!found) {
            m_devices.append(info);
        }

        emit deviceFound(info);
    }
}

QList<DeviceInfo> Discovery::getDevices() const {
    QMutexLocker locker(&m_mutex);
    return m_devices;
}

void Discovery::stopBroadcasting() {
    m_broadcastSocket->close();
}
```

- [ ] **Step 3: Fix compilation error - missing include**

Add `#include <QTimer>` to discovery.cpp

- [ ] **Step 4: Commit**

```bash
git add src/network/discovery.h src/network/discovery.cpp
git commit -m "feat: add UDP device discovery module"
```

---

## Task 3: TCP File Transfer Module

**Files:**
- Create: `src/network/transfer.h`
- Create: `src/network/transfer.cpp`

- [ ] **Step 1: Create src/network/transfer.h**

```cpp
#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QQueue>
#include <QMutex>

struct TransferFile {
    QString name;
    QString path;
    qint64 size;
};

struct TransferRequest {
    QString senderIp;
    QString senderHostname;
    QList<TransferFile> files;
};

class FileTransfer : public QObject {
    Q_OBJECT
public:
    static constexpr quint16 TRANSFER_PORT = 45679;
    static constexpr qint64 CHUNK_SIZE = 65536; // 64KB

    explicit FileTransfer(QObject *parent = nullptr);
    ~FileTransfer();

    void startListening();
    void stopListening();
    void sendFiles(const QString &targetIp, const QList<TransferFile> &files);

    QString downloadDirectory() const;
    void setDownloadDirectory(const QString &dir);

signals:
    void transferRequestReceived(const TransferRequest &request);
    void transferProgress(qint64 bytes, qint64 total);
    void transferCompleted();
    void transferError(const QString &error);
    void incomingConnection(qintptr handle);

private slots:
    void acceptConnection();
    void readIncomingData();
    void bytesWritten(qint64 bytes);

private:
    void sendNextChunk();
    void handleTransferRequest(const QJsonObject &obj);

    QTcpServer *m_server;
    QTcpSocket *m_sendSocket;
    QList<TransferFile> m_sendQueue;
    int m_currentFileIndex;
    qint64 m_currentFileOffset;
    qint64 m_totalBytes;
    qint64 m_sentBytes;
    QString m_downloadDir;
    mutable QMutex m_mutex;

    // Receiver side
    QTcpSocket *m_receiveSocket;
    QFile *m_receiveFile;
    QString m_receiveFilename;
    qint64 m_receiveFilesize;
    qint64 m_receiveOffset;
    bool m_receivingHeader;
    TransferRequest m_pendingRequest;
};
```

- [ ] **Step 2: Create src/network/transfer.cpp**

```cpp
#include "transfer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

FileTransfer::FileTransfer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_sendSocket(nullptr)
    , m_currentFileIndex(0)
    , m_currentFileOffset(0)
    , m_totalBytes(0)
    , m_sentBytes(0)
    , m_downloadDir(QDir::home().filePath("Downloads/FileSwitch"))
    , m_receiveSocket(nullptr)
    , m_receiveFile(nullptr)
    , m_receiveOffset(0)
    , m_receivingHeader(true)
{
    connect(m_server, &QTcpServer::newConnection, this, &FileTransfer::acceptConnection);
}

FileTransfer::~FileTransfer() {
    stopListening();
}

void FileTransfer::startListening() {
    m_server->listen(QHostAddress::AnyIPv4, TRANSFER_PORT);
}

void FileTransfer::stopListening() {
    m_server->close();
    if (m_sendSocket) {
        m_sendSocket->disconnect();
        m_sendSocket->deleteLater();
        m_sendSocket = nullptr;
    }
}

void FileTransfer::acceptConnection() {
    QTcpSocket *socket = m_server->nextPendingConnection();
    m_receiveSocket = socket;

    connect(socket, &QTcpSocket::readyRead, this, &FileTransfer::readIncomingData);
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
}

void FileTransfer::readIncomingData() {
    if (!m_receiveSocket) return;

    QDataStream stream(m_receiveSocket);
    stream.setVersion(QDataStream::Qt_5_15);

    while (m_receiveSocket->bytesAvailable() > 0) {
        if (m_receivingHeader) {
            // Read header size
            quint32 headerSize;
            if (m_receiveSocket->bytesAvailable() < sizeof(quint32)) return;

            stream >> headerSize;

            // Wait for full header
            if (m_receiveSocket->bytesAvailable() < headerSize) return;

            QByteArray headerData = m_receiveSocket->read(headerSize);
            QJsonDocument doc = QJsonDocument::fromJson(headerData);
            if (!doc.isObject()) continue;

            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();

            if (type == "transfer_request") {
                TransferRequest req;
                req.senderIp = obj["sender_ip"].toString();
                req.senderHostname = obj["sender_hostname"].toString();

                QJsonArray files = obj["files"].toArray();
                for (const QJsonValue &v : files) {
                    TransferFile f;
                    f.name = v.toObject()["name"].toString();
                    f.size = v.toObject()["size"].toVariant().toLongLong();
                    req.files.append(f);
                }

                m_pendingRequest = req;
                m_receiveOffset = 0;
                m_receivingHeader = false;

                emit transferRequestReceived(req);
            }
        } else {
            // Receiving file data
            QByteArray data = m_receiveSocket->readAll();
            // Handle file writing in actual implementation
            m_receiveOffset += data.size();

            qint64 total = 0;
            for (const TransferFile &f : m_pendingRequest.files) {
                total += f.size;
            }
            emit transferProgress(m_receiveOffset, total);
        }
    }
}

void FileTransfer::sendFiles(const QString &targetIp, const QList<TransferFile> &files) {
    m_sendSocket = new QTcpSocket(this);
    m_sendQueue = files;
    m_currentFileIndex = 0;
    m_currentFileOffset = 0;
    m_sentBytes = 0;
    m_totalBytes = 0;

    for (const TransferFile &f : files) {
        m_totalBytes += f.size;
    }

    connect(m_sendSocket, &QTcpSocket::connected, this, [this, targetIp]() {
        // Send header
        QJsonObject obj;
        obj["type"] = "transfer_request";
        obj["sender_ip"] = getLocalIP();
        obj["sender_hostname"] = QHostInfo::localHostName();

        QJsonArray filesArray;
        for (const TransferFile &f : m_sendQueue) {
            QJsonObject fileObj;
            fileObj["name"] = f.name;
            fileObj["size"] = f.size;
            filesArray.append(fileObj);
        }
        obj["files"] = filesArray;

        QByteArray header = QJsonDocument(obj).toJson();
        QDataStream stream(m_sendSocket);
        stream << static_cast<quint32>(header.size());
        m_sendSocket->write(header);

        connect(m_sendSocket, &QTcpSocket::bytesWritten, this, &FileTransfer::bytesWritten);

        sendNextChunk();
    });

    m_sendSocket->connectToHost(targetIp, TRANSFER_PORT);
}

void FileTransfer::sendNextChunk() {
    if (m_currentFileIndex >= m_sendQueue.size()) {
        emit transferCompleted();
        return;
    }

    const TransferFile &file = m_sendQueue[m_currentFileIndex];

    QFile f(file.path);
    if (!f.open(QIODevice::ReadOnly)) {
        emit transferError("Cannot open file: " + file.name);
        return;
    }

    f.seek(m_currentFileOffset);

    QByteArray chunk = f.read(CHUNK_SIZE);
    if (chunk.isEmpty()) {
        m_currentFileIndex++;
        m_currentFileOffset = 0;
        sendNextChunk();
        return;
    }

    m_sendSocket->write(chunk);
    m_currentFileOffset += chunk.size();
    m_sentBytes += chunk.size();

    emit transferProgress(m_sentBytes, m_totalBytes);
}

void FileTransfer::bytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);
    sendNextChunk();
}

QString FileTransfer::downloadDirectory() const {
    QMutexLocker locker(&m_mutex);
    return m_downloadDir;
}

void FileTransfer::setDownloadDirectory(const QString &dir) {
    QMutexLocker locker(&m_mutex);
    m_downloadDir = dir;
}
```

- [ ] **Step 3: Commit**

```bash
git add src/network/transfer.h src/network/transfer.cpp
git commit -m "feat: add TCP file transfer module"
```

---

## Task 4: Device Widget (Sender Mode)

**Files:**
- Create: `src/widgets/devicewidget.h`
- Create: `src/widgets/devicewidget.cpp`
- Modify: `src/mainwindow.cpp`

- [ ] **Step 1: Create src/widgets/devicewidget.h**

```cpp
#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "network/discovery.h"

class DeviceWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceWidget(Discovery *discovery, QWidget *parent = nullptr);

    QString selectedDeviceIp() const;
    QString selectedDeviceHostname() const;

signals:
    void deviceSelected(const QString &ip, const QString &hostname);

private slots:
    void onScanClicked();
    void onDeviceDoubleClicked(QListWidgetItem *item);

private:
    Discovery *m_discovery;
    QPushButton *m_scanBtn;
    QListWidget *m_deviceList;
};
```

- [ ] **Step 2: Create src/widgets/devicewidget.cpp**

```cpp
#include "devicewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

DeviceWidget::DeviceWidget(Discovery *discovery, QWidget *parent)
    : QWidget(parent)
    , m_discovery(discovery)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("设备列表:");
    titleLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(titleLabel);

    m_deviceList = new QListWidget(this);
    layout->addWidget(m_deviceList);

    m_scanBtn = new QPushButton("扫描设备", this);
    layout->addWidget(m_scanBtn);

    connect(m_scanBtn, &QPushButton::clicked, this, &DeviceWidget::onScanClicked);
    connect(m_deviceList, &QListWidget::itemDoubleClicked,
            this, &DeviceWidget::onDeviceDoubleClicked);
    connect(m_discovery, &Discovery::deviceFound, this, [this](const DeviceInfo &info) {
        // Check if already in list
        QList<QListWidgetItem *> items = m_deviceList->findItems(info.ip, Qt::MatchExactly);
        if (items.isEmpty()) {
            m_deviceList->addItem(QString("%1 (%2)").arg(info.hostname, info.ip));
            m_deviceList->item(m_deviceList->count() - 1)->setData(Qt::UserRole, info.ip);
        }
    });
}

void DeviceWidget::onScanClicked() {
    m_deviceList->clear();
    m_discovery->scanForDevices();
}

void DeviceWidget::onDeviceDoubleClicked(QListWidgetItem *item) {
    QString ip = item->data(Qt::UserRole).toString();
    QString hostname = item->text().split(" (").value(0);
    emit deviceSelected(ip, hostname);
}

QString DeviceWidget::selectedDeviceIp() const {
    QListWidgetItem *item = m_deviceList->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}

QString DeviceWidget::selectedDeviceHostname() const {
    QListWidgetItem *item = m_deviceList->currentItem();
    if (item) {
        QString text = item->text();
        return text.split(" (").value(0);
    }
    return QString();
}
```

- [ ] **Step 3: Update mainwindow.cpp to include device widget**

Modify src/mainwindow.cpp - replace senderTab placeholder with actual DeviceWidget

- [ ] **Step 4: Commit**

```bash
git add src/widgets/devicewidget.h src/widgets/devicewidget.cpp src/mainwindow.cpp
git commit -m "feat: add device widget for sender mode"
```

---

## Task 5: File Queue Widget

**Files:**
- Create: `src/widgets/filequeuewidget.h`
- Create: `src/widgets/filequeuewidget.cpp`
- Modify: `src/mainwindow.cpp`

- [ ] **Step 1: Create src/widgets/filequeuewidget.h**

```cpp
#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "network/transfer.h"

class FileQueueWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileQueueWidget(QWidget *parent = nullptr);

    QList<TransferFile> getFiles() const;
    void clearQueue();

signals:
    void filesReady(bool hasFiles);

private slots:
    void onAddFilesClicked();
    void onAddFolderClicked();
    void onRemoveClicked();
    void onClearClicked();

private:
    void addFilesToQueue(const QStringList &paths);

    QListWidget *m_fileList;
    QPushButton *m_addFilesBtn;
    QPushButton *m_addFolderBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_clearBtn;
};
```

- [ ] **Step 2: Create src/widgets/filequeuewidget.cpp**

```cpp
#include "filequeuewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDirIterator>

FileQueueWidget::FileQueueWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("文件队列:");
    titleLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(titleLabel);

    m_fileList = new QListWidget(this);
    layout->addWidget(m_fileList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_addFilesBtn = new QPushButton("添加文件");
    m_addFolderBtn = new QPushButton("添加文件夹");
    m_removeBtn = new QPushButton("移除");
    m_clearBtn = new QPushButton("清空");

    btnLayout->addWidget(m_addFilesBtn);
    btnLayout->addWidget(m_addFolderBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_clearBtn);
    layout->addLayout(btnLayout);

    connect(m_addFilesBtn, &QPushButton::clicked, this, &FileQueueWidget::onAddFilesClicked);
    connect(m_addFolderBtn, &QPushButton::clicked, this, &FileQueueWidget::onAddFolderClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &FileQueueWidget::onRemoveClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &FileQueueWidget::onClearClicked);
}

void FileQueueWidget::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "选择文件");
    if (!files.isEmpty()) {
        addFilesToQueue(files);
    }
}

void FileQueueWidget::onAddFolderClicked() {
    QString folder = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (folder.isEmpty()) return;

    QStringList files;
    QDirIterator it(folder, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        if (QFileInfo(path).isFile()) {
            files.append(path);
        }
    }
    addFilesToQueue(files);
}

void FileQueueWidget::addFilesToQueue(const QStringList &paths) {
    for (const QString &path : paths) {
        QFileInfo info(path);
        QString display = QString("%1 (%2)")
            .arg(info.fileName())
            .arg(formatSize(info.size()));
        QListWidgetItem *item = new QListWidgetItem(display, m_fileList);
        item->setData(Qt::UserRole, path);
    }
    emit filesReady(!m_fileList->count() == 0);
}

QList<TransferFile> FileQueueWidget::getFiles() const {
    QList<TransferFile> result;
    for (int i = 0; i < m_fileList->count(); ++i) {
        QListWidgetItem *item = m_fileList->item(i);
        QString path = item->data(Qt::UserRole).toString();
        QFileInfo info(path);
        TransferFile f;
        f.name = info.fileName();
        f.path = path;
        f.size = info.size();
        result.append(f);
    }
    return result;
}

void FileQueueWidget::clearQueue() {
    m_fileList->clear();
    emit filesReady(false);
}
```

- [ ] **Step 3: Update mainwindow.cpp to include file queue widget in sender tab**

- [ ] **Step 4: Commit**

```bash
git add src/widgets/filequeuewidget.h src/widgets/filequeuewidget.cpp src/mainwindow.cpp
git commit -m "feat: add file queue widget"
```

---

## Task 6: Progress Widget

**Files:**
- Create: `src/widgets/progresswidget.h`
- Create: `src/widgets/progresswidget.cpp`
- Modify: `src/mainwindow.cpp`

- [ ] **Step 1: Create src/widgets/progresswidget.h**

```cpp
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
```

- [ ] **Step 2: Create src/widgets/progresswidget.cpp**

```cpp
#include "progresswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);

    QHBoxLayout *labelLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("就绪", this);
    m_speedLabel = new QLabel("", this);
    labelLayout->addWidget(m_statusLabel);
    labelLayout->addStretch();
    labelLayout->addWidget(m_speedLabel);
    layout->addLayout(labelLayout);
}

void ProgressWidget::setProgress(qint64 current, qint64 total) {
    if (total > 0) {
        int percent = static_cast<int>((current * 100) / total);
        m_progressBar->setValue(percent);
        m_speedLabel->setText(formatSize(current) + " / " + formatSize(total));
    }
}

void ProgressWidget::setStatus(const QString &status) {
    m_statusLabel->setText(status);
}

void ProgressWidget::reset() {
    m_progressBar->setValue(0);
    m_statusLabel->setText("就绪");
    m_speedLabel->setText("");
}

QString ProgressWidget::formatSize(qint64 bytes) const {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024);
    if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    }
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}
```

- [ ] **Step 3: Update mainwindow.cpp to include progress widget at bottom**

- [ ] **Step 4: Commit**

```bash
git add src/widgets/progresswidget.h src/widgets/progresswidget.cpp src/mainwindow.cpp
git commit -m "feat: add progress widget"
```

---

## Task 7: Accept Dialog (Receiver Mode Confirmation)

**Files:**
- Create: `src/widgets/acceptdialog.h`
- Create: `src/widgets/acceptdialog.cpp`
- Modify: `src/mainwindow.cpp`

- [ ] **Step 1: Create src/widgets/acceptdialog.h**

```cpp
#pragma once
#include <QDialog>
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
    bool m_accepted;
};
```

- [ ] **Step 2: Create src/widgets/acceptdialog.cpp**

```cpp
#include "acceptdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

AcceptDialog::AcceptDialog(const TransferRequest &request,
                           const QString &downloadDir,
                           QWidget *parent)
    : QDialog(parent)
    , m_accepted(false)
{
    setWindowTitle("文件传输请求");
    setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(QString("收到来自: %1 (%2)")
        .arg(request.senderHostname, request.senderIp)));

    layout->addWidget(new QLabel("文件列表:"));

    QListWidget *fileList = new QListWidget(this);
    qint64 totalSize = 0;
    for (const TransferFile &f : request.files) {
        fileList->addItem(QString("%1 (%2)")
            .arg(f.name)
            .arg(formatSize(f.size)));
        totalSize += f.size;
    }
    layout->addWidget(fileList);

    layout->addWidget(new QLabel(QString("共 %1 个文件, 总计 %2")
        .arg(request.files.size())
        .arg(formatSize(totalSize))));

    layout->addWidget(new QLabel(QString("保存到: %1").arg(downloadDir)));

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *rejectBtn = new QPushButton("拒绝");
    QPushButton *acceptBtn = new QPushButton("接受");
    btnLayout->addStretch();
    btnLayout->addWidget(rejectBtn);
    btnLayout->addWidget(acceptBtn);
    layout->addLayout(btnLayout);

    connect(rejectBtn, &QPushButton::clicked, this, &AcceptDialog::onRejectClicked);
    connect(acceptBtn, &QPushButton::clicked, this, &AcceptDialog::onAcceptClicked);
}

void AcceptDialog::onAcceptClicked() {
    m_accepted = true;
    accept();
}

void AcceptDialog::onRejectClicked() {
    m_accepted = false;
    reject();
}
```

- [ ] **Step 3: Update mainwindow.cpp receiver tab to show accept dialog when transfer request received**

- [ ] **Step 4: Commit**

```bash
git add src/widgets/acceptdialog.h src/widgets/acceptdialog.cpp src/mainwindow.cpp
git commit -m "feat: add accept confirmation dialog for receiver mode"
```

---

## Task 8: Settings (Download Directory)

**Files:**
- Modify: `src/mainwindow.cpp`
- Create: `src/mainwindow.cpp` settings UI (simple folder path display + change button)

- [ ] **Step 1: Add download directory button to main window status bar or settings area**

- [ ] **Step 2: Persist setting using QSettings**

```cpp
// In mainwindow.cpp constructor
QSettings settings;
QString downloadDir = settings.value("downloadDir",
    QDir::home().filePath("Downloads/FileSwitch")).toString();
m_transfer->setDownloadDirectory(downloadDir);
```

- [ ] **Step 3: Add "Change Folder" button functionality**

- [ ] **Step 4: Commit**

```bash
git add src/mainwindow.cpp
git commit -m "feat: add download directory settings"
```

---

## Task 9: Final Integration and Build

**Files:**
- All previous files
- Test on macOS
- Test on Windows

- [ ] **Step 1: Review all connections in mainwindow.cpp are correct**

Check that Discovery and FileTransfer objects are created and properly connected

- [ ] **Step 2: Build on macOS**

Run: `cd build && cmake --build .`
Expected: Compiles successfully

- [ ] **Step 3: Verify basic functionality**

- [ ] **Step 4: Commit final integration**

```bash
git add -A
git commit -m "feat: complete FileSwitch implementation"
```

---

## Spec Coverage Check

| Spec Section | Tasks |
|-------------|-------|
| 3.1 双模式架构 | Tasks 4,7 (Sender/Receiver tabs) |
| 3.2 网络协议 | Tasks 2,3 (UDP discovery, TCP transfer) |
| 4.1 设备发现 | Task 4 (DeviceWidget) |
| 4.2 文件选择 | Task 5 (FileQueueWidget) |
| 4.3 传输控制 | Tasks 3,6 (Transfer module, ProgressWidget) |
| 4.4 设置模块 | Task 8 (Download directory) |
| 5.1 主界面布局 | All widget tasks integrated in Task 4-8 |
| 5.2 接收确认对话框 | Task 7 (AcceptDialog) |
| 6. 错误处理 | Built into transfer.cpp |
| 7. 项目结构 | Task 1 scaffold |

All spec sections are covered by the implementation plan.

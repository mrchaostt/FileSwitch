#include "widgets/filequeuewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDirIterator>
#include <QListWidgetItem>

FileQueueWidget::FileQueueWidget(QWidget *parent)
    : QWidget(parent)
    , m_fileList(new QListWidget(this))
    , m_addFilesBtn(new QPushButton("添加文件", this))
    , m_addFolderBtn(new QPushButton("添加文件夹", this))
    , m_removeBtn(new QPushButton("移除", this))
    , m_clearBtn(new QPushButton("清空", this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("文件队列:", this);
    QFont font = titleLabel->font();
    font.setBold(true);
    titleLabel->setFont(font);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_fileList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_addFilesBtn);
    buttonLayout->addWidget(m_addFolderBtn);
    buttonLayout->addWidget(m_removeBtn);
    buttonLayout->addWidget(m_clearBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_addFilesBtn, &QPushButton::clicked, this, &FileQueueWidget::onAddFilesClicked);
    connect(m_addFolderBtn, &QPushButton::clicked, this, &FileQueueWidget::onAddFolderClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &FileQueueWidget::onRemoveClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &FileQueueWidget::onClearClicked);
    connect(m_fileList, &QListWidget::itemSelectionChanged, this, [this]() {
        emit filesReady(m_fileList->count() > 0);
    });
}

QList<TransferFile> FileQueueWidget::getFiles() const
{
    QList<TransferFile> files;
    for (int i = 0; i < m_fileList->count(); ++i) {
        QListWidgetItem *item = m_fileList->item(i);
        TransferFile file = item->data(Qt::UserRole).value<TransferFile>();
        files.append(file);
    }
    return files;
}

void FileQueueWidget::clearQueue()
{
    m_fileList->clear();
    emit filesReady(false);
}

void FileQueueWidget::onAddFilesClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择文件");
    if (!files.isEmpty()) {
        addFilesToQueue(files);
    }
}

void FileQueueWidget::onAddFolderClicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (!folder.isEmpty()) {
        QStringList files;
        QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            files.append(it.next());
        }
        if (!files.isEmpty()) {
            addFilesToQueue(files);
        }
    }
}

void FileQueueWidget::onRemoveClicked()
{
    QListWidgetItem *item = m_fileList->currentItem();
    if (item) {
        delete m_fileList->takeItem(m_fileList->row(item));
        emit filesReady(m_fileList->count() > 0);
    }
}

void FileQueueWidget::onClearClicked()
{
    clearQueue();
}

void FileQueueWidget::addFilesToQueue(const QStringList &paths)
{
    for (const QString &path : paths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists()) {
            TransferFile file;
            file.name = fileInfo.fileName();
            file.path = fileInfo.absoluteFilePath();
            file.size = fileInfo.size();

            QString displayText = QString("%1 (%2)").arg(file.name).arg(formatSize(file.size));
            QListWidgetItem *item = new QListWidgetItem(displayText, m_fileList);
            item->setData(Qt::UserRole, QVariant::fromValue(file));
        }
    }
    emit filesReady(m_fileList->count() > 0);
}

QString FileQueueWidget::formatSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024 * 1024));
    } else {
        return QString("%1 GB").arg(bytes / (1024 * 1024 * 1024));
    }
}
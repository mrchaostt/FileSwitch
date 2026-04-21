#include "filequeuewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDirIterator>
#include <QListWidgetItem>

// Claude Design System Constants
#define COLOR_IVORY QColor(250, 249, 245)
#define COLOR_WARM_SAND QColor(232, 230, 220)
#define COLOR_BORDER_CREAM QColor(240, 238, 230)
#define COLOR_CHARCOAL_WARM QColor(77, 76, 72)
#define COLOR_OLIVE_GRAY QColor(94, 93, 89)
#define COLOR_STONE_GRAY QColor(135, 134, 127)
#define COLOR_RING_WARM QColor(209, 207, 197)
#define COLOR_NEAR_BLACK QColor(20, 20, 19)
#define COLOR_TERRACOTTA QColor(201, 100, 66)
#define COLOR_ERROR_CRIMSON QColor(181, 51, 51)
#define FONT_SERIF "Georgia, 'Times New Roman', serif"
#define FONT_SANS "system-ui, -apple-system, Arial, sans-serif"

FileQueueWidget::FileQueueWidget(QWidget *parent)
    : QWidget(parent)
    , m_fileList(new QListWidget(this))
    , m_addFilesBtn(new QPushButton("添加文件", this))
    , m_addFolderBtn(new QPushButton("添加文件夹", this))
    , m_removeBtn(new QPushButton("移除", this))
    , m_clearBtn(new QPushButton("清空", this))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel("文件队列");
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        " font-family: %1;"
        " font-size: 12px;"
        " font-weight: 500;"
        " color: %2;"
        " letter-spacing: 0.12px;"
        " text-transform: uppercase;"
        "}"
    ).arg(FONT_SANS).arg(COLOR_STONE_GRAY.name()));
    mainLayout->addWidget(titleLabel);

    m_fileList->setStyleSheet(QString(
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
        "QListWidget::item:selected {"
        " background-color: %5;"
        " color: %6;"
        "}"
        "QListWidget::item:hover:!selected {"
        " background-color: %7;"
        "}"
    ).arg(COLOR_IVORY.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_NEAR_BLACK.name())
     .arg(COLOR_TERRACOTTA.name())
     .arg(COLOR_IVORY.name())
     .arg(COLOR_WARM_SAND.name()));
    m_fileList->setMinimumHeight(60);
    mainLayout->addWidget(m_fileList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    QString btnStyle = QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: 1px solid %3;"
        " border-radius: 6px;"
        " font-family: %4;"
        " font-size: 12px;"
        " font-weight: 500;"
        " padding: 6px 12px;"
        "}"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(COLOR_WARM_SAND.name())
     .arg(COLOR_CHARCOAL_WARM.name())
     .arg(COLOR_BORDER_CREAM.name())
     .arg(FONT_SANS)
     .arg(COLOR_RING_WARM.name())
     .arg(COLOR_BORDER_CREAM.name());

    m_addFilesBtn->setStyleSheet(btnStyle);
    m_addFolderBtn->setStyleSheet(btnStyle);
    m_removeBtn->setStyleSheet(btnStyle);

    m_clearBtn->setStyleSheet(QString(
        "QPushButton {"
        " background-color: %1;"
        " color: %2;"
        " border: 1px solid %3;"
        " border-radius: 6px;"
        " font-family: %4;"
        " font-size: 12px;"
        " font-weight: 500;"
        " padding: 6px 12px;"
        "}"
        "QPushButton:hover { background-color: #cf3d3d; }"
        "QPushButton:pressed { background-color: #a32d2d; }"
    ).arg(COLOR_ERROR_CRIMSON.name())
     .arg(COLOR_IVORY.name())
     .arg(COLOR_ERROR_CRIMSON.name())
     .arg(FONT_SANS));

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
        emit filesReady();
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
    emit filesReady();
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
        emit filesReady();
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

            QString displayText = QString("%1  •  %2").arg(file.name).arg(formatSize(file.size));
            QListWidgetItem *item = new QListWidgetItem(displayText, m_fileList);
            item->setData(Qt::UserRole, QVariant::fromValue(file));
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }
    emit filesReady();
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
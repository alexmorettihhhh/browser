#include "downloadmanager.h"
#include <QNetworkAccessManager>
#include <QStandardItem>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
    , m_downloadPath(QDir::homePath() + "/Downloads")
    , m_model(new QStandardItemModel(this))
{
    m_model->setHorizontalHeaderLabels({"Файл", "Размер", "Прогресс", "Скорость", "Статус"});
}

DownloadManager::~DownloadManager()
{
    for (auto item : m_downloads) {
        if (item->reply) {
            item->reply->abort();
            item->reply->deleteLater();
        }
        if (item->file) {
            item->file->close();
            delete item->file;
        }
        delete item;
    }
}

void DownloadManager::download(const QUrl &url)
{
    QString fileName = QFileInfo(url.path()).fileName();
    if (fileName.isEmpty()) {
        fileName = "download";
    }

    QString filePath = m_downloadPath + "/" + fileName;
    QFile* file = new QFile(filePath);

    // Проверяем, существует ли файл
    if (file->exists()) {
        int i = 1;
        QFileInfo fi(filePath);
        while (file->exists()) {
            fileName = fi.baseName() + QString("_%1.").arg(i) + fi.completeSuffix();
            filePath = m_downloadPath + "/" + fileName;
            delete file;
            file = new QFile(filePath);
            i++;
        }
    }

    if (!file->open(QIODevice::WriteOnly)) {
        emit downloadError(fileName, "Не удалось создать файл");
        delete file;
        return;
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);

    DownloadItem* item = new DownloadItem;
    item->fileName = fileName;
    item->url = url.toString();
    item->size = 0;
    item->received = 0;
    item->startTime = QDateTime::currentDateTime();
    item->reply = reply;
    item->file = file;
    item->completed = false;

    m_downloads.append(item);
    updateModel(item);

    connect(reply, &QNetworkReply::downloadProgress,
            this, &DownloadManager::handleDownloadProgress);
    connect(reply, &QNetworkReply::finished,
            this, &DownloadManager::handleDownloadFinished);
    connect(reply, &QNetworkReply::errorOccurred,
            this, &DownloadManager::handleDownloadError);
}

void DownloadManager::handleDownloadProgress(qint64 received, qint64 total)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    for (auto item : m_downloads) {
        if (item->reply == reply) {
            item->received = received;
            item->size = total;
            updateModel(item);
            emit downloadProgress(item->fileName, received, total);
            break;
        }
    }
}

void DownloadManager::handleDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    for (auto item : m_downloads) {
        if (item->reply == reply) {
            if (reply->error() == QNetworkReply::NoError) {
                item->file->write(reply->readAll());
                item->file->close();
                item->completed = true;
                updateModel(item);
                emit downloadCompleted(item->fileName);
            }
            reply->deleteLater();
            break;
        }
    }
}

void DownloadManager::handleDownloadError(QNetworkReply::NetworkError error)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    for (auto item : m_downloads) {
        if (item->reply == reply) {
            emit downloadError(item->fileName, reply->errorString());
            updateModel(item);
            break;
        }
    }
}

void DownloadManager::updateModel(DownloadItem* item)
{
    int row = m_downloads.indexOf(item);
    if (row == -1) {
        row = m_model->rowCount();
        m_model->insertRow(row);
    }

    m_model->setItem(row, 0, new QStandardItem(item->fileName));
    m_model->setItem(row, 1, new QStandardItem(formatSize(item->size)));

    QString progress;
    if (item->size > 0) {
        int percent = (item->received * 100) / item->size;
        progress = QString("%1%").arg(percent);
    } else {
        progress = formatSize(item->received);
    }
    m_model->setItem(row, 2, new QStandardItem(progress));

    qint64 elapsed = item->startTime.msecsTo(QDateTime::currentDateTime());
    QString speed = formatSpeed(item->received, elapsed);
    m_model->setItem(row, 3, new QStandardItem(speed));

    QString status = item->completed ? "Завершено" : "Загрузка...";
    m_model->setItem(row, 4, new QStandardItem(status));
}

QString DownloadManager::formatSize(qint64 size)
{
    if (size <= 0) return "0 B";
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int digitGroup = qFloor(log(size) / log(1024));
    double num = size / pow(1024, digitGroup);
    return QString("%1 %2").arg(num, 0, 'f', 2).arg(units[digitGroup]);
}

QString DownloadManager::formatSpeed(qint64 bytes, qint64 msecs)
{
    if (msecs <= 0) return "0 B/s";
    double speed = (bytes * 1000.0) / msecs;
    return formatSize(qint64(speed)) + "/s";
} 
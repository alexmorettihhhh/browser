#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDateTime>
#include <QStandardItemModel>

struct DownloadItem {
    QString fileName;
    QString url;
    qint64 size;
    qint64 received;
    QDateTime startTime;
    QNetworkReply* reply;
    QFile* file;
    bool completed;
};

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager();

    void download(const QUrl &url);
    QStandardItemModel* model() { return m_model; }
    void setDownloadPath(const QString &path) { m_downloadPath = path; }

signals:
    void downloadProgress(const QString &fileName, qint64 received, qint64 total);
    void downloadCompleted(const QString &fileName);
    void downloadError(const QString &fileName, const QString &error);

private slots:
    void handleDownloadProgress(qint64 received, qint64 total);
    void handleDownloadFinished();
    void handleDownloadError(QNetworkReply::NetworkError error);

private:
    QString m_downloadPath;
    QList<DownloadItem*> m_downloads;
    QStandardItemModel* m_model;

    void updateModel(DownloadItem* item);
    QString formatSize(qint64 size);
    QString formatSpeed(qint64 bytes, qint64 msecs);
}; 
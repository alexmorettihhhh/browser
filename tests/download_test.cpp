#include <QtTest>
#include "downloadmanager.h"
#include <QNetworkReply>
#include <QTemporaryFile>

class DownloadTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStartDownload();
    void testPauseResumeDownload();
    void testCancelDownload();
    void testMultipleDownloads();
    void testDownloadProgress();
    void testDownloadSettings();
    void testDownloadHistory();
    void testFileOperations();
    void cleanupTestCase();

private:
    DownloadManager *downloads;
    const QString testUrl = "https://example.com/test.zip";
    const QString testPath = "test.zip";
};

void DownloadTest::initTestCase()
{
    downloads = new DownloadManager(this);
}

void DownloadTest::testStartDownload()
{
    // Тестируем начало загрузки
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    QVERIFY(downloadId > 0);
    
    // Проверяем статус загрузки
    auto status = downloads->getDownloadStatus(downloadId);
    QVERIFY(status == "downloading" || status == "completed");
    
    // Проверяем информацию о загрузке
    auto info = downloads->getDownloadInfo(downloadId);
    QCOMPARE(info.url, testUrl);
    QCOMPARE(info.path, testPath);
}

void DownloadTest::testPauseResumeDownload()
{
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    
    // Тестируем паузу
    QVERIFY(downloads->pauseDownload(downloadId));
    QCOMPARE(downloads->getDownloadStatus(downloadId), "paused");
    
    // Тестируем возобновление
    QVERIFY(downloads->resumeDownload(downloadId));
    auto status = downloads->getDownloadStatus(downloadId);
    QVERIFY(status == "downloading" || status == "completed");
}

void DownloadTest::testCancelDownload()
{
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    
    // Тестируем отмену
    QVERIFY(downloads->cancelDownload(downloadId));
    QCOMPARE(downloads->getDownloadStatus(downloadId), "cancelled");
    
    // Проверяем, что файл удален
    QVERIFY(!QFile::exists(testPath));
}

void DownloadTest::testMultipleDownloads()
{
    QList<qint64> downloadIds;
    
    // Запускаем несколько загрузок
    for(int i = 0; i < 3; i++) {
        QString url = QString("https://example.com/test%1.zip").arg(i);
        QString path = QString("test%1.zip").arg(i);
        qint64 id = downloads->startDownload(url, path);
        QVERIFY(id > 0);
        downloadIds.append(id);
    }
    
    // Проверяем активные загрузки
    auto activeDownloads = downloads->getActiveDownloads();
    QVERIFY(activeDownloads.size() <= 3);
    
    // Отменяем все загрузки
    for(auto id : downloadIds) {
        downloads->cancelDownload(id);
    }
}

void DownloadTest::testDownloadProgress()
{
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    
    // Проверяем прогресс
    int progress = downloads->getDownloadProgress(downloadId);
    QVERIFY(progress >= 0 && progress <= 100);
    
    // Проверяем скорость
    qint64 speed = downloads->getDownloadSpeed(downloadId);
    QVERIFY(speed >= 0);
    
    // Проверяем оставшееся время
    int timeLeft = downloads->getDownloadTimeLeft(downloadId);
    QVERIFY(timeLeft >= 0);
}

void DownloadTest::testDownloadSettings()
{
    // Тестируем настройки папки загрузки
    QString downloadPath = QDir::tempPath();
    downloads->setDownloadPath(downloadPath);
    QCOMPARE(downloads->getDownloadPath(), downloadPath);
    
    // Тестируем настройки параллельных загрузок
    downloads->setMaxParallelDownloads(3);
    QCOMPARE(downloads->getMaxParallelDownloads(), 3);
    
    // Тестируем настройки автоматического запуска
    downloads->setAutoStartDownloads(false);
    QVERIFY(!downloads->getAutoStartDownloads());
}

void DownloadTest::testDownloadHistory()
{
    // Очищаем историю
    downloads->clearDownloadHistory();
    QVERIFY(downloads->getDownloadHistory().isEmpty());
    
    // Добавляем загрузку
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    
    // Проверяем историю
    auto history = downloads->getDownloadHistory();
    QVERIFY(!history.isEmpty());
    
    // Удаляем запись из истории
    QVERIFY(downloads->removeFromHistory(downloadId));
    QVERIFY(!downloads->getDownloadHistory().contains(downloadId));
}

void DownloadTest::testFileOperations()
{
    qint64 downloadId = downloads->startDownload(testUrl, testPath);
    
    // Тестируем открытие файла
    QVERIFY(downloads->openDownloadedFile(downloadId) || 
            downloads->getDownloadStatus(downloadId) != "completed");
    
    // Тестируем открытие папки с файлом
    QVERIFY(downloads->openDownloadFolder(downloadId));
    
    // Тестируем переименование файла
    QString newPath = "test_renamed.zip";
    QVERIFY(downloads->renameDownloadedFile(downloadId, newPath) ||
            downloads->getDownloadStatus(downloadId) != "completed");
    
    // Тестируем удаление файла
    QVERIFY(downloads->removeDownloadedFile(downloadId));
}

void DownloadTest::cleanupTestCase()
{
    // Отменяем все активные загрузки
    for(auto id : downloads->getActiveDownloads()) {
        downloads->cancelDownload(id);
    }
    
    // Очищаем историю
    downloads->clearDownloadHistory();
    
    delete downloads;
    
    // Удаляем тестовые файлы
    QFile::remove(testPath);
    QFile::remove("test_renamed.zip");
    for(int i = 0; i < 3; i++) {
        QFile::remove(QString("test%1.zip").arg(i));
    }
}

QTEST_MAIN(DownloadTest)
#include "download_test.moc" 
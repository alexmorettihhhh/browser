#include <QtTest>
#include "syncmanager.h"
#include <QNetworkAccessManager>
#include <QJsonDocument>

class SyncTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testAuthentication();
    void testSyncOperations();
    void testDataManagement();
    void testConflictResolution();
    void testOfflineMode();
    void testDeviceManagement();
    void testEncryption();
    void testStatistics();
    void testErrorHandling();
    void cleanupTestCase();

private:
    SyncManager *sync;
    const QString testEmail = "test@example.com";
    const QString testPassword = "testPassword123";
    const QString testDeviceId = "test-device-001";
};

void SyncTest::initTestCase()
{
    sync = new SyncManager(this);
}

void SyncTest::testAuthentication()
{
    // Тестируем вход
    QVERIFY(sync->signIn(testEmail, testPassword));
    QVERIFY(sync->isSignedIn());
    
    // Тестируем получение токена
    QString token = sync->getAuthToken();
    QVERIFY(!token.isEmpty());
    
    // Тестируем выход
    sync->signOut();
    QVERIFY(!sync->isSignedIn());
}

void SyncTest::testSyncOperations()
{
    sync->signIn(testEmail, testPassword);
    
    // Тестируем запуск синхронизации
    QVERIFY(sync->startSync());
    QVERIFY(sync->isSyncing());
    
    // Тестируем остановку синхронизации
    sync->stopSync();
    QVERIFY(!sync->isSyncing());
    
    // Тестируем принудительную синхронизацию
    QVERIFY(sync->forceSync());
    
    // Тестируем статус синхронизации
    auto status = sync->getSyncStatus();
    QVERIFY(status.contains("lastSync"));
    QVERIFY(status.contains("itemCount"));
}

void SyncTest::testDataManagement()
{
    // Тестируем добавление элемента
    QJsonObject data;
    data["url"] = "https://example.com";
    data["title"] = "Test Bookmark";
    
    qint64 itemId = sync->addItem(SyncItemType::Bookmark, data);
    QVERIFY(itemId > 0);
    
    // Тестируем обновление элемента
    data["title"] = "Updated Title";
    QVERIFY(sync->updateItem(itemId, data));
    
    // Тестируем получение элемента
    auto item = sync->getItem(itemId);
    QCOMPARE(item.data["title"].toString(), "Updated Title");
    
    // Тестируем удаление элемента
    QVERIFY(sync->removeItem(itemId));
}

void SyncTest::testConflictResolution()
{
    // Создаем конфликтующие элементы
    QJsonObject localData;
    localData["title"] = "Local Title";
    
    QJsonObject remoteData;
    remoteData["title"] = "Remote Title";
    
    // Тестируем разрешение конфликтов
    auto resolution = sync->resolveConflict(localData, remoteData);
    QVERIFY(!resolution.isEmpty());
    
    // Тестируем стратегии разрешения конфликтов
    sync->setConflictResolutionStrategy("remote-wins");
    QCOMPARE(sync->getConflictResolutionStrategy(), "remote-wins");
}

void SyncTest::testOfflineMode()
{
    // Тестируем переход в оффлайн режим
    sync->setOfflineMode(true);
    QVERIFY(sync->isOfflineMode());
    
    // Тестируем очередь изменений
    QJsonObject data;
    data["title"] = "Offline Item";
    qint64 itemId = sync->addItem(SyncItemType::Bookmark, data);
    
    auto pendingChanges = sync->getPendingChanges();
    QVERIFY(!pendingChanges.isEmpty());
    
    // Тестируем синхронизацию после выхода из оффлайн режима
    sync->setOfflineMode(false);
    QVERIFY(!sync->isOfflineMode());
}

void SyncTest::testDeviceManagement()
{
    // Тестируем регистрацию устройства
    QVERIFY(sync->registerDevice(testDeviceId));
    
    // Тестируем получение списка устройств
    auto devices = sync->getRegisteredDevices();
    QVERIFY(devices.contains(testDeviceId));
    
    // Тестируем удаление устройства
    QVERIFY(sync->unregisterDevice(testDeviceId));
}

void SyncTest::testEncryption()
{
    // Тестируем включение шифрования
    QVERIFY(sync->enableEncryption("encryption-password"));
    QVERIFY(sync->isEncryptionEnabled());
    
    // Тестируем шифрование данных
    QJsonObject data;
    data["sensitive"] = "secret";
    auto encrypted = sync->encryptData(data);
    QVERIFY(!encrypted.isEmpty());
    
    // Тестируем расшифровку данных
    auto decrypted = sync->decryptData(encrypted);
    QCOMPARE(decrypted["sensitive"].toString(), "secret");
}

void SyncTest::testStatistics()
{
    // Тестируем сбор статистики
    auto stats = sync->getSyncStatistics();
    QVERIFY(stats.contains("totalSyncs"));
    QVERIFY(stats.contains("totalItems"));
    QVERIFY(stats.contains("lastSyncDuration"));
    
    // Тестируем сброс статистики
    sync->resetStatistics();
    stats = sync->getSyncStatistics();
    QCOMPARE(stats["totalSyncs"].toInt(), 0);
}

void SyncTest::testErrorHandling()
{
    // Тестируем обработку ошибок сети
    sync->simulateNetworkError();
    QVERIFY(!sync->startSync());
    QVERIFY(!sync->getLastError().isEmpty());
    
    // Тестируем обработку ошибок аутентификации
    sync->signOut();
    QVERIFY(!sync->startSync());
    QVERIFY(sync->getLastError().contains("authentication"));
    
    // Тестируем обработку ошибок данных
    QJsonObject invalidData;
    QVERIFY(!sync->addItem(SyncItemType::Bookmark, invalidData));
    QVERIFY(sync->getLastError().contains("invalid data"));
}

void SyncTest::cleanupTestCase()
{
    sync->signOut();
    delete sync;
}

QTEST_MAIN(SyncTest)
#include "sync_test.moc" 
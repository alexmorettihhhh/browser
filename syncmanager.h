#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include <QQueue>
#include <QMutex>

enum class SyncItemType {
    Bookmark,
    History,
    Password,
    Setting,
    Extension,
    Custom
};

struct SyncItem {
    QString id;
    SyncItemType type;
    QDateTime timestamp;
    QJsonDocument data;
    bool deleted;
    QString deviceId;
    QString version;
    QHash<QString, QVariant> metadata;
};

class SyncManager : public QObject
{
    Q_OBJECT

public:
    explicit SyncManager(QObject *parent = nullptr);
    ~SyncManager();

    // Управление аккаунтом
    bool signIn(const QString &username, const QString &password);
    bool signOut();
    bool isSignedIn() const;
    QString getCurrentUser() const;
    
    // Основные операции синхронизации
    bool startSync();
    bool stopSync();
    bool forceSyncNow();
    bool isSyncing() const;
    
    // Настройка синхронизации
    void setSyncInterval(int minutes);
    void enableAutoSync(bool enable);
    void setSyncTypes(const QList<SyncItemType> &types);
    void setMaxSyncSize(qint64 size);
    
    // Управление данными
    bool addSyncItem(const SyncItem &item);
    bool removeSyncItem(const QString &id);
    bool updateSyncItem(const SyncItem &item);
    SyncItem getSyncItem(const QString &id) const;
    
    // Конфликты и версионность
    QList<SyncItem> getConflicts() const;
    bool resolveConflict(const QString &id, const SyncItem &resolution);
    QList<SyncItem> getItemHistory(const QString &id) const;
    bool revertToVersion(const QString &id, const QString &version);
    
    // Статистика и мониторинг
    QDateTime getLastSyncTime() const;
    int getPendingChangesCount() const;
    qint64 getTotalSyncSize() const;
    QHash<SyncItemType, int> getSyncStats() const;
    
    // Экспорт/Импорт
    bool exportSyncData(const QString &filename) const;
    bool importSyncData(const QString &filename);
    bool mergeSyncData(const QString &filename);

signals:
    void syncStarted();
    void syncCompleted(bool success);
    void syncProgress(int percent);
    void syncError(const QString &error);
    void itemAdded(const SyncItem &item);
    void itemRemoved(const QString &id);
    void itemUpdated(const SyncItem &item);
    void conflictDetected(const SyncItem &local, const SyncItem &remote);
    void authenticationRequired();
    void quotaExceeded(qint64 current, qint64 limit);
    void connectionError(const QString &error);

private slots:
    void performSync();
    void handleNetworkReply(QNetworkReply *reply);
    void checkConnectivity();
    void retryFailedOperations();
    void processQueue();

private:
    bool initializeSync();
    bool validateToken();
    QString generateDeviceId() const;
    bool uploadChanges();
    bool downloadChanges();
    void mergeChanges(const QList<SyncItem> &remoteChanges);
    bool resolveConflicts(const QList<SyncItem> &conflicts);
    void updateLocalDatabase(const SyncItem &item);
    void addToQueue(const SyncItem &item);
    void saveState();
    void loadState();
    QString calculateChecksum(const SyncItem &item) const;
    
    QNetworkAccessManager *m_network;
    QString m_authToken;
    QString m_deviceId;
    QDateTime m_lastSync;
    bool m_autoSyncEnabled;
    int m_syncInterval;
    QList<SyncItemType> m_syncTypes;
    qint64 m_maxSyncSize;
    QQueue<SyncItem> m_syncQueue;
    QHash<QString, SyncItem> m_pendingChanges;
    QMutex m_syncMutex;
    QTimer *m_syncTimer;
    bool m_isSyncing;
    
    static const QString API_ENDPOINT;
    static const int MAX_RETRY_ATTEMPTS = 3;
    static const int DEFAULT_SYNC_INTERVAL = 30; // minutes
};

#endif // SYNCMANAGER_H 
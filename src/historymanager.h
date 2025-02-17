#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QVariant>

struct HistoryItem {
    qint64 id;
    QString url;
    QString title;
    QDateTime timestamp;
    int visitCount;
    QString favicon;
    QString description;
    QHash<QString, QVariant> metadata;
    bool isBookmarked;
    bool isPrivate;
};

class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(QObject *parent = nullptr);
    ~HistoryManager();

    // Основные операции с историей
    bool addVisit(const QString &url, const QString &title, bool isPrivate = false);
    bool removeVisit(qint64 id);
    bool removeVisits(const QDateTime &begin, const QDateTime &end);
    bool clearHistory();
    
    // Поиск и получение истории
    QList<HistoryItem> getVisits(const QDateTime &begin, const QDateTime &end) const;
    QList<HistoryItem> searchHistory(const QString &query) const;
    QList<HistoryItem> getMostVisited(int limit = 10) const;
    QList<HistoryItem> getRecentlyVisited(int limit = 10) const;
    
    // Синхронизация
    bool exportHistory(const QString &filename) const;
    bool importHistory(const QString &filename);
    bool syncWithCloud(const QString &account);
    
    // Метаданные и статистика
    bool addMetadata(qint64 id, const QString &key, const QVariant &value);
    QVariant getMetadata(qint64 id, const QString &key) const;
    QHash<QString, int> getDomainStatistics() const;
    QHash<QDateTime, int> getTimeStatistics() const;
    
    // Управление и настройки
    void setRetentionPeriod(int days);
    void setMaxEntries(int count);
    void enablePrivateBrowsingHistory(bool enable);
    void setAutoCleanup(bool enable);
    
    // Анализ и рекомендации
    QList<QString> getSuggestedSearches(const QString &input) const;
    QList<HistoryItem> getRelatedPages(const QString &url) const;
    QList<QString> getFrequentlyUsedTerms() const;
    
signals:
    void visitAdded(const HistoryItem &item);
    void visitRemoved(qint64 id);
    void historyCleared();
    void syncCompleted(bool success);
    void retentionPeriodChanged(int days);
    void maxEntriesChanged(int count);
    void autoCleanupStatusChanged(bool enabled);
    void metadataChanged(qint64 id, const QString &key);

private slots:
    void performAutoCleanup();
    void handleDatabaseError(const QString &error);
    void syncWithCloudImpl();

private:
    bool initDatabase();
    bool upgradeDatabase(int oldVersion, int newVersion);
    void cleanupOldEntries();
    void optimizeDatabase();
    void backupDatabase();
    void indexSearchTerms();
    QString normalizeUrl(const QString &url) const;
    bool isUrlValid(const QString &url) const;
    void updateVisitCount(const QString &url);
    void mergeHistoryItems(const QList<HistoryItem> &items);
    
    QSqlDatabase m_db;
    int m_retentionDays;
    int m_maxEntries;
    bool m_privateHistoryEnabled;
    bool m_autoCleanupEnabled;
    QDateTime m_lastCleanup;
    QDateTime m_lastSync;
    QHash<QString, int> m_visitCache;
    
    static const int DATABASE_VERSION = 2;
    static const QString DATABASE_NAME;
    static const int DEFAULT_RETENTION_DAYS = 90;
    static const int DEFAULT_MAX_ENTRIES = 10000;
};

#endif // HISTORYMANAGER_H 
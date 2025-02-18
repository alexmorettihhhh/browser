#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <QIcon>
#include <QJsonDocument>
#include <QSqlDatabase>

struct BookmarkItem {
    qint64 id;
    QString url;
    QString title;
    QString description;
    QIcon favicon;
    QDateTime added;
    QDateTime lastVisited;
    int visitCount;
    QStringList tags;
    QString folder;
    int position;
    bool isFolder;
    QList<qint64> children;
    QHash<QString, QVariant> metadata;
};

class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkManager(QObject *parent = nullptr);
    ~BookmarkManager();

    // Основные операции с закладками
    qint64 addBookmark(const QString &url, const QString &title, 
                      const QString &folder = "root");
    bool removeBookmark(qint64 id);
    bool updateBookmark(qint64 id, const QHash<QString, QVariant> &properties);
    BookmarkItem getBookmark(qint64 id) const;
    
    // Работа с папками
    qint64 createFolder(const QString &name, const QString &parentFolder = "root");
    bool removeFolder(qint64 id, bool removeContents = false);
    bool moveBookmark(qint64 id, const QString &newFolder);
    bool reorderBookmarks(const QString &folder, const QList<qint64> &newOrder);
    
    // Поиск и фильтрация
    QList<BookmarkItem> searchBookmarks(const QString &query) const;
    QList<BookmarkItem> getBookmarksByTag(const QString &tag) const;
    QList<BookmarkItem> getBookmarksByFolder(const QString &folder) const;
    QList<BookmarkItem> getRecentBookmarks(int limit = 10) const;
    QList<BookmarkItem> getMostVisited(int limit = 10) const;
    
    // Теги и метаданные
    bool addTag(qint64 id, const QString &tag);
    bool removeTag(qint64 id, const QString &tag);
    QStringList getAllTags() const;
    bool setMetadata(qint64 id, const QString &key, const QVariant &value);
    QVariant getMetadata(qint64 id, const QString &key) const;
    
    // Импорт/Экспорт
    bool exportBookmarks(const QString &filename, const QString &format = "json") const;
    bool importBookmarks(const QString &filename, const QString &format = "json");
    bool importFromBrowser(const QString &browserName);
    
    // Синхронизация
    bool syncWithCloud(const QString &account);
    QDateTime getLastSyncTime() const;
    bool enableAutoSync(bool enable);
    
    // Анализ и рекомендации
    QList<BookmarkItem> getSimilarBookmarks(qint64 id) const;
    QList<QString> getSuggestedTags(qint64 id) const;
    QStringList getPopularDomains() const;
    
    // Управление
    void vacuum();
    void backup();
    bool restore(const QString &backupFile);
    bool validateUrls(bool fix = false);

signals:
    void bookmarkAdded(const BookmarkItem &item);
    void bookmarkRemoved(qint64 id);
    void bookmarkUpdated(const BookmarkItem &item);
    void folderCreated(qint64 id, const QString &name);
    void folderRemoved(qint64 id);
    void bookmarkMoved(qint64 id, const QString &newFolder);
    void bookmarksReordered(const QString &folder);
    void tagsChanged(qint64 id);
    void metadataChanged(qint64 id, const QString &key);
    void syncCompleted(bool success);
    void importCompleted(int count);
    void exportCompleted(int count);
    void databaseError(const QString &error);

private slots:
    void handleDatabaseError(const QString &error);
    void performAutoSync();
    void checkUrls();

private:
    bool initDatabase();
    bool upgradeDatabase(int oldVersion, int newVersion);
    void createDefaultFolders();
    QString normalizeUrl(const QString &url) const;
    bool isUrlValid(const QString &url) const;
    void updateVisitCount(qint64 id);
    QJsonDocument bookmarksToJson() const;
    bool jsonToBookmarks(const QJsonDocument &doc);
    void indexBookmarkContent();
    void cleanupOrphanedRecords();
    
    QSqlDatabase m_db;
    QDateTime m_lastSync;
    bool m_autoSyncEnabled;
    QHash<QString, qint64> m_folderCache;
    QHash<QString, QIcon> m_faviconCache;
    
    static const int DATABASE_VERSION = 2;
    static const QString DATABASE_NAME;
    static const QStringList DEFAULT_FOLDERS;
};

#endif // BOOKMARKMANAGER_H 
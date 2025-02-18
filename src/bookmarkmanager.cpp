#include "bookmarkmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

const QString BookmarkManager::BOOKMARKS_FILE = "bookmarks.json";

BookmarkManager::BookmarkManager(QObject *parent)
    : QObject(parent)
{
    loadBookmarks();
}

BookmarkManager::~BookmarkManager()
{
    saveBookmarks();
}

void BookmarkManager::loadBookmarks()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + BOOKMARKS_FILE);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загрузка папок
            QJsonArray folders = root["folders"].toArray();
            for (const QJsonValue &val : folders) {
                QJsonObject folderObj = val.toObject();
                BookmarkFolder folder;
                folder.id = folderObj["id"].toString();
                folder.name = folderObj["name"].toString();
                folder.parentId = folderObj["parentId"].toString();
                m_folders[folder.id] = folder;
            }
            
            // Загрузка закладок
            QJsonArray bookmarks = root["bookmarks"].toArray();
            for (const QJsonValue &val : bookmarks) {
                QJsonObject bookmarkObj = val.toObject();
                Bookmark bookmark;
                bookmark.id = bookmarkObj["id"].toString();
                bookmark.url = bookmarkObj["url"].toString();
                bookmark.title = bookmarkObj["title"].toString();
                bookmark.folderId = bookmarkObj["folderId"].toString();
                bookmark.icon = bookmarkObj["icon"].toString();
                bookmark.addedDate = QDateTime::fromString(
                    bookmarkObj["addedDate"].toString(), Qt::ISODate);
                bookmark.lastVisited = QDateTime::fromString(
                    bookmarkObj["lastVisited"].toString(), Qt::ISODate);
                
                m_bookmarks[bookmark.id] = bookmark;
            }
        }
    }
}

void BookmarkManager::saveBookmarks()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + BOOKMARKS_FILE);
    
    QJsonObject root;
    
    // Сохранение папок
    QJsonArray folders;
    for (const BookmarkFolder &folder : m_folders) {
        QJsonObject folderObj;
        folderObj["id"] = folder.id;
        folderObj["name"] = folder.name;
        folderObj["parentId"] = folder.parentId;
        folders.append(folderObj);
    }
    root["folders"] = folders;
    
    // Сохранение закладок
    QJsonArray bookmarks;
    for (const Bookmark &bookmark : m_bookmarks) {
        QJsonObject bookmarkObj;
        bookmarkObj["id"] = bookmark.id;
        bookmarkObj["url"] = bookmark.url;
        bookmarkObj["title"] = bookmark.title;
        bookmarkObj["folderId"] = bookmark.folderId;
        bookmarkObj["icon"] = bookmark.icon;
        bookmarkObj["addedDate"] = bookmark.addedDate.toString(Qt::ISODate);
        bookmarkObj["lastVisited"] = bookmark.lastVisited.toString(Qt::ISODate);
        bookmarks.append(bookmarkObj);
    }
    root["bookmarks"] = bookmarks;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

QString BookmarkManager::addBookmark(const QString &url, const QString &title,
                                   const QString &folderId)
{
    Bookmark bookmark;
    bookmark.id = QUuid::createUuid().toString();
    bookmark.url = url;
    bookmark.title = title;
    bookmark.folderId = folderId;
    bookmark.addedDate = QDateTime::currentDateTime();
    bookmark.lastVisited = bookmark.addedDate;
    
    m_bookmarks[bookmark.id] = bookmark;
    emit bookmarkAdded(bookmark);
    saveBookmarks();
    
    return bookmark.id;
}

bool BookmarkManager::removeBookmark(const QString &id)
{
    if (m_bookmarks.remove(id) > 0) {
        emit bookmarkRemoved(id);
        saveBookmarks();
        return true;
    }
    return false;
}

bool BookmarkManager::updateBookmark(const QString &id, const QString &url,
                                   const QString &title)
{
    if (m_bookmarks.contains(id)) {
        Bookmark &bookmark = m_bookmarks[id];
        bookmark.url = url;
        bookmark.title = title;
        bookmark.lastVisited = QDateTime::currentDateTime();
        
        emit bookmarkUpdated(bookmark);
        saveBookmarks();
        return true;
    }
    return false;
}

QString BookmarkManager::createFolder(const QString &name, const QString &parentId)
{
    BookmarkFolder folder;
    folder.id = QUuid::createUuid().toString();
    folder.name = name;
    folder.parentId = parentId;
    
    m_folders[folder.id] = folder;
    emit folderCreated(folder);
    saveBookmarks();
    
    return folder.id;
}

bool BookmarkManager::removeFolder(const QString &id)
{
    if (!m_folders.contains(id)) {
        return false;
    }
    
    // Удаляем все закладки в папке
    QList<QString> bookmarksToRemove;
    for (const Bookmark &bookmark : m_bookmarks) {
        if (bookmark.folderId == id) {
            bookmarksToRemove.append(bookmark.id);
        }
    }
    
    for (const QString &bookmarkId : bookmarksToRemove) {
        removeBookmark(bookmarkId);
    }
    
    // Удаляем все подпапки
    QList<QString> foldersToRemove;
    for (const BookmarkFolder &folder : m_folders) {
        if (folder.parentId == id) {
            foldersToRemove.append(folder.id);
        }
    }
    
    for (const QString &folderId : foldersToRemove) {
        removeFolder(folderId);
    }
    
    m_folders.remove(id);
    emit folderRemoved(id);
    saveBookmarks();
    
    return true;
}

bool BookmarkManager::updateFolder(const QString &id, const QString &name)
{
    if (m_folders.contains(id)) {
        BookmarkFolder &folder = m_folders[id];
        folder.name = name;
        
        emit folderUpdated(folder);
        saveBookmarks();
        return true;
    }
    return false;
}

bool BookmarkManager::moveBookmark(const QString &id, const QString &newFolderId)
{
    if (m_bookmarks.contains(id)) {
        Bookmark &bookmark = m_bookmarks[id];
        bookmark.folderId = newFolderId;
        
        emit bookmarkMoved(bookmark);
        saveBookmarks();
        return true;
    }
    return false;
}

bool BookmarkManager::moveFolder(const QString &id, const QString &newParentId)
{
    if (m_folders.contains(id)) {
        // Проверяем, что новый родитель не является потомком
        QString parentId = newParentId;
        while (!parentId.isEmpty()) {
            if (parentId == id) {
                return false; // Циклическая ссылка
            }
            if (!m_folders.contains(parentId)) {
                break;
            }
            parentId = m_folders[parentId].parentId;
        }
        
        BookmarkFolder &folder = m_folders[id];
        folder.parentId = newParentId;
        
        emit folderMoved(folder);
        saveBookmarks();
        return true;
    }
    return false;
}

Bookmark BookmarkManager::getBookmark(const QString &id) const
{
    return m_bookmarks.value(id);
}

BookmarkFolder BookmarkManager::getFolder(const QString &id) const
{
    return m_folders.value(id);
}

QList<Bookmark> BookmarkManager::getBookmarksInFolder(const QString &folderId) const
{
    QList<Bookmark> result;
    for (const Bookmark &bookmark : m_bookmarks) {
        if (bookmark.folderId == folderId) {
            result.append(bookmark);
        }
    }
    return result;
}

QList<BookmarkFolder> BookmarkManager::getSubfolders(const QString &parentId) const
{
    QList<BookmarkFolder> result;
    for (const BookmarkFolder &folder : m_folders) {
        if (folder.parentId == parentId) {
            result.append(folder);
        }
    }
    return result;
}

QList<Bookmark> BookmarkManager::searchBookmarks(const QString &query) const
{
    QList<Bookmark> result;
    QString lowerQuery = query.toLower();
    
    for (const Bookmark &bookmark : m_bookmarks) {
        if (bookmark.title.toLower().contains(lowerQuery) ||
            bookmark.url.toLower().contains(lowerQuery)) {
            result.append(bookmark);
        }
    }
    return result;
}

void BookmarkManager::importBookmarks(const QString &path)
{
    // TODO: Реализовать импорт закладок из HTML или JSON
}

bool BookmarkManager::exportBookmarks(const QString &path)
{
    // TODO: Реализовать экспорт закладок в HTML или JSON
    return false;
}

void BookmarkManager::updateIcon(const QString &id, const QString &iconUrl)
{
    if (m_bookmarks.contains(id)) {
        Bookmark &bookmark = m_bookmarks[id];
        bookmark.icon = iconUrl;
        emit bookmarkUpdated(bookmark);
        saveBookmarks();
    }
}

void BookmarkManager::updateLastVisited(const QString &id)
{
    if (m_bookmarks.contains(id)) {
        Bookmark &bookmark = m_bookmarks[id];
        bookmark.lastVisited = QDateTime::currentDateTime();
        emit bookmarkUpdated(bookmark);
        saveBookmarks();
    }
} 
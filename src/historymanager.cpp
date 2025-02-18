#include "historymanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>

const QString HistoryManager::DATABASE_NAME = "browser_history.db";
const int HistoryManager::MAX_TITLE_LENGTH = 1000;
const int HistoryManager::MAX_URL_LENGTH = 2048;

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent)
{
    initializeDatabase();
}

HistoryManager::~HistoryManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void HistoryManager::initializeDatabase()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path + "/" + DATABASE_NAME);
    
    if (!m_db.open()) {
        emit databaseError(m_db.lastError().text());
        return;
    }
    
    // Создание таблиц
    QSqlQuery query(m_db);
    
    // Таблица visits (посещения)
    if (!query.exec("CREATE TABLE IF NOT EXISTS visits ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "url TEXT NOT NULL,"
                   "title TEXT,"
                   "visit_time INTEGER NOT NULL,"
                   "visit_count INTEGER DEFAULT 1,"
                   "last_visit_time INTEGER NOT NULL"
                   ")")) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    // Таблица visit_details (детали посещений)
    if (!query.exec("CREATE TABLE IF NOT EXISTS visit_details ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "url TEXT NOT NULL,"
                   "visit_time INTEGER NOT NULL,"
                   "referrer TEXT,"
                   "transition_type INTEGER"
                   ")")) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    // Индексы для ускорения поиска
    query.exec("CREATE INDEX IF NOT EXISTS idx_visits_url ON visits(url)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_visits_time ON visits(visit_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_visit_details_url ON visit_details(url)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_visit_details_time ON visit_details(visit_time)");
}

void HistoryManager::addVisit(const QString &url, const QString &title,
                            const QString &referrer, int transitionType)
{
    if (url.isEmpty() || url.length() > MAX_URL_LENGTH) {
        return;
    }
    
    QString sanitizedTitle = title;
    if (sanitizedTitle.length() > MAX_TITLE_LENGTH) {
        sanitizedTitle = sanitizedTitle.left(MAX_TITLE_LENGTH);
    }
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 timestamp = now.toSecsSinceEpoch();
    
    QSqlQuery query(m_db);
    
    // Проверяем, существует ли уже запись для этого URL
    query.prepare("SELECT id, visit_count FROM visits WHERE url = ?");
    query.addBindValue(url);
    
    if (query.exec() && query.next()) {
        // Обновляем существующую запись
        int id = query.value(0).toInt();
        int visitCount = query.value(1).toInt() + 1;
        
        query.prepare("UPDATE visits SET title = ?, visit_count = ?, "
                     "last_visit_time = ? WHERE id = ?");
        query.addBindValue(sanitizedTitle);
        query.addBindValue(visitCount);
        query.addBindValue(timestamp);
        query.addBindValue(id);
        
        if (!query.exec()) {
            emit databaseError(query.lastError().text());
            return;
        }
    } else {
        // Создаем новую запись
        query.prepare("INSERT INTO visits (url, title, visit_time, last_visit_time) "
                     "VALUES (?, ?, ?, ?)");
        query.addBindValue(url);
        query.addBindValue(sanitizedTitle);
        query.addBindValue(timestamp);
        query.addBindValue(timestamp);
        
        if (!query.exec()) {
            emit databaseError(query.lastError().text());
            return;
        }
    }
    
    // Добавляем детали посещения
    query.prepare("INSERT INTO visit_details (url, visit_time, referrer, transition_type) "
                 "VALUES (?, ?, ?, ?)");
    query.addBindValue(url);
    query.addBindValue(timestamp);
    query.addBindValue(referrer);
    query.addBindValue(transitionType);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    emit visitAdded(url, title);
}

QList<HistoryItem> HistoryManager::getHistory(const QDateTime &start,
                                            const QDateTime &end,
                                            int limit) const
{
    QList<HistoryItem> result;
    
    QSqlQuery query(m_db);
    QString sql = "SELECT url, title, visit_time, visit_count, last_visit_time "
                 "FROM visits WHERE 1=1";
    
    if (start.isValid()) {
        sql += " AND visit_time >= " + QString::number(start.toSecsSinceEpoch());
    }
    
    if (end.isValid()) {
        sql += " AND visit_time <= " + QString::number(end.toSecsSinceEpoch());
    }
    
    sql += " ORDER BY visit_time DESC";
    
    if (limit > 0) {
        sql += " LIMIT " + QString::number(limit);
    }
    
    if (query.exec(sql)) {
        while (query.next()) {
            HistoryItem item;
            item.url = query.value(0).toString();
            item.title = query.value(1).toString();
            item.visitTime = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());
            item.visitCount = query.value(3).toInt();
            item.lastVisitTime = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
            result.append(item);
        }
    }
    
    return result;
}

QList<HistoryItem> HistoryManager::searchHistory(const QString &text, int limit) const
{
    QList<HistoryItem> result;
    
    if (text.isEmpty()) {
        return result;
    }
    
    QSqlQuery query(m_db);
    QString sql = "SELECT url, title, visit_time, visit_count, last_visit_time "
                 "FROM visits WHERE url LIKE ? OR title LIKE ? "
                 "ORDER BY last_visit_time DESC";
    
    if (limit > 0) {
        sql += " LIMIT " + QString::number(limit);
    }
    
    query.prepare(sql);
    QString pattern = "%" + text + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    
    if (query.exec()) {
        while (query.next()) {
            HistoryItem item;
            item.url = query.value(0).toString();
            item.title = query.value(1).toString();
            item.visitTime = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());
            item.visitCount = query.value(3).toInt();
            item.lastVisitTime = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
            result.append(item);
        }
    }
    
    return result;
}

void HistoryManager::deleteUrl(const QString &url)
{
    QSqlQuery query(m_db);
    
    // Удаляем из таблицы visits
    query.prepare("DELETE FROM visits WHERE url = ?");
    query.addBindValue(url);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    // Удаляем из таблицы visit_details
    query.prepare("DELETE FROM visit_details WHERE url = ?");
    query.addBindValue(url);
    
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    emit urlDeleted(url);
}

void HistoryManager::deleteTimeRange(const QDateTime &start, const QDateTime &end)
{
    QSqlQuery query(m_db);
    QString sql = "DELETE FROM visits WHERE 1=1";
    
    if (start.isValid()) {
        sql += " AND visit_time >= " + QString::number(start.toSecsSinceEpoch());
    }
    
    if (end.isValid()) {
        sql += " AND visit_time <= " + QString::number(end.toSecsSinceEpoch());
    }
    
    if (!query.exec(sql)) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    sql = sql.replace("visits", "visit_details");
    
    if (!query.exec(sql)) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    emit historyRangeDeleted(start, end);
}

void HistoryManager::clearHistory()
{
    QSqlQuery query(m_db);
    
    // Очищаем таблицу visits
    if (!query.exec("DELETE FROM visits")) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    // Очищаем таблицу visit_details
    if (!query.exec("DELETE FROM visit_details")) {
        emit databaseError(query.lastError().text());
        return;
    }
    
    // Сбрасываем автоинкремент
    query.exec("DELETE FROM sqlite_sequence WHERE name='visits'");
    query.exec("DELETE FROM sqlite_sequence WHERE name='visit_details'");
    
    emit historyCleared();
}

QList<HistoryItem> HistoryManager::getMostVisited(int limit) const
{
    QList<HistoryItem> result;
    
    QSqlQuery query(m_db);
    QString sql = "SELECT url, title, visit_time, visit_count, last_visit_time "
                 "FROM visits ORDER BY visit_count DESC";
    
    if (limit > 0) {
        sql += " LIMIT " + QString::number(limit);
    }
    
    if (query.exec(sql)) {
        while (query.next()) {
            HistoryItem item;
            item.url = query.value(0).toString();
            item.title = query.value(1).toString();
            item.visitTime = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());
            item.visitCount = query.value(3).toInt();
            item.lastVisitTime = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
            result.append(item);
        }
    }
    
    return result;
}

QList<HistoryItem> HistoryManager::getRecentlyVisited(int limit) const
{
    QList<HistoryItem> result;
    
    QSqlQuery query(m_db);
    QString sql = "SELECT url, title, visit_time, visit_count, last_visit_time "
                 "FROM visits ORDER BY last_visit_time DESC";
    
    if (limit > 0) {
        sql += " LIMIT " + QString::number(limit);
    }
    
    if (query.exec(sql)) {
        while (query.next()) {
            HistoryItem item;
            item.url = query.value(0).toString();
            item.title = query.value(1).toString();
            item.visitTime = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());
            item.visitCount = query.value(3).toInt();
            item.lastVisitTime = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
            result.append(item);
        }
    }
    
    return result;
}

void HistoryManager::optimizeDatabase()
{
    QSqlQuery query(m_db);
    
    // Удаляем дубликаты
    query.exec("DELETE FROM visits WHERE id NOT IN ("
              "SELECT MIN(id) FROM visits GROUP BY url)");
    
    // Очищаем старые записи (старше 90 дней)
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-90);
    query.prepare("DELETE FROM visits WHERE visit_time < ?");
    query.addBindValue(cutoff.toSecsSinceEpoch());
    query.exec();
    
    query.prepare("DELETE FROM visit_details WHERE visit_time < ?");
    query.addBindValue(cutoff.toSecsSinceEpoch());
    query.exec();
    
    // Оптимизируем базу данных
    query.exec("VACUUM");
    
    emit databaseOptimized();
}

void HistoryManager::importHistory(const QString &path)
{
    // TODO: Реализовать импорт истории из других браузеров
}

bool HistoryManager::exportHistory(const QString &path) const
{
    // TODO: Реализовать экспорт истории
    return false;
} 
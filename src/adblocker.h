#ifndef ADBLOCKER_H
#define ADBLOCKER_H

#include <QObject>
#include <QUrl>
#include <QDateTime>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QWebEngineUrlRequestInfo>
#include <QMap>
#include <QPair>

class QNetworkAccessManager;
class QTimer;

struct FilterRule {
    QString pattern;
    QRegularExpression regex;
    QStringList domains;
    QStringList excludedDomains;
    bool isException = false;
    bool isHtmlFilter = false;
    bool isCssFilter = false;
    bool isJsFilter = false;
    bool enabled = true;
    int hitCount = 0;
    QDateTime creationDate;
    QDateTime lastHit;
};

struct FilterList {
    QString name;
    QString url;
    QString version;
    QString homepage;
    QString author;
    bool enabled = true;
    int updateInterval = 24; // часы
    QDateTime lastUpdate;
    QList<FilterRule> rules;
};

class AdBlocker : public QObject
{
    Q_OBJECT

public:
    explicit AdBlocker(QObject *parent = nullptr);
    ~AdBlocker();

    // Основные методы
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    bool isAggressiveBlocking() const { return m_aggressiveBlocking; }
    void setAggressiveBlocking(bool aggressive);
    int getAutoUpdateInterval() const { return m_autoUpdateInterval; }
    void setAutoUpdateInterval(int hours);
    
    // Методы управления правилами
    bool addCustomRule(const QString &rule);
    bool removeCustomRule(const QString &rule);
    bool enableRule(const QString &rule);
    bool disableRule(const QString &rule);
    QList<FilterRule> getCustomRules() const;
    
    // Методы управления списками фильтров
    bool enableFilterList(const QString &listId);
    bool disableFilterList(const QString &listId);
    bool updateFilterList(const QString &listId);
    QMap<QString, FilterList> getFilterLists() const;
    
    // Статистика
    int getTotalBlockedCount() const { return m_totalBlocked; }
    QHash<QString, int> getDomainStats() const;
    void resetStats();

signals:
    void enabledChanged(bool enabled);
    void aggressiveBlockingChanged(bool aggressive);
    void filterListUpdated(const QString &listId);
    void filterListUpdateFailed(const QString &listId, const QString &error);
    void ruleAdded(const FilterRule &rule);
    void ruleRemoved(const FilterRule &rule);
    void statsUpdated();

private:
    static const QString SETTINGS_FILENAME;
    static const QString STATS_FILENAME;
    static const QMap<QString, QPair<QString, QString>> PREDEFINED_LISTS;
    static const QMap<QString, QPair<QString, QString>> REGIONAL_LISTS;
    static const int DEFAULT_UPDATE_INTERVAL = 24; // часы

    bool initializeFilters();
    void loadCustomRules();
    bool parseRule(const QString &line, FilterRule &rule);
    void compileRegex(FilterRule &rule);
    void loadSettings();
    void saveSettings();
    void updateAllFilterLists();
    void cleanupExpiredRules();
    
    QNetworkAccessManager *m_networkManager;
    QString m_settingsPath;
    bool m_enabled;
    bool m_aggressiveBlocking;
    int m_autoUpdateInterval;
    int m_totalBlocked;
    QDateTime m_lastUpdate;
    
    QMap<QString, FilterList> m_filterLists;
    QList<FilterRule> m_customRules;
    QHash<QString, QStringList> m_cssRulesCache;
    QHash<QString, QStringList> m_jsRulesCache;
    QHash<QString, QStringList> m_domainRules;
};

#endif // ADBLOCKER_H 
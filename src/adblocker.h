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

struct FilterRule {
    QString pattern;
    QRegularExpression regex;
    bool isException;
    bool isHtmlFilter;
    bool isCssFilter;
    bool isJsFilter;
    QStringList domains;
    QStringList excludedDomains;
    QDateTime creationDate;
    QString source;
    bool enabled;
    int hitCount;
};

struct FilterList {
    QString name;
    QString url;
    QString description;
    QString version;
    QDateTime lastUpdate;
    bool enabled;
    QList<FilterRule> rules;
    QString homepage;
    QString author;
    QString license;
    QDateTime expirationDate;
    int updateInterval;
};

class AdBlocker : public QObject
{
    Q_OBJECT

public:
    explicit AdBlocker(QObject *parent = nullptr);
    ~AdBlocker();

    // Управление фильтрами
    bool addFilterList(const QString &url, bool enabled = true);
    bool removeFilterList(const QString &url);
    bool enableFilterList(const QString &url, bool enable = true);
    bool updateFilterList(const QString &url);
    bool updateAllFilterLists();
    
    // Добавление пользовательских правил
    bool addCustomRule(const QString &rule);
    bool removeCustomRule(const QString &rule);
    QStringList getCustomRules() const;
    
    // Проверка блокировки
    bool shouldBlock(const QUrl &url, const QString &sourceUrl) const;
    bool shouldBlockRequest(const QWebEngineUrlRequestInfo &info) const;
    bool isElementHidden(const QString &url, const QString &elementId) const;
    
    // Статистика
    int getBlockedCount() const;
    QHash<QString, int> getBlockedDomains() const;
    QList<QPair<QString, int>> getTopBlockedDomains(int limit = 10) const;
    void resetStatistics();
    
    // Белый список
    bool addToWhitelist(const QString &domain);
    bool removeFromWhitelist(const QString &domain);
    QStringList getWhitelist() const;
    bool isDomainWhitelisted(const QString &domain) const;
    
    // Настройки
    void setEnabled(bool enable);
    bool isEnabled() const;
    void setAggressiveBlocking(bool enable);
    bool isAggressiveBlocking() const;
    void setAutoUpdateInterval(int hours);
    int getAutoUpdateInterval() const;
    
    // Экспорт/Импорт
    bool exportSettings(const QString &path) const;
    bool importSettings(const QString &path);
    bool exportStatistics(const QString &path) const;
    
    // Косметические правила
    QString getCssRules(const QString &url) const;
    QString getJsRules(const QString &url) const;
    QStringList getElementHidingRules(const QString &url) const;

signals:
    void filterListAdded(const QString &url);
    void filterListRemoved(const QString &url);
    void filterListEnabled(const QString &url, bool enabled);
    void filterListUpdated(const QString &url);
    void filterListUpdateFailed(const QString &url, const QString &error);
    void customRuleAdded(const QString &rule);
    void customRuleRemoved(const QString &rule);
    void whitelistChanged();
    void statisticsReset();
    void blockCountChanged(int count);
    void settingsChanged();
    void elementBlocked(const QString &url, const QString &element);
    void requestBlocked(const QUrl &url, const QString &sourceUrl);

private slots:
    void handleAutoUpdate();
    void processUpdateResponse(const QString &url, const QByteArray &data);
    void checkForUpdates();

private:
    bool initializeFilters();
    bool parseFilterList(const QString &content, FilterList &list);
    bool parseRule(const QString &line, FilterRule &rule);
    void compileRegex(FilterRule &rule);
    bool matchesFilter(const FilterRule &rule, const QString &url, 
                      const QString &sourceUrl) const;
    bool matchesDomain(const QString &domain, const QStringList &domains, 
                      const QStringList &excludedDomains) const;
    QString extractDomain(const QString &url) const;
    void saveSettings();
    void loadSettings();
    void updateCosmeticFilters();
    void cleanupExpiredRules();
    
    QHash<QString, FilterList> m_filterLists;
    QList<FilterRule> m_customRules;
    QStringList m_whitelist;
    QHash<QString, int> m_blockedDomains;
    int m_totalBlocked;
    bool m_enabled;
    bool m_aggressiveBlocking;
    int m_autoUpdateInterval;
    QDateTime m_lastUpdate;
    QString m_settingsPath;
    
    // Кэши для оптимизации
    QHash<QString, QList<FilterRule>> m_domainRules;
    QHash<QString, QString> m_cssRulesCache;
    QHash<QString, QString> m_jsRulesCache;
    
    static const QString SETTINGS_FILENAME;
    static const QString STATS_FILENAME;
    static const int DEFAULT_UPDATE_INTERVAL = 24; // часов
};

#endif // ADBLOCKER_H 
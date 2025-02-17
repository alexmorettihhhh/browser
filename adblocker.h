#ifndef ADBLOCKER_H
#define ADBLOCKER_H

#include <QWebEngineUrlRequestInterceptor>
#include <QStringList>
#include <QRegularExpression>
#include <QHash>
#include <QDateTime>
#include <QUrl>

struct BlockRule {
    QString pattern;
    QRegularExpression regex;
    bool isException;
    QString domain;
    bool isUrlFilter;
    bool isElementHide;
};

struct BlockStatistics {
    int totalRequests;
    int blockedRequests;
    QHash<QString, int> blockedDomains;
    QDateTime lastUpdate;
};

class AdBlocker : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    explicit AdBlocker(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
    
    // Новые публичные методы
    void reloadRules();
    void addCustomRule(const QString &rule);
    void removeCustomRule(const QString &rule);
    void addToWhitelist(const QString &domain);
    void removeFromWhitelist(const QString &domain);
    BlockStatistics getStatistics() const;
    void clearStatistics();
    void enableEasyList(bool enable);
    
signals:
    void ruleAdded(const QString &rule);
    void ruleRemoved(const QString &rule);
    void statisticsUpdated(const BlockStatistics &stats);

private:
    QList<BlockRule> loadBlockList();
    QList<BlockRule> parseEasyList(const QString &content);
    bool shouldBlock(const QUrl &url, const QString &type);
    bool isWhitelisted(const QString &domain);
    void updateStatistics(const QString &domain, bool blocked);
    QString getBaseDomain(const QString &url);
    void downloadEasyList();
    void parseRule(const QString &rule, QList<BlockRule> &rules);
    
    QList<BlockRule> m_rules;
    QStringList m_whitelist;
    QHash<QString, bool> m_cache;
    BlockStatistics m_statistics;
    bool m_easyListEnabled;
    QString m_easyListPath;
    QDateTime m_lastEasyListUpdate;
};

#endif // ADBLOCKER_H 
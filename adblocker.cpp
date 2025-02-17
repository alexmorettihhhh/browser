#include "adblocker.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

AdBlocker::AdBlocker(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_easyListEnabled(false)
    , m_statistics{0, 0, QHash<QString, int>(), QDateTime::currentDateTime()}
{
    m_easyListPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/easylist.txt";
    m_rules = loadBlockList();
    
    // Автоматическое обновление EasyList каждые 7 дней
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &AdBlocker::downloadEasyList);
    updateTimer->start(7 * 24 * 60 * 60 * 1000); // 7 дней в миллисекундах
}

void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    QString urlString = info.requestUrl().toString();
    QString type = info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage ? "image" :
                  info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeScript ? "script" :
                  info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ? "stylesheet" :
                  "other";
    
    m_statistics.totalRequests++;
    
    // Проверяем кэш
    if (m_cache.contains(urlString)) {
        if (m_cache[urlString]) {
            info.block(true);
            updateStatistics(getBaseDomain(urlString), true);
        }
        return;
    }
    
    bool shouldBeBlocked = shouldBlock(info.requestUrl(), type);
    m_cache[urlString] = shouldBeBlocked;
    
    if (shouldBeBlocked) {
        info.block(true);
        updateStatistics(getBaseDomain(urlString), true);
    }
}

QList<BlockRule> AdBlocker::loadBlockList()
{
    QList<BlockRule> rules;
    
    // Загружаем встроенные правила
    QFile file(":/adblock/rules.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            parseRule(in.readLine(), rules);
        }
        file.close();
    }
    
    // Загружаем пользовательские правила
    QFile customFile(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/custom_rules.txt");
    if (customFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&customFile);
        while (!in.atEnd()) {
            parseRule(in.readLine(), rules);
        }
        customFile.close();
    }
    
    // Загружаем EasyList если включен
    if (m_easyListEnabled && QFile::exists(m_easyListPath)) {
        QFile easyList(m_easyListPath);
        if (easyList.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&easyList);
            QString content = in.readAll();
            easyList.close();
            rules.append(parseEasyList(content));
        }
    }
    
    return rules;
}

void AdBlocker::parseRule(const QString &rule, QList<BlockRule> &rules)
{
    if (rule.isEmpty() || rule.startsWith("!"))
        return;
        
    BlockRule blockRule;
    blockRule.pattern = rule;
    blockRule.isException = rule.startsWith("@@");
    blockRule.isElementHide = rule.contains("##") || rule.contains("#@#");
    
    QString processedRule = rule;
    if (blockRule.isException)
        processedRule = rule.mid(2);
        
    // Обработка доменных ограничений
    if (processedRule.contains("$domain=")) {
        int domainStart = processedRule.indexOf("$domain=");
        blockRule.domain = processedRule.mid(domainStart + 8);
        processedRule = processedRule.left(domainStart);
    }
    
    // Преобразование правила в регулярное выражение
    QString regexPattern = processedRule;
    regexPattern.replace(".", "\\.");
    regexPattern.replace("*", ".*");
    regexPattern.replace("?", ".");
    regexPattern.replace("|", "^");
    regexPattern.replace("^", "\\^");
    regexPattern.replace("[", "\\[");
    regexPattern.replace("]", "\\]");
    
    blockRule.regex = QRegularExpression(regexPattern);
    blockRule.isUrlFilter = !blockRule.isElementHide;
    
    rules.append(blockRule);
}

bool AdBlocker::shouldBlock(const QUrl &url, const QString &type)
{
    QString urlString = url.toString();
    QString domain = getBaseDomain(urlString);
    
    // Проверяем белый список
    if (isWhitelisted(domain))
        return false;
        
    for (const BlockRule &rule : m_rules) {
        if (!rule.isUrlFilter)
            continue;
            
        // Проверяем доменные ограничения
        if (!rule.domain.isEmpty()) {
            if (!domain.contains(rule.domain))
                continue;
        }
        
        if (rule.regex.match(urlString).hasMatch()) {
            return !rule.isException;
        }
    }
    
    return false;
}

QString AdBlocker::getBaseDomain(const QString &urlString)
{
    QUrl url(urlString);
    QString host = url.host();
    
    // Удаляем www. если есть
    if (host.startsWith("www."))
        host = host.mid(4);
        
    return host;
}

void AdBlocker::updateStatistics(const QString &domain, bool blocked)
{
    if (blocked) {
        m_statistics.blockedRequests++;
        m_statistics.blockedDomains[domain]++;
        m_statistics.lastUpdate = QDateTime::currentDateTime();
        emit statisticsUpdated(m_statistics);
    }
}

bool AdBlocker::isWhitelisted(const QString &domain)
{
    return m_whitelist.contains(domain);
}

void AdBlocker::addToWhitelist(const QString &domain)
{
    if (!m_whitelist.contains(domain)) {
        m_whitelist.append(domain);
        m_cache.clear(); // Очищаем кэш при изменении белого списка
    }
}

void AdBlocker::removeFromWhitelist(const QString &domain)
{
    m_whitelist.removeAll(domain);
    m_cache.clear();
}

void AdBlocker::addCustomRule(const QString &rule)
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/custom_rules.txt");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << rule << "\n";
        file.close();
        
        QList<BlockRule> rules;
        parseRule(rule, rules);
        if (!rules.isEmpty()) {
            m_rules.append(rules.first());
            m_cache.clear();
            emit ruleAdded(rule);
        }
    }
}

void AdBlocker::removeCustomRule(const QString &rule)
{
    // Читаем все правила
    QFile file(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/custom_rules.txt");
    QStringList rules;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line != rule)
                rules.append(line);
        }
        file.close();
    }
    
    // Записываем обратно без удаленного правила
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &r : rules)
            out << r << "\n";
        file.close();
    }
    
    // Обновляем правила в памяти
    m_rules = loadBlockList();
    m_cache.clear();
    emit ruleRemoved(rule);
}

void AdBlocker::downloadEasyList()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://easylist.to/easylist/easylist.txt"));
    
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString content = reply->readAll();
            QFile file(m_easyListPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << content;
                file.close();
                
                if (m_easyListEnabled) {
                    m_rules = loadBlockList();
                    m_cache.clear();
                }
            }
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

void AdBlocker::enableEasyList(bool enable)
{
    if (m_easyListEnabled != enable) {
        m_easyListEnabled = enable;
        if (enable && !QFile::exists(m_easyListPath)) {
            downloadEasyList();
        }
        m_rules = loadBlockList();
        m_cache.clear();
    }
}

BlockStatistics AdBlocker::getStatistics() const
{
    return m_statistics;
}

void AdBlocker::clearStatistics()
{
    m_statistics = BlockStatistics{0, 0, QHash<QString, int>(), QDateTime::currentDateTime()};
    emit statisticsUpdated(m_statistics);
}

void AdBlocker::reloadRules()
{
    m_rules = loadBlockList();
    m_cache.clear();
} 
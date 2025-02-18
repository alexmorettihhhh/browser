#include "adblocker.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>

const QString AdBlocker::SETTINGS_FILENAME = "adblock_settings.json";
const QString AdBlocker::STATS_FILENAME = "adblock_stats.json";

// Предопределенные списки фильтров
const QMap<QString, QPair<QString, QString>> AdBlocker::PREDEFINED_LISTS = {
    {"easylist", {"EasyList", "https://easylist.to/easylist/easylist.txt"}},
    {"easyprivacy", {"EasyPrivacy", "https://easylist.to/easylist/easyprivacy.txt"}},
    {"fanboy-social", {"Fanboy's Social Blocking List", "https://easylist.to/easylist/fanboy-social.txt"}},
    {"fanboy-annoyance", {"Fanboy's Annoyance List", "https://easylist.to/easylist/fanboy-annoyance.txt"}},
    {"peter-lowe", {"Peter Lowe's List", "https://pgl.yoyo.org/adservers/serverlist.php?hostformat=adblockplus&showintro=0&mimetype=plaintext"}},
    {"adguard-base", {"AdGuard Base Filter", "https://filters.adtidy.org/extension/ublock/filters/2.txt"}},
    {"adguard-mobile", {"AdGuard Mobile Ads Filter", "https://filters.adtidy.org/extension/ublock/filters/11.txt"}},
    {"ublock-filters", {"uBlock Origin Filters", "https://raw.githubusercontent.com/uBlockOrigin/uAssets/master/filters/filters.txt"}}
};

// Региональные списки фильтров
const QMap<QString, QPair<QString, QString>> AdBlocker::REGIONAL_LISTS = {
    {"ru", {"RU AdList", "https://easylist-downloads.adblockplus.org/advblock.txt"}},
    {"de", {"DE List", "https://easylist-downloads.adblockplus.org/easylistgermany.txt"}},
    {"fr", {"FR List", "https://easylist-downloads.adblockplus.org/liste_fr.txt"}},
    {"jp", {"JP List", "https://raw.githubusercontent.com/k2jp/abp-japanese-filters/master/abpjf.txt"}},
    {"cn", {"CN List", "https://easylist-downloads.adblockplus.org/easylistchina.txt"}}
};

AdBlocker::AdBlocker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_enabled(true)
    , m_aggressiveBlocking(false)
    , m_autoUpdateInterval(DEFAULT_UPDATE_INTERVAL)
    , m_totalBlocked(0)
{
    m_settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(m_settingsPath);
    
    loadSettings();
    initializeFilters();
    
    // Автоматическое обновление списков
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &AdBlocker::checkForUpdates);
    updateTimer->start(m_autoUpdateInterval * 60 * 60 * 1000);
    
    // Очистка устаревших правил каждые 24 часа
    QTimer *cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &AdBlocker::cleanupExpiredRules);
    cleanupTimer->start(24 * 60 * 60 * 1000);
    
    // Оптимизация кэша каждые 6 часов
    QTimer *cacheTimer = new QTimer(this);
    connect(cacheTimer, &QTimer::timeout, this, [this]() {
        m_cssRulesCache.clear();
        m_jsRulesCache.clear();
        m_domainRules.clear();
    });
    cacheTimer->start(6 * 60 * 60 * 1000);
}

AdBlocker::~AdBlocker()
{
    saveSettings();
    delete m_networkManager;
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

bool AdBlocker::initializeFilters()
{
    // Добавляем основные списки фильтров
    for (auto it = PREDEFINED_LISTS.constBegin(); it != PREDEFINED_LISTS.constEnd(); ++it) {
        FilterList list;
        list.name = it.value().first;
        list.url = it.value().second;
        list.enabled = true;
        list.updateInterval = 24; // Обновление раз в сутки
        m_filterLists[it.key()] = list;
    }
    
    // Добавляем региональные списки на основе локали системы
    QString locale = QLocale::system().name().split('_').first();
    if (REGIONAL_LISTS.contains(locale)) {
        auto regional = REGIONAL_LISTS[locale];
        FilterList list;
        list.name = regional.first;
        list.url = regional.second;
        list.enabled = true;
        list.updateInterval = 24;
        m_filterLists[locale] = list;
    }
    
    // Загружаем сохраненные правила
    loadCustomRules();
    
    // Обновляем все списки при первом запуске
    if (m_lastUpdate.isNull()) {
        updateAllFilterLists();
    }
    
    return true;
}

void AdBlocker::loadCustomRules()
{
    QString path = m_settingsPath + "/custom_rules.txt";
    QFile file(path);
    
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith("!")) {
                FilterRule rule;
                if (parseRule(line, rule)) {
                    m_customRules.append(rule);
                }
            }
        }
        file.close();
    }
}

bool AdBlocker::parseRule(const QString &line, FilterRule &rule)
{
    rule.pattern = line;
    rule.enabled = true;
    rule.hitCount = 0;
    rule.creationDate = QDateTime::currentDateTime();
    
    // Проверяем, является ли правило исключением
    if (line.startsWith("@@")) {
        rule.isException = true;
        rule.pattern = line.mid(2);
    } else {
        rule.isException = false;
    }
    
    // Проверяем тип правила
    if (line.contains("##")) {
        rule.isHtmlFilter = true;
        rule.isCssFilter = true;
    } else if (line.contains("#@#")) {
        rule.isHtmlFilter = true;
        rule.isException = true;
    } else if (line.contains("#?#")) {
        rule.isJsFilter = true;
    }
    
    // Извлекаем домены
    int domainSeparator = line.indexOf("$domain=");
    if (domainSeparator != -1) {
        QString domains = line.mid(domainSeparator + 8);
        QStringList domainList = domains.split("|");
        for (const QString &domain : domainList) {
            if (domain.startsWith("~")) {
                rule.excludedDomains.append(domain.mid(1));
            } else {
                rule.domains.append(domain);
            }
        }
    }
    
    // Компилируем регулярное выражение
    compileRegex(rule);
    
    return true;
}

void AdBlocker::compileRegex(FilterRule &rule)
{
    QString pattern = rule.pattern;
    
    // Заменяем специальные символы
    pattern.replace(".", "\\.");
    pattern.replace("*", ".*");
    pattern.replace("?", "\\?");
    pattern.replace("|", "\\|");
    pattern.replace("^", "[^\\w\\d\\_\\-\\.]");
    
    // Добавляем границы для точного соответствия
    if (pattern.startsWith("||")) {
        pattern = "^[\\w\\-]+:\\/+(?:[^\\/]+\\.)?(" + pattern.mid(2) + ")";
    } else if (pattern.startsWith("|")) {
        pattern = "^" + pattern.mid(1);
    }
    
    if (pattern.endsWith("|")) {
        pattern = pattern.left(pattern.length() - 1) + "$";
    }
    
    rule.regex = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
}

bool AdBlocker::shouldBlockRequest(const QWebEngineUrlRequestInfo &info) const
{
    return shouldBlock(info.requestUrl(), info.firstPartyUrl().toString());
}

bool AdBlocker::matchesFilter(const FilterRule &rule, const QString &url,
                            const QString &sourceUrl) const
{
    // Проверка доменных ограничений
    if (!rule.domains.isEmpty() || !rule.excludedDomains.isEmpty()) {
        QString sourceDomain = extractDomain(sourceUrl);
        if (!matchesDomain(sourceDomain, rule.domains, rule.excludedDomains)) {
            return false;
        }
    }
    
    // Проверка регулярного выражения
    return rule.regex.match(url).hasMatch();
}

bool AdBlocker::matchesDomain(const QString &domain, const QStringList &domains,
                            const QStringList &excludedDomains) const
{
    // Проверка исключенных доменов
    for (const QString &excluded : excludedDomains) {
        if (domain.endsWith(excluded)) return false;
    }
    
    // Если список разрешенных доменов пуст, разрешаем все
    if (domains.isEmpty()) return true;
    
    // Проверка разрешенных доменов
    for (const QString &allowed : domains) {
        if (domain.endsWith(allowed)) return true;
    }
    
    return false;
}

QString AdBlocker::extractDomain(const QString &url) const
{
    QUrl parsedUrl(url);
    QString host = parsedUrl.host();
    
    // Удаляем www. если есть
    if (host.startsWith("www.")) {
        host = host.mid(4);
    }
    
    return host;
}

bool AdBlocker::addFilterList(const QString &url, bool enabled)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, url, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            processUpdateResponse(url, data);
        } else {
            emit filterListUpdateFailed(url, reply->errorString());
        }
        reply->deleteLater();
        reply->manager()->deleteLater();
    });
    
    return true;
}

void AdBlocker::processUpdateResponse(const QString &url, const QByteArray &data)
{
    FilterList list;
    if (parseFilterList(QString::fromUtf8(data), list)) {
        list.url = url;
        list.lastUpdate = QDateTime::currentDateTime();
        m_filterLists[url] = list;
        emit filterListUpdated(url);
        saveSettings();
    } else {
        emit filterListUpdateFailed(url, "Failed to parse filter list");
    }
}

bool AdBlocker::removeFilterList(const QString &url)
{
    if (m_filterLists.remove(url) > 0) {
        emit filterListRemoved(url);
        saveSettings();
        return true;
    }
    return false;
}

bool AdBlocker::enableFilterList(const QString &url, bool enable)
{
    if (m_filterLists.contains(url)) {
        m_filterLists[url].enabled = enable;
        emit filterListEnabled(url, enable);
        saveSettings();
        return true;
    }
    return false;
}

bool AdBlocker::updateFilterList(const QString &url)
{
    if (!m_filterLists.contains(url)) return false;
    return addFilterList(url, m_filterLists[url].enabled);
}

bool AdBlocker::updateAllFilterLists()
{
    bool success = true;
    for (const auto &list : m_filterLists) {
        if (!list.url.isEmpty()) {
            if (!updateFilterList(list.url)) {
                success = false;
            }
        }
    }
    return success;
}

void AdBlocker::saveSettings()
{
    QString filePath = m_settingsPath + "/" + SETTINGS_FILENAME;
    QJsonObject root;
    
    root["enabled"] = m_enabled;
    root["aggressiveBlocking"] = m_aggressiveBlocking;
    root["autoUpdateInterval"] = m_autoUpdateInterval;
    
    QJsonArray whitelistArray;
    for (const QString &domain : m_whitelist) {
        whitelistArray.append(domain);
    }
    root["whitelist"] = whitelistArray;
    
    QJsonArray listsArray;
    for (const auto &pair : m_filterLists.toStdMap()) {
        QJsonObject listObj;
        const FilterList &list = pair.second;
        
        listObj["url"] = list.url;
        listObj["name"] = list.name;
        listObj["enabled"] = list.enabled;
        listObj["version"] = list.version;
        listObj["lastUpdate"] = list.lastUpdate.toString(Qt::ISODate);
        
        listsArray.append(listObj);
    }
    root["filterLists"] = listsArray;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void AdBlocker::loadSettings()
{
    QString filePath = m_settingsPath + "/" + SETTINGS_FILENAME;
    QFile file(filePath);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            m_enabled = root["enabled"].toBool(true);
            m_aggressiveBlocking = root["aggressiveBlocking"].toBool(false);
            m_autoUpdateInterval = root["autoUpdateInterval"].toInt(DEFAULT_UPDATE_INTERVAL);
            
            m_whitelist.clear();
            QJsonArray whitelistArray = root["whitelist"].toArray();
            for (const QJsonValue &val : whitelistArray) {
                m_whitelist.append(val.toString());
            }
            
            QJsonArray listsArray = root["filterLists"].toArray();
            for (const QJsonValue &val : listsArray) {
                QJsonObject listObj = val.toObject();
                QString url = listObj["url"].toString();
                
                if (!url.isEmpty()) {
                    FilterList &list = m_filterLists[url];
                    list.url = url;
                    list.name = listObj["name"].toString();
                    list.enabled = listObj["enabled"].toBool(true);
                    list.version = listObj["version"].toString();
                    list.lastUpdate = QDateTime::fromString(
                        listObj["lastUpdate"].toString(),
                        Qt::ISODate
                    );
                }
            }
        }
    }
}

void AdBlocker::updateCosmeticFilters()
{
    m_cssRulesCache.clear();
    m_jsRulesCache.clear();
    
    for (const auto &list : m_filterLists) {
        if (!list.enabled) continue;
        
        for (const auto &rule : list.rules) {
            if (!rule.enabled) continue;
            
            if (rule.isCssFilter) {
                // Обработка CSS-правил
                for (const QString &domain : rule.domains) {
                    QString &css = m_cssRulesCache[domain];
                    css += rule.pattern + "\n";
                }
            }
            
            if (rule.isJsFilter) {
                // Обработка JS-правил
                for (const QString &domain : rule.domains) {
                    QString &js = m_jsRulesCache[domain];
                    js += rule.pattern + "\n";
                }
            }
        }
    }
}

QString AdBlocker::getCssRules(const QString &url) const
{
    QString domain = extractDomain(url);
    return m_cssRulesCache.value(domain);
}

QString AdBlocker::getJsRules(const QString &url) const
{
    QString domain = extractDomain(url);
    return m_jsRulesCache.value(domain);
}

QStringList AdBlocker::getElementHidingRules(const QString &url) const
{
    QStringList rules;
    QString domain = extractDomain(url);
    
    for (const auto &list : m_filterLists) {
        if (!list.enabled) continue;
        
        for (const auto &rule : list.rules) {
            if (!rule.enabled || !rule.isHtmlFilter) continue;
            
            if (matchesDomain(domain, rule.domains, rule.excludedDomains)) {
                rules.append(rule.pattern);
            }
        }
    }
    
    return rules;
}

bool AdBlocker::addCustomRule(const QString &rule)
{
    FilterRule newRule;
    if (parseRule(rule, newRule)) {
        m_customRules.append(newRule);
        saveSettings();
        emit ruleAdded(newRule);
        return true;
    }
    return false;
}

bool AdBlocker::removeCustomRule(const QString &rule)
{
    for (int i = 0; i < m_customRules.size(); ++i) {
        if (m_customRules[i].pattern == rule) {
            FilterRule removedRule = m_customRules.takeAt(i);
            saveSettings();
            emit ruleRemoved(removedRule);
            return true;
        }
    }
    return false;
}

bool AdBlocker::enableRule(const QString &rule)
{
    for (auto &customRule : m_customRules) {
        if (customRule.pattern == rule) {
            customRule.enabled = true;
            saveSettings();
            return true;
        }
    }
    return false;
}

bool AdBlocker::disableRule(const QString &rule)
{
    for (auto &customRule : m_customRules) {
        if (customRule.pattern == rule) {
            customRule.enabled = false;
            saveSettings();
            return true;
        }
    }
    return false;
}

QList<FilterRule> AdBlocker::getCustomRules() const
{
    return m_customRules;
}

bool AdBlocker::addToWhitelist(const QString &domain)
{
    if (!m_whitelist.contains(domain)) {
        m_whitelist.append(domain);
        emit whitelistChanged();
        saveSettings();
        return true;
    }
    return false;
}

bool AdBlocker::removeFromWhitelist(const QString &domain)
{
    if (m_whitelist.removeOne(domain)) {
        emit whitelistChanged();
        saveSettings();
        return true;
    }
    return false;
}

QStringList AdBlocker::getWhitelist() const
{
    return m_whitelist;
}

bool AdBlocker::isDomainWhitelisted(const QString &domain) const
{
    for (const QString &whitelisted : m_whitelist) {
        if (domain.endsWith(whitelisted)) {
            return true;
        }
    }
    return false;
}

void AdBlocker::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        saveSettings();
        emit enabledChanged(enabled);
    }
}

bool AdBlocker::isEnabled() const
{
    return m_enabled;
}

void AdBlocker::setAggressiveBlocking(bool aggressive)
{
    if (m_aggressiveBlocking != aggressive) {
        m_aggressiveBlocking = aggressive;
        saveSettings();
        emit aggressiveBlockingChanged(aggressive);
    }
}

bool AdBlocker::isAggressiveBlocking() const
{
    return m_aggressiveBlocking;
}

void AdBlocker::setAutoUpdateInterval(int hours)
{
    if (hours > 0 && m_autoUpdateInterval != hours) {
        m_autoUpdateInterval = hours;
        saveSettings();
    }
}

int AdBlocker::getAutoUpdateInterval() const
{
    return m_autoUpdateInterval;
}

int AdBlocker::getBlockedCount() const
{
    return m_totalBlocked;
}

QHash<QString, int> AdBlocker::getBlockedDomains() const
{
    return m_blockedDomains;
}

QList<QPair<QString, int>> AdBlocker::getTopBlockedDomains(int limit) const
{
    QList<QPair<QString, int>> result;
    for (auto it = m_blockedDomains.begin(); it != m_blockedDomains.end(); ++it) {
        result.append(qMakePair(it.key(), it.value()));
    }
    
    std::sort(result.begin(), result.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                  return a.second > b.second;
              });
    
    if (limit > 0 && result.size() > limit) {
        result = result.mid(0, limit);
    }
    
    return result;
}

void AdBlocker::resetStatistics()
{
    m_totalBlocked = 0;
    m_blockedDomains.clear();
    emit statisticsReset();
    emit blockCountChanged(0);
}

bool AdBlocker::exportSettings(const QString &path) const
{
    QJsonObject root;
    
    root["enabled"] = m_enabled;
    root["aggressiveBlocking"] = m_aggressiveBlocking;
    root["autoUpdateInterval"] = m_autoUpdateInterval;
    
    QJsonArray whitelistArray;
    for (const QString &domain : m_whitelist) {
        whitelistArray.append(domain);
    }
    root["whitelist"] = whitelistArray;
    
    QJsonArray customRulesArray;
    for (const FilterRule &rule : m_customRules) {
        customRulesArray.append(rule.pattern);
    }
    root["customRules"] = customRulesArray;
    
    QJsonArray listsArray;
    for (const auto &pair : m_filterLists.toStdMap()) {
        QJsonObject listObj;
        const FilterList &list = pair.second;
        
        listObj["url"] = list.url;
        listObj["name"] = list.name;
        listObj["enabled"] = list.enabled;
        listObj["version"] = list.version;
        
        listsArray.append(listObj);
    }
    root["filterLists"] = listsArray;
    
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
        return true;
    }
    return false;
}

bool AdBlocker::importSettings(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject root = doc.object();
    
    m_enabled = root["enabled"].toBool(true);
    m_aggressiveBlocking = root["aggressiveBlocking"].toBool(false);
    m_autoUpdateInterval = root["autoUpdateInterval"].toInt(DEFAULT_UPDATE_INTERVAL);
    
    m_whitelist.clear();
    QJsonArray whitelistArray = root["whitelist"].toArray();
    for (const QJsonValue &val : whitelistArray) {
        m_whitelist.append(val.toString());
    }
    
    m_customRules.clear();
    QJsonArray customRulesArray = root["customRules"].toArray();
    for (const QJsonValue &val : customRulesArray) {
        FilterRule rule;
        if (parseRule(val.toString(), rule)) {
            m_customRules.append(rule);
        }
    }
    
    QJsonArray listsArray = root["filterLists"].toArray();
    for (const QJsonValue &val : listsArray) {
        QJsonObject listObj = val.toObject();
        QString url = listObj["url"].toString();
        
        if (!url.isEmpty()) {
            addFilterList(url, listObj["enabled"].toBool(true));
        }
    }
    
    emit settingsChanged();
    saveSettings();
    return true;
}

bool AdBlocker::exportStatistics(const QString &path) const
{
    QJsonObject root;
    
    root["totalBlocked"] = m_totalBlocked;
    
    QJsonObject domainsObj;
    for (auto it = m_blockedDomains.begin(); it != m_blockedDomains.end(); ++it) {
        domainsObj[it.key()] = it.value();
    }
    root["blockedDomains"] = domainsObj;
    
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
        return true;
    }
    return false;
}

void AdBlocker::handleAutoUpdate()
{
    if (m_enabled) {
        updateAllFilterLists();
    }
}

void AdBlocker::checkForUpdates()
{
    if (!m_enabled) return;
    
    QDateTime now = QDateTime::currentDateTime();
    for (auto &list : m_filterLists) {
        if (list.enabled && !list.url.isEmpty()) {
            if (list.lastUpdate.addSecs(m_autoUpdateInterval * 3600) < now) {
                updateFilterList(list.url);
            }
        }
    }
}

void AdBlocker::cleanupExpiredRules()
{
    const int RULE_EXPIRATION_DAYS = 30;
    QDateTime expirationDate = QDateTime::currentDateTime().addDays(-RULE_EXPIRATION_DAYS);
    
    // Очистка неиспользуемых правил
    for (auto it = m_filterLists.begin(); it != m_filterLists.end(); ++it) {
        QList<FilterRule> &rules = it.value().rules;
        for (int i = rules.size() - 1; i >= 0; --i) {
            if (!rules[i].enabled && rules[i].hitCount == 0 &&
                rules[i].creationDate < expirationDate) {
                rules.removeAt(i);
            }
        }
    }
    
    // Очистка пользовательских правил
    for (int i = m_customRules.size() - 1; i >= 0; --i) {
        if (!m_customRules[i].enabled && m_customRules[i].hitCount == 0 &&
            m_customRules[i].creationDate < expirationDate) {
            m_customRules.removeAt(i);
        }
    }
    
    saveSettings();
}

QMap<QString, FilterList> AdBlocker::getFilterLists() const
{
    return m_filterLists;
}

QHash<QString, int> AdBlocker::getDomainStats() const
{
    QHash<QString, int> stats;
    for (const auto &list : m_filterLists) {
        for (const auto &rule : list.rules) {
            if (rule.hitCount > 0) {
                for (const QString &domain : rule.domains) {
                    stats[domain] += rule.hitCount;
                }
            }
        }
    }
    return stats;
}

void AdBlocker::resetStats()
{
    m_totalBlocked = 0;
    for (auto &list : m_filterLists) {
        for (auto &rule : list.rules) {
            rule.hitCount = 0;
            rule.lastHit = QDateTime();
        }
    }
    for (auto &rule : m_customRules) {
        rule.hitCount = 0;
        rule.lastHit = QDateTime();
    }
    saveSettings();
    emit statsUpdated();
} 
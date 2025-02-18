#include "privacymanager.h"
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QWebEngineScript>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

const QString PrivacyManager::SETTINGS_FILE = "privacy_settings.json";

PrivacyManager::PrivacyManager(QWebEngineProfile *profile, QObject *parent)
    : QObject(parent)
    , m_profile(profile)
    , m_cookiePolicy("accept")
    , m_doNotTrack(false)
    , m_trackingProtection(true)
    , m_httpsOnly(false)
    , m_javascriptEnabled(true)
    , m_webRTCPolicy("default")
    , m_incognitoMode(false)
    , m_proxyEnabled(false)
    , m_proxyPort(0)
    , m_storageMonitorTimer(nullptr)
{
    initializeProfile();
    loadSettings();
    
    // Подключаем сигналы
    connect(m_profile->cookieStore(), &QWebEngineCookieStore::cookieAdded,
            this, &PrivacyManager::handleCookieAdded);
    connect(m_profile->cookieStore(), &QWebEngineCookieStore::cookieRemoved,
            this, &PrivacyManager::handleCookieRemoved);
}

PrivacyManager::~PrivacyManager()
{
    saveSettings();
}

void PrivacyManager::initializeProfile()
{
    // Настройка профиля
    QWebEngineSettings *settings = m_profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, m_javascriptEnabled);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,
                         m_webRTCPolicy == "disable-non-proxied-udp");
    
    // Настройка cookie store
    QWebEngineCookieStore *cookieStore = m_profile->cookieStore();
    cookieStore->setCookieFilter([this](const QWebEngineCookieStore::FilterRequest &request) {
        QString domain = request.origin.host();
        
        // Проверяем исключения
        if (m_cookieExceptions.contains(domain)) {
            return m_cookieExceptions[domain];
        }
        
        // Применяем политику
        if (m_cookiePolicy == "reject") {
            return false;
        } else if (m_cookiePolicy == "reject-third-party") {
            return request.thirdParty ? false : true;
        }
        
        return true;
    });
}

void PrivacyManager::loadSettings()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + SETTINGS_FILE);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загрузка основных настроек
            m_cookiePolicy = root["cookiePolicy"].toString("accept");
            m_doNotTrack = root["doNotTrack"].toBool(false);
            m_trackingProtection = root["trackingProtection"].toBool(true);
            m_httpsOnly = root["httpsOnly"].toBool(false);
            m_javascriptEnabled = root["javascriptEnabled"].toBool(true);
            m_webRTCPolicy = root["webRTCPolicy"].toString("default");
            
            // Загрузка исключений для cookies
            QJsonObject cookieExceptions = root["cookieExceptions"].toObject();
            for (auto it = cookieExceptions.begin(); it != cookieExceptions.end(); ++it) {
                m_cookieExceptions[it.key()] = it.value().toBool();
            }
            
            // Загрузка исключений для tracking protection
            QJsonArray trackingExceptions = root["trackingExceptions"].toArray();
            for (const QJsonValue &val : trackingExceptions) {
                m_trackingExceptions.append(val.toString());
            }
            
            // Загрузка разрешений
            QJsonObject permissions = root["permissions"].toObject();
            for (auto it = permissions.begin(); it != permissions.end(); ++it) {
                QJsonObject originPerms = it.value().toObject();
                for (auto pit = originPerms.begin(); pit != originPerms.end(); ++pit) {
                    m_permissions[it.key()][pit.key()] = pit.value().toBool();
                }
            }
            
            // Загрузка квот хранилища
            QJsonObject quotas = root["storageQuotas"].toObject();
            for (auto it = quotas.begin(); it != quotas.end(); ++it) {
                m_storageQuotas[it.key()] = it.value().toVariant().toLongLong();
            }
            
            // Загрузка настроек прокси
            m_proxyEnabled = root["proxyEnabled"].toBool(false);
            m_proxyHost = root["proxyHost"].toString();
            m_proxyPort = root["proxyPort"].toInt(0);
            m_proxyUsername = root["proxyUsername"].toString();
            m_proxyPassword = root["proxyPassword"].toString();
            
            QJsonArray bypassList = root["proxyBypassList"].toArray();
            for (const QJsonValue &val : bypassList) {
                m_proxyBypassList.append(val.toString());
            }
            
            // Загрузка исключений для сертификатов
            QJsonArray certExceptions = root["certificateExceptions"].toArray();
            for (const QJsonValue &val : certExceptions) {
                m_certificateExceptions.append(val.toString());
            }
        }
    }
    
    // Применяем загруженные настройки
    updateCookiePolicy();
    updateTrackingProtection();
    updatePermissions();
    updateProxySettings();
}

void PrivacyManager::saveSettings()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + SETTINGS_FILE);
    
    QJsonObject root;
    
    // Сохранение основных настроек
    root["cookiePolicy"] = m_cookiePolicy;
    root["doNotTrack"] = m_doNotTrack;
    root["trackingProtection"] = m_trackingProtection;
    root["httpsOnly"] = m_httpsOnly;
    root["javascriptEnabled"] = m_javascriptEnabled;
    root["webRTCPolicy"] = m_webRTCPolicy;
    
    // Сохранение исключений для cookies
    QJsonObject cookieExceptions;
    for (auto it = m_cookieExceptions.begin(); it != m_cookieExceptions.end(); ++it) {
        cookieExceptions[it.key()] = it.value();
    }
    root["cookieExceptions"] = cookieExceptions;
    
    // Сохранение исключений для tracking protection
    QJsonArray trackingExceptions;
    for (const QString &domain : m_trackingExceptions) {
        trackingExceptions.append(domain);
    }
    root["trackingExceptions"] = trackingExceptions;
    
    // Сохранение разрешений
    QJsonObject permissions;
    for (auto it = m_permissions.begin(); it != m_permissions.end(); ++it) {
        QJsonObject originPerms;
        for (auto pit = it.value().begin(); pit != it.value().end(); ++pit) {
            originPerms[pit.key()] = pit.value();
        }
        permissions[it.key()] = originPerms;
    }
    root["permissions"] = permissions;
    
    // Сохранение квот хранилища
    QJsonObject quotas;
    for (auto it = m_storageQuotas.begin(); it != m_storageQuotas.end(); ++it) {
        quotas[it.key()] = QJsonValue::fromVariant(it.value());
    }
    root["storageQuotas"] = quotas;
    
    // Сохранение настроек прокси
    root["proxyEnabled"] = m_proxyEnabled;
    root["proxyHost"] = m_proxyHost;
    root["proxyPort"] = m_proxyPort;
    root["proxyUsername"] = m_proxyUsername;
    root["proxyPassword"] = m_proxyPassword;
    
    QJsonArray bypassList;
    for (const QString &domain : m_proxyBypassList) {
        bypassList.append(domain);
    }
    root["proxyBypassList"] = bypassList;
    
    // Сохранение исключений для сертификатов
    QJsonArray certExceptions;
    for (const QString &host : m_certificateExceptions) {
        certExceptions.append(host);
    }
    root["certificateExceptions"] = certExceptions;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void PrivacyManager::setCookiePolicy(const QString &policy)
{
    if (m_cookiePolicy != policy) {
        m_cookiePolicy = policy;
        updateCookiePolicy();
        emit cookiePolicyChanged(policy);
        saveSettings();
    }
}

QString PrivacyManager::getCookiePolicy() const
{
    return m_cookiePolicy;
}

void PrivacyManager::clearCookies()
{
    m_profile->cookieStore()->deleteAllCookies();
    emit dataCleared("cookies");
}

void PrivacyManager::addCookieException(const QString &domain, bool allow)
{
    m_cookieExceptions[domain] = allow;
    emit cookieExceptionAdded(domain, allow);
    saveSettings();
}

void PrivacyManager::removeCookieException(const QString &domain)
{
    if (m_cookieExceptions.remove(domain) > 0) {
        emit cookieExceptionRemoved(domain);
        saveSettings();
    }
}

QStringList PrivacyManager::getCookieExceptions() const
{
    return m_cookieExceptions.keys();
}

void PrivacyManager::clearBrowsingData(const QDateTime &start, const QDateTime &end)
{
    m_profile->clearHttpCache();
    m_profile->clearAllVisitedLinks();
    
    // Очистка истории
    emit dataCleared("browsing-data");
}

void PrivacyManager::clearCache()
{
    m_profile->clearHttpCache();
    emit dataCleared("cache");
}

void PrivacyManager::clearHistory()
{
    m_profile->clearAllVisitedLinks();
    emit dataCleared("history");
}

void PrivacyManager::clearDownloads()
{
    // Очистка загрузок
    emit dataCleared("downloads");
}

void PrivacyManager::clearPasswords()
{
    // Очистка паролей
    emit dataCleared("passwords");
}

void PrivacyManager::clearFormData()
{
    // Очистка данных форм
    emit dataCleared("form-data");
}

void PrivacyManager::clearLocalStorage()
{
    // Очистка локального хранилища
    emit dataCleared("local-storage");
}

void PrivacyManager::setDoNotTrack(bool enable)
{
    if (m_doNotTrack != enable) {
        m_doNotTrack = enable;
        m_profile->setHttpUserAgent(m_profile->httpUserAgent() + 
                                  (enable ? " DNT:1" : ""));
        emit doNotTrackChanged(enable);
        saveSettings();
    }
}

bool PrivacyManager::isDoNotTrackEnabled() const
{
    return m_doNotTrack;
}

void PrivacyManager::setTrackingProtection(bool enable)
{
    if (m_trackingProtection != enable) {
        m_trackingProtection = enable;
        updateTrackingProtection();
        emit trackingProtectionChanged(enable);
        saveSettings();
    }
}

bool PrivacyManager::isTrackingProtectionEnabled() const
{
    return m_trackingProtection;
}

void PrivacyManager::addTrackingException(const QString &domain)
{
    if (!m_trackingExceptions.contains(domain)) {
        m_trackingExceptions.append(domain);
        updateTrackingProtection();
        emit trackingExceptionAdded(domain);
        saveSettings();
    }
}

void PrivacyManager::removeTrackingException(const QString &domain)
{
    if (m_trackingExceptions.removeOne(domain)) {
        updateTrackingProtection();
        emit trackingExceptionRemoved(domain);
        saveSettings();
    }
}

QStringList PrivacyManager::getTrackingExceptions() const
{
    return m_trackingExceptions;
}

void PrivacyManager::setHttpsOnly(bool enable)
{
    if (m_httpsOnly != enable) {
        m_httpsOnly = enable;
        emit httpsOnlyChanged(enable);
        saveSettings();
    }
}

bool PrivacyManager::isHttpsOnlyEnabled() const
{
    return m_httpsOnly;
}

void PrivacyManager::setJavaScriptEnabled(bool enable)
{
    if (m_javascriptEnabled != enable) {
        m_javascriptEnabled = enable;
        m_profile->settings()->setAttribute(
            QWebEngineSettings::JavascriptEnabled, enable);
        emit javaScriptEnabledChanged(enable);
        saveSettings();
    }
}

bool PrivacyManager::isJavaScriptEnabled() const
{
    return m_javascriptEnabled;
}

void PrivacyManager::setWebRTCPolicy(const QString &policy)
{
    if (m_webRTCPolicy != policy) {
        m_webRTCPolicy = policy;
        m_profile->settings()->setAttribute(
            QWebEngineSettings::WebRTCPublicInterfacesOnly,
            policy == "disable-non-proxied-udp");
        emit webRTCPolicyChanged(policy);
        saveSettings();
    }
}

QString PrivacyManager::getWebRTCPolicy() const
{
    return m_webRTCPolicy;
}

void PrivacyManager::setPermission(const QString &origin, const QString &feature, bool allow)
{
    QString sanitizedOrigin = sanitizeOrigin(origin);
    m_permissions[sanitizedOrigin][feature] = allow;
    emit permissionChanged(sanitizedOrigin, feature, allow);
    saveSettings();
}

bool PrivacyManager::getPermission(const QString &origin, const QString &feature) const
{
    QString sanitizedOrigin = sanitizeOrigin(origin);
    return m_permissions.value(sanitizedOrigin).value(feature, false);
}

void PrivacyManager::clearPermissions()
{
    m_permissions.clear();
    saveSettings();
}

QHash<QString, bool> PrivacyManager::getPermissionsForOrigin(const QString &origin) const
{
    return m_permissions.value(sanitizeOrigin(origin));
}

void PrivacyManager::setStorageQuota(const QString &origin, qint64 quota)
{
    m_storageQuotas[sanitizeOrigin(origin)] = quota;
    emit storageQuotaChanged(origin, quota);
    saveSettings();
}

qint64 PrivacyManager::getStorageQuota(const QString &origin) const
{
    return m_storageQuotas.value(sanitizeOrigin(origin), DEFAULT_STORAGE_QUOTA);
}

qint64 PrivacyManager::getStorageUsage(const QString &origin) const
{
    QString sanitizedOrigin = sanitizeOrigin(origin);
    QWebEngineProfile *profile = m_incognitoMode ? m_incognitoProfile : m_defaultProfile;
    
    // Get the storage path for the origin
    QString storagePath = profile->persistentStoragePath() + "/" + sanitizedOrigin;
    
    // Calculate total storage usage
    qint64 totalSize = 0;
    QDir storageDir(storagePath);
    
    if (storageDir.exists()) {
        // Calculate size of all files in the storage directory
        QFileInfoList entries = storageDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            if (entry.isFile()) {
                totalSize += entry.size();
            } else if (entry.isDir()) {
                QDirIterator it(entry.filePath(), QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    it.next();
                    totalSize += it.fileInfo().size();
                }
            }
        }
    }
    
    return totalSize;
}

void PrivacyManager::clearStorageData(const QString &origin)
{
    QString sanitizedOrigin = sanitizeOrigin(origin);
    QWebEngineProfile *profile = m_incognitoMode ? m_incognitoProfile : m_defaultProfile;
    
    // Clear various types of storage data
    profile->clearHttpCache();
    
    // Clear local storage for the specific origin
    QString storagePath = profile->persistentStoragePath() + "/" + sanitizedOrigin;
    QDir storageDir(storagePath);
    
    if (storageDir.exists()) {
        // Remove all files and subdirectories
        storageDir.removeRecursively();
    }
    
    // Clear other storage types
    QWebEngineScript clearScript;
    clearScript.setSourceCode(QString(
        "try {"
        "    localStorage.clear();"
        "    sessionStorage.clear();"
        "    indexedDB.deleteDatabase('*');"
        "    caches.keys().then(keys => keys.forEach(key => caches.delete(key)));"
        "} catch(e) { console.error('Storage clearing error:', e); }"));
    clearScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
    clearScript.setWorldId(QWebEngineScript::MainWorld);
    clearScript.setRunsOnSubFrames(true);
    
    // Add the script to clear storages
    m_scriptCollection.insert(clearScript);
    
    emit storageCleared(origin);
}

void PrivacyManager::setIncognitoMode(bool enable)
{
    if (m_incognitoMode != enable) {
        m_incognitoMode = enable;
        m_profile->setOffTheRecord(enable);
        emit incognitoModeChanged(enable);
    }
}

bool PrivacyManager::isIncognitoModeEnabled() const
{
    return m_incognitoMode;
}

void PrivacyManager::clearIncognitoData()
{
    if (m_incognitoMode) {
        clearBrowsingData(QDateTime(), QDateTime::currentDateTime());
    }
}

void PrivacyManager::setProxyEnabled(bool enable)
{
    if (m_proxyEnabled != enable) {
        m_proxyEnabled = enable;
        updateProxySettings();
        emit proxySettingsChanged();
        saveSettings();
    }
}

bool PrivacyManager::isProxyEnabled() const
{
    return m_proxyEnabled;
}

void PrivacyManager::setProxySettings(const QString &host, int port,
                                    const QString &username, const QString &password)
{
    m_proxyHost = host;
    m_proxyPort = port;
    m_proxyUsername = username;
    m_proxyPassword = password;
    
    if (m_proxyEnabled) {
        updateProxySettings();
    }
    
    emit proxySettingsChanged();
    saveSettings();
}

void PrivacyManager::setProxyBypassList(const QStringList &domains)
{
    m_proxyBypassList = domains;
    if (m_proxyEnabled) {
        updateProxySettings();
    }
    saveSettings();
}

QStringList PrivacyManager::getProxyBypassList() const
{
    return m_proxyBypassList;
}

void PrivacyManager::setCustomCertificates(const QList<QSslCertificate> &certificates)
{
    m_customCertificates = certificates;
    // TODO: Применить сертификаты
    saveSettings();
}

QList<QSslCertificate> PrivacyManager::getCustomCertificates() const
{
    return m_customCertificates;
}

void PrivacyManager::addCertificateException(const QString &host)
{
    if (!m_certificateExceptions.contains(host)) {
        m_certificateExceptions.append(host);
        emit certificateExceptionAdded(host);
        saveSettings();
    }
}

void PrivacyManager::removeCertificateException(const QString &host)
{
    if (m_certificateExceptions.removeOne(host)) {
        emit certificateExceptionRemoved(host);
        saveSettings();
    }
}

QStringList PrivacyManager::getCertificateExceptions() const
{
    return m_certificateExceptions;
}

void PrivacyManager::handleCookieAdded(const QNetworkCookie &cookie)
{
    QString domain = cookie.domain();
    if (domain.startsWith(".")) {
        domain = domain.mid(1);
    }
    
    if (m_trackingProtection && !m_trackingExceptions.contains(domain)) {
        // Проверяем на трекинг
        if (isTrackingCookie(cookie)) {
            emit privacyBreach("Tracking cookie detected: " + domain);
        }
    }
}

void PrivacyManager::handleCookieRemoved(const QNetworkCookie &cookie)
{
    // Обработка удаления cookie
}

void PrivacyManager::handleStorageCleared()
{
    emit dataCleared("storage");
}

void PrivacyManager::handlePermissionRequest(const QUrl &origin, QWebEnginePage::Feature feature)
{
    QString originStr = origin.toString();
    QString featureStr;
    
    switch (feature) {
        case QWebEnginePage::Geolocation:
            featureStr = "geolocation";
            break;
        case QWebEnginePage::MediaAudioCapture:
            featureStr = "microphone";
            break;
        case QWebEnginePage::MediaVideoCapture:
            featureStr = "camera";
            break;
        case QWebEnginePage::MediaAudioVideoCapture:
            featureStr = "camera-microphone";
            break;
        case QWebEnginePage::Notifications:
            featureStr = "notifications";
            break;
        default:
            featureStr = "unknown";
    }
    
    // Проверяем разрешения
    bool allowed = getPermission(originStr, featureStr);
    emit permissionChanged(originStr, featureStr, allowed);
}

void PrivacyManager::handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator)
{
    if (m_proxyEnabled && !m_proxyUsername.isEmpty()) {
        authenticator->setUser(m_proxyUsername);
        authenticator->setPassword(m_proxyPassword);
    }
}

void PrivacyManager::handleCertificateError(const QWebEngineCertificateError &error)
{
    QString host = error.url().host();
    
    if (!m_certificateExceptions.contains(host)) {
        emit securityWarning("Certificate error for " + host + ": " + error.errorDescription());
    }
}

void PrivacyManager::updateCookiePolicy()
{
    QWebEngineCookieStore *cookieStore = m_profile->cookieStore();
    
    if (m_cookiePolicy == "reject") {
        cookieStore->setCookieFilter([](const QWebEngineCookieStore::FilterRequest &) {
            return false;
        });
    } else if (m_cookiePolicy == "reject-third-party") {
        cookieStore->setCookieFilter([](const QWebEngineCookieStore::FilterRequest &request) {
            return !request.thirdParty;
        });
    } else {
        cookieStore->setCookieFilter(nullptr);
    }
}

void PrivacyManager::updateTrackingProtection()
{
    if (!m_trackingProtectionEnabled) {
        return;
    }

    // Download latest tracking protection lists
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    // Disconnect Point Lists (known tracking domains)
    QNetworkRequest request(QUrl("https://disconnect.me/trackerprotection/blocked"));
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                m_trackingDomains.clear();
                
                // Parse and store tracking domains
                for (const QString &category : root.keys()) {
                    QJsonObject categoryObj = root[category].toObject();
                    for (const QString &service : categoryObj.keys()) {
                        QJsonArray domains = categoryObj[service].toArray();
                        for (const QJsonValue &domain : domains) {
                            m_trackingDomains.insert(domain.toString());
                        }
                    }
                }
                
                // Save to file
                saveTrackingProtectionRules();
                emit trackingProtectionUpdated();
            }
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void PrivacyManager::updatePermissions()
{
    QHash<QString, QHash<QString, bool>> newPermissions;
    
    // Scan all profiles for permissions
    QList<QWebEngineProfile*> profiles = {m_defaultProfile};
    if (m_incognitoProfile) {
        profiles.append(m_incognitoProfile);
    }
    
    for (QWebEngineProfile *profile : profiles) {
        // Get all content settings
        QWebEngineSettings *settings = profile->settings();
        
        // Scan known features
        QStringList features = {"geolocation", "microphone", "camera", "camera-microphone", 
                              "notifications", "javascript", "plugins", "fullscreen"};
        
        for (const QString &feature : features) {
            QStringList origins = getOriginsWithPermission(feature);
            for (const QString &origin : origins) {
                bool allowed = isPermissionAllowed(origin, feature);
                newPermissions[origin][feature] = allowed;
            }
        }
    }
    
    // Update stored permissions
    m_permissions = newPermissions;
    
    // Save to settings
    savePermissions();
    emit permissionsUpdated();
}

void PrivacyManager::monitorStorageUsage()
{
    if (!m_storageMonitorTimer) {
        m_storageMonitorTimer = new QTimer(this);
        connect(m_storageMonitorTimer, &QTimer::timeout, this, [this]() {
            QHash<QString, qint64> currentUsage;
            
            // Check storage usage for all known origins
            for (const QString &origin : m_storageQuotas.keys()) {
                qint64 usage = getStorageUsage(origin);
                currentUsage[origin] = usage;
                
                // Check if usage exceeds quota
                qint64 quota = getStorageQuota(origin);
                if (usage > quota) {
                    emit storageQuotaExceeded(origin, usage, quota);
                }
                
                // Check if usage changed significantly (more than 10%)
                if (m_lastStorageUsage.contains(origin)) {
                    qint64 lastUsage = m_lastStorageUsage[origin];
                    double change = qAbs(usage - lastUsage) / (double)lastUsage;
                    if (change > 0.1) {
                        emit storageUsageChanged(origin, usage, lastUsage);
                    }
                }
            }
            
            m_lastStorageUsage = currentUsage;
        });
        
        // Monitor every 5 minutes
        m_storageMonitorTimer->start(5 * 60 * 1000);
    }
}

void PrivacyManager::logPrivacyEvent(const QString &eventType, const QString &details)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    
    // Create log entry
    QJsonObject entry;
    entry["timestamp"] = timestamp.toString(Qt::ISODate);
    entry["type"] = eventType;
    entry["details"] = details;
    
    // Add to in-memory log
    m_privacyLog.append(entry);
    
    // Trim log if it gets too large (keep last 1000 entries)
    while (m_privacyLog.size() > 1000) {
        m_privacyLog.removeFirst();
    }
    
    // Save to file
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                     + "/privacy_log.json";
    
    QJsonDocument doc(QJsonArray::fromVariantList(m_privacyLog));
    QFile file(logPath);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
    
    emit privacyEventLogged(eventType, details, timestamp);
}

bool PrivacyManager::isTrackingCookie(const QNetworkCookie &cookie) const
{
    // Проверяем известные трекинговые cookie
    static const QStringList trackingNames = {
        "_ga", "_gid", "__utma", "__utmb", "__utmc", "__utmz",
        "fbp", "_fbp", "_fbc",
        "_ym_uid", "_ym_d",
        "NID", "APISID", "SSID", "SID",
        "_gcl_au"
    };
    
    return trackingNames.contains(cookie.name());
}

QString PrivacyManager::sanitizeOrigin(const QString &origin) const
{
    QUrl url(origin);
    return url.scheme() + "://" + url.host() + 
           (url.port() != -1 ? ":" + QString::number(url.port()) : "");
} 
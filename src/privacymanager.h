#ifndef PRIVACYMANAGER_H
#define PRIVACYMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QHash>
#include <QVariant>

class PrivacyManager : public QObject
{
    Q_OBJECT

public:
    explicit PrivacyManager(QWebEngineProfile *profile, QObject *parent = nullptr);
    ~PrivacyManager();

    // Управление cookies
    void setCookiePolicy(const QString &policy); // "accept", "reject", "reject-third-party"
    QString getCookiePolicy() const;
    void clearCookies();
    void addCookieException(const QString &domain, bool allow);
    void removeCookieException(const QString &domain);
    QStringList getCookieExceptions() const;
    
    // Управление данными
    void clearBrowsingData(const QDateTime &start, const QDateTime &end);
    void clearCache();
    void clearHistory();
    void clearDownloads();
    void clearPasswords();
    void clearFormData();
    void clearLocalStorage();
    
    // Отслеживание
    void setDoNotTrack(bool enable);
    bool isDoNotTrackEnabled() const;
    void setTrackingProtection(bool enable);
    bool isTrackingProtectionEnabled() const;
    void addTrackingException(const QString &domain);
    void removeTrackingException(const QString &domain);
    QStringList getTrackingExceptions() const;
    
    // Безопасность
    void setHttpsOnly(bool enable);
    bool isHttpsOnlyEnabled() const;
    void setJavaScriptEnabled(bool enable);
    bool isJavaScriptEnabled() const;
    void setWebRTCPolicy(const QString &policy); // "default", "disable-non-proxied-udp"
    QString getWebRTCPolicy() const;
    
    // Разрешения
    void setPermission(const QString &origin, const QString &feature, bool allow);
    bool getPermission(const QString &origin, const QString &feature) const;
    void clearPermissions();
    QHash<QString, bool> getPermissionsForOrigin(const QString &origin) const;
    
    // Управление локальным хранилищем
    void setStorageQuota(const QString &origin, qint64 quota);
    qint64 getStorageQuota(const QString &origin) const;
    qint64 getStorageUsage(const QString &origin) const;
    void clearStorageData(const QString &origin);
    
    // Режим инкогнито
    void setIncognitoMode(bool enable);
    bool isIncognitoModeEnabled() const;
    void clearIncognitoData();
    
    // Настройки прокси
    void setProxyEnabled(bool enable);
    bool isProxyEnabled() const;
    void setProxySettings(const QString &host, int port, 
                         const QString &username = QString(),
                         const QString &password = QString());
    void setProxyBypassList(const QStringList &domains);
    QStringList getProxyBypassList() const;
    
    // Сертификаты
    void setCustomCertificates(const QList<QSslCertificate> &certificates);
    QList<QSslCertificate> getCustomCertificates() const;
    void addCertificateException(const QString &host);
    void removeCertificateException(const QString &host);
    QStringList getCertificateExceptions() const;

signals:
    void cookiePolicyChanged(const QString &policy);
    void cookieExceptionAdded(const QString &domain, bool allow);
    void cookieExceptionRemoved(const QString &domain);
    void dataCleared(const QString &dataType);
    void doNotTrackChanged(bool enabled);
    void trackingProtectionChanged(bool enabled);
    void trackingExceptionAdded(const QString &domain);
    void trackingExceptionRemoved(const QString &domain);
    void httpsOnlyChanged(bool enabled);
    void javaScriptEnabledChanged(bool enabled);
    void webRTCPolicyChanged(const QString &policy);
    void permissionChanged(const QString &origin, const QString &feature, bool allow);
    void storageQuotaChanged(const QString &origin, qint64 quota);
    void storageCleared(const QString &origin);
    void incognitoModeChanged(bool enabled);
    void proxySettingsChanged();
    void certificateExceptionAdded(const QString &host);
    void certificateExceptionRemoved(const QString &host);
    void securityWarning(const QString &message);
    void privacyBreach(const QString &description);

private slots:
    void handleCookieAdded(const QNetworkCookie &cookie);
    void handleCookieRemoved(const QNetworkCookie &cookie);
    void handleStorageCleared();
    void handlePermissionRequest(const QUrl &origin, QWebEnginePage::Feature feature);
    void handleProxyAuthenticationRequired(const QUrl &url, QAuthenticator *authenticator);
    void handleCertificateError(const QWebEngineCertificateError &error);

private:
    void initializeProfile();
    void loadSettings();
    void saveSettings();
    void updateCookiePolicy();
    void updateTrackingProtection();
    void updatePermissions();
    void updateProxySettings();
    void monitorStorageUsage();
    void checkSecuritySettings();
    bool isSecureOrigin(const QString &origin) const;
    QString sanitizeOrigin(const QString &origin) const;
    void logPrivacyEvent(const QString &event, const QString &details);
    
    QWebEngineProfile *m_profile;
    QString m_cookiePolicy;
    QHash<QString, bool> m_cookieExceptions;
    bool m_doNotTrack;
    bool m_trackingProtection;
    QStringList m_trackingExceptions;
    bool m_httpsOnly;
    bool m_javascriptEnabled;
    QString m_webRTCPolicy;
    QHash<QString, QHash<QString, bool>> m_permissions;
    QHash<QString, qint64> m_storageQuotas;
    bool m_incognitoMode;
    bool m_proxyEnabled;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyUsername;
    QString m_proxyPassword;
    QStringList m_proxyBypassList;
    QList<QSslCertificate> m_customCertificates;
    QStringList m_certificateExceptions;
    
    static const QString SETTINGS_FILE;
    static const qint64 DEFAULT_STORAGE_QUOTA = 50 * 1024 * 1024; // 50 MB
};

#endif // PRIVACYMANAGER_H 
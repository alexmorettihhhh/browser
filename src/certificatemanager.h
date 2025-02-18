#ifndef CERTIFICATEMANAGER_H
#define CERTIFICATEMANAGER_H

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QDateTime>
#include <QHash>
#include <QSslError>

struct CertificateInfo {
    QSslCertificate certificate;
    QString nickname;
    QDateTime importDate;
    bool trusted;
    bool autoTrust;
    QStringList trustedDomains;
    QHash<QString, QVariant> metadata;
};

class CertificateManager : public QObject
{
    Q_OBJECT

public:
    explicit CertificateManager(QObject *parent = nullptr);
    ~CertificateManager();

    // Управление сертификатами
    bool importCertificate(const QString &path, const QString &nickname = QString());
    bool removeCertificate(const QString &fingerprint);
    bool trustCertificate(const QString &fingerprint, bool trust = true);
    bool setAutoTrust(const QString &fingerprint, bool autoTrust = true);
    
    // Получение информации
    CertificateInfo getCertificateInfo(const QString &fingerprint) const;
    QList<CertificateInfo> getAllCertificates() const;
    QList<CertificateInfo> getTrustedCertificates() const;
    QList<CertificateInfo> getExpiredCertificates() const;
    QList<CertificateInfo> getSystemCertificates() const;
    
    // Проверка сертификатов
    bool verifyCertificate(const QSslCertificate &certificate) const;
    bool verifyChain(const QList<QSslCertificate> &chain) const;
    QList<QSslError> validateCertificate(const QSslCertificate &certificate) const;
    
    // Управление доверием
    bool addTrustedDomain(const QString &fingerprint, const QString &domain);
    bool removeTrustedDomain(const QString &fingerprint, const QString &domain);
    QStringList getTrustedDomains(const QString &fingerprint) const;
    bool isDomainTrusted(const QString &domain, const QSslCertificate &certificate) const;
    
    // Экспорт/Импорт
    bool exportCertificate(const QString &fingerprint, const QString &path, 
                          const QString &format = "PEM");
    bool exportPrivateKey(const QString &fingerprint, const QString &path, 
                         const QString &password);
    bool importPKCS12(const QString &path, const QString &password);
    
    // Метаданные
    bool setMetadata(const QString &fingerprint, const QString &key, 
                    const QVariant &value);
    QVariant getMetadata(const QString &fingerprint, const QString &key) const;
    
    // Уведомления и мониторинг
    void setExpiryWarningPeriod(int days);
    QList<CertificateInfo> getExpiringCertificates() const;
    bool enableAutoUpdate(bool enable);
    
    // Системные операции
    void refreshSystemCertificates();
    bool installSystemCertificate(const QString &fingerprint);
    bool removeSystemCertificate(const QString &fingerprint);

signals:
    void certificateImported(const QString &fingerprint);
    void certificateRemoved(const QString &fingerprint);
    void certificateTrustChanged(const QString &fingerprint, bool trusted);
    void certificateExpiring(const QString &fingerprint, int daysLeft);
    void certificateError(const QString &fingerprint, const QString &error);
    void domainTrustChanged(const QString &fingerprint, const QString &domain);
    void metadataChanged(const QString &fingerprint, const QString &key);
    void systemCertificatesChanged();

private slots:
    void checkExpiringCertificates();
    void handleSystemCertificateUpdate();
    void validateStoredCertificates();

private:
    bool initializeStore();
    QString calculateFingerprint(const QSslCertificate &certificate) const;
    bool storeCertificate(const QSslCertificate &certificate, const QString &nickname);
    void updateCertificateMetadata(const QString &fingerprint);
    bool validateCertificateFile(const QString &path) const;
    void loadTrustedDomains();
    void saveTrustedDomains();
    void backupStore();
    void restoreStore();
    
    QHash<QString, CertificateInfo> m_certificates;
    QHash<QString, QSslKey> m_privateKeys;
    QHash<QString, QStringList> m_domainTrust;
    int m_expiryWarningDays;
    bool m_autoUpdateEnabled;
    QString m_storePath;
    QDateTime m_lastCheck;
    
    static const QString STORE_FILENAME;
    static const QString BACKUP_FILENAME;
    static const int DEFAULT_EXPIRY_WARNING_DAYS = 30;
};

#endif // CERTIFICATEMANAGER_H 
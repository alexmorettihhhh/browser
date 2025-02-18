#include "certificatemanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QProcess>

const QString CertificateManager::STORE_FILENAME = "certificates.json";
const QString CertificateManager::BACKUP_FILENAME = "certificates.json.bak";

CertificateManager::CertificateManager(QObject *parent)
    : QObject(parent)
    , m_expiryWarningDays(DEFAULT_EXPIRY_WARNING_DAYS)
    , m_autoUpdateEnabled(false)
{
    m_storePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(m_storePath);
    
    if (!initializeStore()) {
        qWarning() << "Failed to initialize certificate store";
    }
    
    loadTrustedDomains();
    
    // Настройка таймера для проверки истекающих сертификатов
    QTimer *expiryTimer = new QTimer(this);
    connect(expiryTimer, &QTimer::timeout, this, &CertificateManager::checkExpiringCertificates);
    expiryTimer->start(24 * 60 * 60 * 1000); // Проверка раз в сутки
    
    // Загрузка системных сертификатов
    refreshSystemCertificates();
}

CertificateManager::~CertificateManager()
{
    saveTrustedDomains();
    backupStore();
}

bool CertificateManager::initializeStore()
{
    QString filePath = m_storePath + "/" + STORE_FILENAME;
    QFile file(filePath);
    
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загрузка сертификатов
            QJsonArray certs = root["certificates"].toArray();
            for (const QJsonValue &val : certs) {
                QJsonObject certObj = val.toObject();
                CertificateInfo info;
                
                QByteArray certData = QByteArray::fromBase64(certObj["data"].toString().toLatin1());
                info.certificate = QSslCertificate(certData, QSsl::Pem);
                
                if (info.certificate.isNull()) {
                    continue;
                }
                
                info.nickname = certObj["nickname"].toString();
                info.importDate = QDateTime::fromString(certObj["importDate"].toString(), Qt::ISODate);
                info.trusted = certObj["trusted"].toBool();
                info.autoTrust = certObj["autoTrust"].toBool();
                
                QJsonObject meta = certObj["metadata"].toObject();
                for (auto it = meta.begin(); it != meta.end(); ++it) {
                    info.metadata.insert(it.key(), it.value().toVariant());
                }
                
                QString fingerprint = calculateFingerprint(info.certificate);
                m_certificates.insert(fingerprint, info);
                
                // Загрузка приватного ключа, если есть
                if (certObj.contains("privateKey")) {
                    QByteArray keyData = QByteArray::fromBase64(certObj["privateKey"].toString().toLatin1());
                    QSslKey key(keyData, QSsl::Rsa);
                    if (!key.isNull()) {
                        m_privateKeys.insert(fingerprint, key);
                    }
                }
            }
            return true;
        }
    }
    return true; // Возвращаем true, если файл не существует (первый запуск)
}

void CertificateManager::backupStore()
{
    QString filePath = m_storePath + "/" + STORE_FILENAME;
    QString backupPath = m_storePath + "/" + BACKUP_FILENAME;
    
    QJsonObject root;
    QJsonArray certs;
    
    for (auto it = m_certificates.begin(); it != m_certificates.end(); ++it) {
        QJsonObject certObj;
        const CertificateInfo &info = it.value();
        
        certObj["data"] = QString(info.certificate.toPem().toBase64());
        certObj["nickname"] = info.nickname;
        certObj["importDate"] = info.importDate.toString(Qt::ISODate);
        certObj["trusted"] = info.trusted;
        certObj["autoTrust"] = info.autoTrust;
        
        QJsonObject meta;
        for (auto mit = info.metadata.begin(); mit != info.metadata.end(); ++mit) {
            meta[mit.key()] = QJsonValue::fromVariant(mit.value());
        }
        certObj["metadata"] = meta;
        
        // Сохранение приватного ключа, если есть
        if (m_privateKeys.contains(it.key())) {
            certObj["privateKey"] = QString(m_privateKeys[it.key()].toPem().toBase64());
        }
        
        certs.append(certObj);
    }
    
    root["certificates"] = certs;
    
    QJsonDocument doc(root);
    QFile file(filePath);
    
    // Создаем резервную копию перед сохранением
    if (QFile::exists(filePath)) {
        QFile::remove(backupPath);
        QFile::copy(filePath, backupPath);
    }
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

bool CertificateManager::importCertificate(const QString &path, const QString &nickname)
{
    if (!validateCertificateFile(path)) {
        emit certificateError(QString(), "Invalid certificate file");
        return false;
    }
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit certificateError(QString(), "Cannot open certificate file");
        return false;
    }
    
    QSslCertificate cert(file.readAll(), QSsl::Pem);
    file.close();
    
    if (cert.isNull()) {
        emit certificateError(QString(), "Invalid certificate data");
        return false;
    }
    
    QString fingerprint = calculateFingerprint(cert);
    
    if (m_certificates.contains(fingerprint)) {
        emit certificateError(fingerprint, "Certificate already exists");
        return false;
    }
    
    if (!storeCertificate(cert, nickname)) {
        emit certificateError(fingerprint, "Failed to store certificate");
        return false;
    }
    
    emit certificateImported(fingerprint);
    return true;
}

bool CertificateManager::removeCertificate(const QString &fingerprint)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    m_certificates.remove(fingerprint);
    m_privateKeys.remove(fingerprint);
    m_domainTrust.remove(fingerprint);
    
    backupStore();
    emit certificateRemoved(fingerprint);
    return true;
}

bool CertificateManager::trustCertificate(const QString &fingerprint, bool trust)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    CertificateInfo &info = m_certificates[fingerprint];
    if (info.trusted != trust) {
        info.trusted = trust;
        emit certificateTrustChanged(fingerprint, trust);
        backupStore();
    }
    
    return true;
}

CertificateInfo CertificateManager::getCertificateInfo(const QString &fingerprint) const
{
    return m_certificates.value(fingerprint);
}

QList<CertificateInfo> CertificateManager::getAllCertificates() const
{
    return m_certificates.values();
}

QList<CertificateInfo> CertificateManager::getTrustedCertificates() const
{
    QList<CertificateInfo> trusted;
    for (const CertificateInfo &info : m_certificates) {
        if (info.trusted) {
            trusted.append(info);
        }
    }
    return trusted;
}

QList<CertificateInfo> CertificateManager::getExpiredCertificates() const
{
    QList<CertificateInfo> expired;
    QDateTime now = QDateTime::currentDateTime();
    
    for (const CertificateInfo &info : m_certificates) {
        if (info.certificate.expiryDate() < now) {
            expired.append(info);
        }
    }
    return expired;
}

bool CertificateManager::verifyCertificate(const QSslCertificate &certificate) const
{
    if (certificate.isNull()) {
        return false;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    return !certificate.isBlacklisted() &&
           certificate.effectiveDate() <= now &&
           certificate.expiryDate() >= now;
}

QList<QSslError> CertificateManager::validateCertificate(const QSslCertificate &certificate) const
{
    QList<QSslError> errors;
    
    if (certificate.isNull()) {
        errors.append(QSslError(QSslError::UnableToGetIssuerCertificate));
        return errors;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    
    if (certificate.isBlacklisted()) {
        errors.append(QSslError(QSslError::CertificateBlacklisted));
    }
    
    if (certificate.effectiveDate() > now) {
        errors.append(QSslError(QSslError::CertificateNotYetValid));
    }
    
    if (certificate.expiryDate() < now) {
        errors.append(QSslError(QSslError::CertificateExpired));
    }
    
    // Проверка цепочки доверия
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> caCerts = sslConfig.caCertificates();
    
    bool foundIssuer = false;
    for (const QSslCertificate &caCert : caCerts) {
        if (certificate.issuerDisplayName() == caCert.subjectDisplayName()) {
            foundIssuer = true;
            break;
        }
    }
    
    if (!foundIssuer) {
        errors.append(QSslError(QSslError::UnableToGetIssuerCertificate));
    }
    
    return errors;
}

bool CertificateManager::addTrustedDomain(const QString &fingerprint, const QString &domain)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    QStringList &domains = m_domainTrust[fingerprint];
    if (!domains.contains(domain)) {
        domains.append(domain);
        emit domainTrustChanged(fingerprint, domain);
        saveTrustedDomains();
        return true;
    }
    return false;
}

bool CertificateManager::removeTrustedDomain(const QString &fingerprint, const QString &domain)
{
    if (!m_domainTrust.contains(fingerprint)) {
        return false;
    }
    
    QStringList &domains = m_domainTrust[fingerprint];
    if (domains.removeOne(domain)) {
        emit domainTrustChanged(fingerprint, domain);
        saveTrustedDomains();
        return true;
    }
    return false;
}

QStringList CertificateManager::getTrustedDomains(const QString &fingerprint) const
{
    return m_domainTrust.value(fingerprint);
}

bool CertificateManager::isDomainTrusted(const QString &domain, const QSslCertificate &certificate) const
{
    QString fingerprint = calculateFingerprint(certificate);
    
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    const CertificateInfo &info = m_certificates[fingerprint];
    if (!info.trusted) {
        return false;
    }
    
    if (info.autoTrust) {
        return true;
    }
    
    const QStringList &domains = m_domainTrust.value(fingerprint);
    return domains.contains(domain, Qt::CaseInsensitive);
}

bool CertificateManager::exportCertificate(const QString &fingerprint, const QString &path,
                                         const QString &format)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    const CertificateInfo &info = m_certificates[fingerprint];
    QByteArray data;
    
    if (format.toUpper() == "PEM") {
        data = info.certificate.toPem();
    } else if (format.toUpper() == "DER") {
        data = info.certificate.toDer();
    } else {
        return false;
    }
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(data);
    file.close();
    return true;
}

bool CertificateManager::exportPrivateKey(const QString &fingerprint, const QString &path,
                                        const QString &password)
{
    if (!m_privateKeys.contains(fingerprint)) {
        return false;
    }
    
    const QSslKey &key = m_privateKeys[fingerprint];
    QByteArray data = key.toPem(password.toUtf8());
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(data);
    file.close();
    return true;
}

bool CertificateManager::importPKCS12(const QString &path, const QString &password)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QSslKey key;
    QSslCertificate cert;
    QList<QSslCertificate> caCerts;
    
    if (!QSslCertificate::importPkcs12(data, &key, &cert, &caCerts, password.toUtf8())) {
        return false;
    }
    
    QString fingerprint = calculateFingerprint(cert);
    
    // Сохраняем сертификат
    CertificateInfo info;
    info.certificate = cert;
    info.importDate = QDateTime::currentDateTime();
    info.trusted = false;
    info.autoTrust = false;
    
    m_certificates[fingerprint] = info;
    
    // Сохраняем приватный ключ
    if (!key.isNull()) {
        m_privateKeys[fingerprint] = key;
    }
    
    // Сохраняем CA сертификаты
    for (const QSslCertificate &caCert : caCerts) {
        QString caFingerprint = calculateFingerprint(caCert);
        if (!m_certificates.contains(caFingerprint)) {
            CertificateInfo caInfo;
            caInfo.certificate = caCert;
            caInfo.importDate = QDateTime::currentDateTime();
            caInfo.trusted = false;
            caInfo.autoTrust = false;
            m_certificates[caFingerprint] = caInfo;
        }
    }
    
    backupStore();
    emit certificateImported(fingerprint);
    return true;
}

bool CertificateManager::setMetadata(const QString &fingerprint, const QString &key,
                                   const QVariant &value)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }
    
    CertificateInfo &info = m_certificates[fingerprint];
    info.metadata[key] = value;
    emit metadataChanged(fingerprint, key);
    backupStore();
    return true;
}

QVariant CertificateManager::getMetadata(const QString &fingerprint, const QString &key) const
{
    if (!m_certificates.contains(fingerprint)) {
        return QVariant();
    }
    
    return m_certificates[fingerprint].metadata.value(key);
}

void CertificateManager::setExpiryWarningPeriod(int days)
{
    m_expiryWarningDays = days;
}

QList<CertificateInfo> CertificateManager::getExpiringCertificates() const
{
    QList<CertificateInfo> expiring;
    QDateTime now = QDateTime::currentDateTime();
    QDateTime warningDate = now.addDays(m_expiryWarningDays);
    
    for (const CertificateInfo &info : m_certificates) {
        QDateTime expiryDate = info.certificate.expiryDate();
        if (expiryDate > now && expiryDate <= warningDate) {
            expiring.append(info);
        }
    }
    return expiring;
}

bool CertificateManager::enableAutoUpdate(bool enable)
{
    if (m_autoUpdateEnabled != enable) {
        m_autoUpdateEnabled = enable;
        if (enable) {
            refreshSystemCertificates();
        }
    }
    return true;
}

void CertificateManager::refreshSystemCertificates()
{
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> systemCerts = sslConfig.caCertificates();
    
    for (const QSslCertificate &cert : systemCerts) {
        QString fingerprint = calculateFingerprint(cert);
        if (!m_certificates.contains(fingerprint)) {
            CertificateInfo info;
            info.certificate = cert;
            info.importDate = QDateTime::currentDateTime();
            info.trusted = true;
            info.autoTrust = true;
            m_certificates[fingerprint] = info;
        }
    }
    
    emit systemCertificatesChanged();
}

bool CertificateManager::installSystemCertificate(const QString &fingerprint)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }

    const QSslCertificate &cert = m_certificates[fingerprint];
    QString certData = QString::fromLatin1(cert.toPem());

#ifdef Q_OS_WIN
    // Windows implementation using certutil
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString certFile = tempPath + "/" + fingerprint + ".crt";
    QFile file(certFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(cert.toPem());
    file.close();

    // Run certutil to install the certificate
    QProcess process;
    process.start("certutil", QStringList() << "-addstore" << "ROOT" << certFile);
    bool success = process.waitForFinished() && process.exitCode() == 0;
    QFile::remove(certFile);
    
    if (success) {
        emit systemCertificatesChanged();
    }
    return success;

#elif defined(Q_OS_LINUX)
    // Linux implementation using update-ca-certificates
    QString certPath = "/usr/local/share/ca-certificates/" + fingerprint + ".crt";
    QFile file(certPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(cert.toPem());
    file.close();

    // Update system certificates
    QProcess process;
    process.start("sudo", QStringList() << "update-ca-certificates");
    bool success = process.waitForFinished() && process.exitCode() == 0;
    
    if (success) {
        emit systemCertificatesChanged();
    }
    return success;

#elif defined(Q_OS_MAC)
    // macOS implementation using security tool
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString certFile = tempPath + "/" + fingerprint + ".crt";
    QFile file(certFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(cert.toPem());
    file.close();

    // Add to system keychain
    QProcess process;
    process.start("security", QStringList() 
        << "add-trusted-cert" 
        << "-d" 
        << "-k" << "/Library/Keychains/System.keychain"
        << certFile);
    bool success = process.waitForFinished() && process.exitCode() == 0;
    QFile::remove(certFile);
    
    if (success) {
        emit systemCertificatesChanged();
    }
    return success;

#else
    // Unsupported platform
    return false;
#endif
}

bool CertificateManager::removeSystemCertificate(const QString &fingerprint)
{
    if (!m_certificates.contains(fingerprint)) {
        return false;
    }

#ifdef Q_OS_WIN
    // Windows implementation using certutil
    QProcess process;
    process.start("certutil", QStringList() << "-delstore" << "ROOT" << fingerprint);
    bool success = process.waitForFinished() && process.exitCode() == 0;
    
    if (success) {
        emit systemCertificatesChanged();
    }
    return success;

#elif defined(Q_OS_LINUX)
    // Linux implementation
    QString certPath = "/usr/local/share/ca-certificates/" + fingerprint + ".crt";
    if (QFile::remove(certPath)) {
        QProcess process;
        process.start("sudo", QStringList() << "update-ca-certificates" << "--fresh");
        bool success = process.waitForFinished() && process.exitCode() == 0;
        
        if (success) {
            emit systemCertificatesChanged();
        }
        return success;
    }
    return false;

#elif defined(Q_OS_MAC)
    // macOS implementation
    const QSslCertificate &cert = m_certificates[fingerprint];
    QString commonName = cert.subjectInfo(QSslCertificate::CommonName).join(",");
    
    QProcess process;
    process.start("security", QStringList()
        << "remove-trusted-cert"
        << "-d"
        << "-k" << "/Library/Keychains/System.keychain"
        << commonName);
    bool success = process.waitForFinished() && process.exitCode() == 0;
    
    if (success) {
        emit systemCertificatesChanged();
    }
    return success;

#else
    // Unsupported platform
    return false;
#endif
}

void CertificateManager::checkExpiringCertificates()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime warningDate = now.addDays(m_expiryWarningDays);
    
    for (const auto &pair : m_certificates.toStdMap()) {
        const QString &fingerprint = pair.first;
        const CertificateInfo &info = pair.second;
        
        QDateTime expiryDate = info.certificate.expiryDate();
        if (expiryDate > now && expiryDate <= warningDate) {
            int daysLeft = now.daysTo(expiryDate);
            emit certificateExpiring(fingerprint, daysLeft);
        }
    }
}

void CertificateManager::handleSystemCertificateUpdate()
{
    if (m_autoUpdateEnabled) {
        refreshSystemCertificates();
    }
}

void CertificateManager::validateStoredCertificates()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> toRemove;
    
    for (auto it = m_certificates.begin(); it != m_certificates.end(); ++it) {
        const QString &fingerprint = it.key();
        const CertificateInfo &info = it.value();
        
        if (info.certificate.isNull() || 
            info.certificate.isBlacklisted() ||
            info.certificate.expiryDate() < now) {
            toRemove.append(fingerprint);
        }
    }
    
    for (const QString &fingerprint : toRemove) {
        removeCertificate(fingerprint);
    }
}

QString CertificateManager::calculateFingerprint(const QSslCertificate &certificate) const
{
    return QString(certificate.digest(QCryptographicHash::Sha256).toHex());
}

bool CertificateManager::storeCertificate(const QSslCertificate &certificate, const QString &nickname)
{
    QString fingerprint = calculateFingerprint(certificate);
    
    CertificateInfo info;
    info.certificate = certificate;
    info.nickname = nickname;
    info.importDate = QDateTime::currentDateTime();
    info.trusted = false;
    info.autoTrust = false;
    
    m_certificates[fingerprint] = info;
    backupStore();
    
    return true;
}

void CertificateManager::loadTrustedDomains()
{
    QString filePath = m_storePath + "/trusted_domains.json";
    QFile file(filePath);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            for (auto it = root.begin(); it != root.end(); ++it) {
                QJsonArray domains = it.value().toArray();
                QStringList domainList;
                for (const QJsonValue &val : domains) {
                    domainList.append(val.toString());
                }
                m_domainTrust[it.key()] = domainList;
            }
        }
    }
}

void CertificateManager::saveTrustedDomains()
{
    QString filePath = m_storePath + "/trusted_domains.json";
    QJsonObject root;
    
    for (auto it = m_domainTrust.begin(); it != m_domainTrust.end(); ++it) {
        QJsonArray domains;
        for (const QString &domain : it.value()) {
            domains.append(domain);
        }
        root[it.key()] = domains;
    }
    
    QJsonDocument doc(root);
    QFile file(filePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

bool CertificateManager::validateCertificateFile(const QString &path) const
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QSslCertificate cert(data, QSsl::Pem);
    return !cert.isNull();
} 
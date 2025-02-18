#include "passwordmanager.h"
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QDir>
#include <QMessageAuthenticationCode>

const QString PasswordManager::CONFIG_FILE = "passwords.config";
const QString PasswordManager::PASSWORDS_FILE = "passwords.dat";

PasswordManager::PasswordManager(QObject *parent)
    : QObject(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope, "Browser", "Passwords")
{
    loadPasswords();
}

PasswordManager::~PasswordManager()
{
    savePasswords();
}

bool PasswordManager::savePassword(const QString &url, const QString &username, 
                                 const QString &password, const QString &formData)
{
    if (isNeverSave(url) || !validatePassword(password)) {
        return false;
    }

    PasswordEntry entry;
    entry.url = url;
    entry.username = username;
    entry.password = encryptPassword(password);
    entry.formData = formData;
    entry.created = QDateTime::currentDateTime();
    entry.lastUsed = entry.created;
    entry.useCount = 1;
    entry.neverSave = false;

    QString key = url + username;
    QList<PasswordEntry> &entries = m_passwords[key];

    // Проверяем существующие записи
    for (auto &existingEntry : entries) {
        if (existingEntry.username == username) {
            existingEntry = entry;
            emit passwordUpdated(url);
            savePasswords();
            return true;
        }
    }

    entries.append(entry);
    emit passwordAdded(url);
    savePasswords();
    
    // Проверяем силу пароля
    checkPasswordStrength(password);
    
    return true;
}

bool PasswordManager::removePassword(const QString &url, const QString &username)
{
    QString key = url + username;
    if (!m_passwords.contains(key)) {
        return false;
    }

    QList<PasswordEntry> &entries = m_passwords[key];
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i].username == username) {
            entries.removeAt(i);
            if (entries.isEmpty()) {
                m_passwords.remove(key);
            }
            emit passwordRemoved(url);
            savePasswords();
            return true;
        }
    }
    
    return false;
}

bool PasswordManager::updatePassword(const QString &url, const QString &username, 
                                   const QString &newPassword)
{
    if (!validatePassword(newPassword)) {
        return false;
    }

    QString key = url + username;
    if (!m_passwords.contains(key)) {
        return false;
    }

    QList<PasswordEntry> &entries = m_passwords[key];
    for (auto &entry : entries) {
        if (entry.username == username) {
            entry.password = encryptPassword(newPassword);
            entry.lastUsed = QDateTime::currentDateTime();
            entry.useCount++;
            emit passwordUpdated(url);
            savePasswords();
            
            // Проверяем силу нового пароля
            checkPasswordStrength(newPassword);
            
            return true;
        }
    }
    
    return false;
}

QList<PasswordEntry> PasswordManager::getPasswords(const QString &url) const
{
    QList<PasswordEntry> result;
    QString key = url;
    
    if (m_passwords.contains(key)) {
        result = m_passwords[key];
        // Дешифруем пароли перед возвратом
        for (auto &entry : result) {
            entry.password = decryptPassword(entry.password);
        }
    }
    
    return result;
}

QString PasswordManager::findPassword(const QString &url, const QString &username) const
{
    QString key = url + username;
    if (!m_passwords.contains(key)) {
        return QString();
    }

    const QList<PasswordEntry> &entries = m_passwords[key];
    for (const auto &entry : entries) {
        if (entry.username == username) {
            return decryptPassword(entry.password);
        }
    }
    
    return QString();
}

QStringList PasswordManager::findUsernames(const QString &url) const
{
    QStringList usernames;
    QString key = url;
    
    if (m_passwords.contains(key)) {
        const QList<PasswordEntry> &entries = m_passwords[key];
        for (const auto &entry : entries) {
            usernames.append(entry.username);
        }
    }
    
    return usernames;
}

QString PasswordManager::findFormData(const QString &url, const QString &username) const
{
    QString key = url + username;
    if (!m_passwords.contains(key)) {
        return QString();
    }

    const QList<PasswordEntry> &entries = m_passwords[key];
    for (const auto &entry : entries) {
        if (entry.username == username) {
            return entry.formData;
        }
    }
    
    return QString();
}

void PasswordManager::addToNeverSave(const QString &url)
{
    if (!m_neverSaveList.contains(url)) {
        m_neverSaveList.append(url);
        m_settings.setValue("neverSave", m_neverSaveList);
    }
}

void PasswordManager::removeFromNeverSave(const QString &url)
{
    if (m_neverSaveList.removeOne(url)) {
        m_settings.setValue("neverSave", m_neverSaveList);
    }
}

bool PasswordManager::isNeverSave(const QString &url) const
{
    return m_neverSaveList.contains(url);
}

QStringList PasswordManager::getNeverSaveList() const
{
    return m_neverSaveList;
}

bool PasswordManager::exportPasswords(const QString &filename, const QString &masterPassword)
{
    if (!verifyMasterPassword(masterPassword)) {
        return false;
    }

    QJsonArray passwordsArray;
    for (const auto &entries : m_passwords) {
        for (const auto &entry : entries) {
            QJsonObject entryObj;
            entryObj["url"] = entry.url;
            entryObj["username"] = entry.username;
            entryObj["password"] = entry.password;
            entryObj["formData"] = entry.formData;
            entryObj["lastUsed"] = entry.lastUsed.toString(Qt::ISODate);
            entryObj["created"] = entry.created.toString(Qt::ISODate);
            entryObj["useCount"] = entry.useCount;
            passwordsArray.append(entryObj);
        }
    }

    QJsonDocument doc(passwordsArray);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    emit exportCompleted(passwordsArray.size());
    return true;
}

bool PasswordManager::importPasswords(const QString &filename, const QString &masterPassword)
{
    if (!verifyMasterPassword(masterPassword)) {
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return false;
    }

    QJsonArray passwordsArray = doc.array();
    int importCount = 0;

    for (const auto &value : passwordsArray) {
        QJsonObject entryObj = value.toObject();
        PasswordEntry entry;
        entry.url = entryObj["url"].toString();
        entry.username = entryObj["username"].toString();
        entry.password = entryObj["password"].toString();
        entry.formData = entryObj["formData"].toString();
        entry.lastUsed = QDateTime::fromString(entryObj["lastUsed"].toString(), Qt::ISODate);
        entry.created = QDateTime::fromString(entryObj["created"].toString(), Qt::ISODate);
        entry.useCount = entryObj["useCount"].toInt();

        QString key = entry.url + entry.username;
        m_passwords[key].append(entry);
        importCount++;
    }

    savePasswords();
    emit importCompleted(importCount);
    return true;
}

bool PasswordManager::setMasterPassword(const QString &password)
{
    if (!validatePassword(password)) {
        return false;
    }

    m_salt = generateSalt();
    m_masterPasswordHash = hashPassword(password, m_salt);
    m_encryptionKey = hashPassword(password, generateSalt());
    
    m_settings.setValue("salt", m_salt);
    m_settings.setValue("masterPasswordHash", m_masterPasswordHash);
    m_settings.setValue("encryptionKey", m_encryptionKey);
    
    emit masterPasswordChanged();
    return true;
}

bool PasswordManager::changeMasterPassword(const QString &oldPassword, const QString &newPassword)
{
    if (!verifyMasterPassword(oldPassword) || !validatePassword(newPassword)) {
        return false;
    }

    // Перешифровываем все пароли с новым ключом
    QString newSalt = generateSalt();
    QString newEncryptionKey = hashPassword(newPassword, newSalt);
    
    for (auto &entries : m_passwords) {
        for (auto &entry : entries) {
            QString decrypted = decryptPassword(entry.password);
            entry.password = encryptPassword(decrypted);
        }
    }

    m_salt = newSalt;
    m_masterPasswordHash = hashPassword(newPassword, m_salt);
    m_encryptionKey = newEncryptionKey;
    
    m_settings.setValue("salt", m_salt);
    m_settings.setValue("masterPasswordHash", m_masterPasswordHash);
    m_settings.setValue("encryptionKey", m_encryptionKey);
    
    savePasswords();
    emit masterPasswordChanged();
    return true;
}

bool PasswordManager::verifyMasterPassword(const QString &password) const
{
    return hashPassword(password, m_salt) == m_masterPasswordHash;
}

void PasswordManager::clearPasswords()
{
    m_passwords.clear();
    savePasswords();
    emit passwordsCleared();
}

int PasswordManager::getPasswordCount() const
{
    int count = 0;
    for (const auto &entries : m_passwords) {
        count += entries.size();
    }
    return count;
}

QDateTime PasswordManager::getLastSync() const
{
    return m_lastSync;
}

QList<PasswordEntry> PasswordManager::getWeakPasswords() const
{
    QList<PasswordEntry> weakPasswords;
    for (const auto &entries : m_passwords) {
        for (const auto &entry : entries) {
            QString password = decryptPassword(entry.password);
            if (password.length() < 8 || 
                password.toLower() == password ||
                password.toUpper() == password ||
                !password.contains(QRegExp("\\d")) ||
                !password.contains(QRegExp("[!@#$%^&*(),.?\":{}|<>]"))) {
                weakPasswords.append(entry);
            }
        }
    }
    return weakPasswords;
}

QList<PasswordEntry> PasswordManager::getReusedPasswords() const
{
    QHash<QString, QStringList> passwordUsage;
    QList<PasswordEntry> reusedPasswords;

    // Собираем информацию об использовании паролей
    for (const auto &entries : m_passwords) {
        for (const auto &entry : entries) {
            QString password = entry.password;
            passwordUsage[password].append(entry.url);
        }
    }

    // Находим повторно используемые пароли
    for (const auto &entries : m_passwords) {
        for (const auto &entry : entries) {
            if (passwordUsage[entry.password].size() > 1) {
                reusedPasswords.append(entry);
            }
        }
    }

    return reusedPasswords;
}

QList<PasswordEntry> PasswordManager::getOldPasswords() const
{
    const int OLD_PASSWORD_DAYS = 90;
    QList<PasswordEntry> oldPasswords;
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-OLD_PASSWORD_DAYS);

    for (const auto &entries : m_passwords) {
        for (const auto &entry : entries) {
            if (entry.lastUsed < cutoffDate) {
                oldPasswords.append(entry);
            }
        }
    }

    return oldPasswords;
}

QString PasswordManager::encryptPassword(const QString &password) const
{
    // Используем AES шифрование
    QByteArray data = password.toUtf8();
    QByteArray key = m_encryptionKey.toUtf8();
    
    // В реальном приложении здесь должна быть реализация AES шифрования
    // Для примера используем простое XOR шифрование
    QByteArray encrypted;
    for (int i = 0; i < data.size(); ++i) {
        encrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    return encrypted.toBase64();
}

QString PasswordManager::decryptPassword(const QString &encrypted) const
{
    QByteArray data = QByteArray::fromBase64(encrypted.toUtf8());
    QByteArray key = m_encryptionKey.toUtf8();
    
    // В реальном приложении здесь должна быть реализация AES дешифрования
    // Для примера используем простое XOR дешифрование
    QByteArray decrypted;
    for (int i = 0; i < data.size(); ++i) {
        decrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    return QString::fromUtf8(decrypted);
}

QString PasswordManager::generateSalt() const
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString salt;
    for (int i = 0; i < SALT_LENGTH; ++i) {
        salt.append(chars[QRandomGenerator::global()->bounded(chars.length())]);
    }
    return salt;
}

QString PasswordManager::hashPassword(const QString &password, const QString &salt) const
{
    QByteArray data = (password + salt).toUtf8();
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

bool PasswordManager::validatePassword(const QString &password) const
{
    if (password.length() < MIN_PASSWORD_LENGTH || 
        password.length() > MAX_PASSWORD_LENGTH) {
        return false;
    }

    // Проверяем наличие цифр
    if (!password.contains(QRegExp("\\d"))) {
        return false;
    }

    // Проверяем наличие букв в разных регистрах
    if (!password.contains(QRegExp("[a-z]")) || 
        !password.contains(QRegExp("[A-Z]"))) {
        return false;
    }

    // Проверяем наличие специальных символов
    if (!password.contains(QRegExp("[!@#$%^&*(),.?\":{}|<>]"))) {
        return false;
    }

    return true;
}

void PasswordManager::loadPasswords()
{
    m_neverSaveList = m_settings.value("neverSave").toStringList();
    m_salt = m_settings.value("salt").toString();
    m_masterPasswordHash = m_settings.value("masterPasswordHash").toString();
    m_encryptionKey = m_settings.value("encryptionKey").toString();
    m_lastSync = m_settings.value("lastSync").toDateTime();

    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + PASSWORDS_FILE);
    
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        QJsonArray entriesArray = it.value().toArray();
        QList<PasswordEntry> entries;
        
        for (const auto &entryValue : entriesArray) {
            QJsonObject entryObj = entryValue.toObject();
            PasswordEntry entry;
            entry.url = entryObj["url"].toString();
            entry.username = entryObj["username"].toString();
            entry.password = entryObj["password"].toString();
            entry.formData = entryObj["formData"].toString();
            entry.lastUsed = QDateTime::fromString(entryObj["lastUsed"].toString(), Qt::ISODate);
            entry.created = QDateTime::fromString(entryObj["created"].toString(), Qt::ISODate);
            entry.useCount = entryObj["useCount"].toInt();
            entries.append(entry);
        }
        
        m_passwords[it.key()] = entries;
    }
}

void PasswordManager::savePasswords()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + PASSWORDS_FILE);
    
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonObject root;
    for (auto it = m_passwords.begin(); it != m_passwords.end(); ++it) {
        QJsonArray entriesArray;
        for (const auto &entry : it.value()) {
            QJsonObject entryObj;
            entryObj["url"] = entry.url;
            entryObj["username"] = entry.username;
            entryObj["password"] = entry.password;
            entryObj["formData"] = entry.formData;
            entryObj["lastUsed"] = entry.lastUsed.toString(Qt::ISODate);
            entryObj["created"] = entry.created.toString(Qt::ISODate);
            entryObj["useCount"] = entry.useCount;
            entriesArray.append(entryObj);
        }
        root[it.key()] = entriesArray;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    
    m_lastSync = QDateTime::currentDateTime();
    m_settings.setValue("lastSync", m_lastSync);
}

void PasswordManager::checkPasswordStrength(const QString &password)
{
    if (password.length() < 12) {
        emit securityAlert("Рекомендуется использовать пароль длиной не менее 12 символов");
    }
    
    if (!password.contains(QRegExp("[!@#$%^&*(),.?\":{}|<>]"))) {
        emit securityAlert("Рекомендуется использовать специальные символы в пароле");
    }
    
    if (password.toLower() == password || password.toUpper() == password) {
        emit securityAlert("Рекомендуется использовать буквы разных регистров");
    }
    
    if (!password.contains(QRegExp("\\d"))) {
        emit securityAlert("Рекомендуется использовать цифры в пароле");
    }
}

QString PasswordManager::generateStrongPassword(int length) const
{
    const QString lowercase = "abcdefghijklmnopqrstuvwxyz";
    const QString uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const QString numbers = "0123456789";
    const QString special = "!@#$%^&*(),.?\":{}|<>";
    
    QString password;
    
    // Гарантируем наличие всех типов символов
    password += lowercase[QRandomGenerator::global()->bounded(lowercase.length())];
    password += uppercase[QRandomGenerator::global()->bounded(uppercase.length())];
    password += numbers[QRandomGenerator::global()->bounded(numbers.length())];
    password += special[QRandomGenerator::global()->bounded(special.length())];
    
    // Добавляем остальные символы
    const QString allChars = lowercase + uppercase + numbers + special;
    while (password.length() < length) {
        password += allChars[QRandomGenerator::global()->bounded(allChars.length())];
    }
    
    // Перемешиваем пароль
    for (int i = password.length() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        QChar temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
    
    return password;
} 
#ifndef PASSWORDMANAGER_H
#define PASSWORDMANAGER_H

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QSettings>
#include <QUrl>

struct PasswordEntry {
    QString url;
    QString username;
    QString password;
    QString formData;
    QDateTime lastUsed;
    QDateTime created;
    bool neverSave;
    int useCount;
};

class PasswordManager : public QObject
{
    Q_OBJECT

public:
    explicit PasswordManager(QObject *parent = nullptr);
    ~PasswordManager();

    // Основные операции с паролями
    bool savePassword(const QString &url, const QString &username, 
                     const QString &password, const QString &formData = QString());
    bool removePassword(const QString &url, const QString &username);
    bool updatePassword(const QString &url, const QString &username, 
                       const QString &newPassword);
    QList<PasswordEntry> getPasswords(const QString &url) const;
    
    // Автозаполнение
    QString findPassword(const QString &url, const QString &username) const;
    QStringList findUsernames(const QString &url) const;
    QString findFormData(const QString &url, const QString &username) const;

    // Управление исключениями
    void addToNeverSave(const QString &url);
    void removeFromNeverSave(const QString &url);
    bool isNeverSave(const QString &url) const;
    QStringList getNeverSaveList() const;

    // Импорт/Экспорт
    bool exportPasswords(const QString &filename, const QString &masterPassword);
    bool importPasswords(const QString &filename, const QString &masterPassword);
    bool importFromChrome(const QString &filename);
    bool importFromFirefox(const QString &filename);

    // Безопасность
    bool setMasterPassword(const QString &password);
    bool changeMasterPassword(const QString &oldPassword, const QString &newPassword);
    bool verifyMasterPassword(const QString &password) const;
    void clearPasswords();
    
    // Статистика и аналитика
    int getPasswordCount() const;
    QDateTime getLastSync() const;
    QList<PasswordEntry> getWeakPasswords() const;
    QList<PasswordEntry> getReusedPasswords() const;
    QList<PasswordEntry> getOldPasswords() const;

signals:
    void passwordAdded(const QString &url);
    void passwordRemoved(const QString &url);
    void passwordUpdated(const QString &url);
    void masterPasswordChanged();
    void passwordsCleared();
    void importCompleted(int count);
    void exportCompleted(int count);
    void securityAlert(const QString &message);

private:
    QString encryptPassword(const QString &password) const;
    QString decryptPassword(const QString &encrypted) const;
    QString generateSalt() const;
    QString hashPassword(const QString &password, const QString &salt) const;
    bool validatePassword(const QString &password) const;
    void loadPasswords();
    void savePasswords();
    void checkPasswordStrength(const QString &password);
    QString generateStrongPassword(int length = 16) const;

    QHash<QString, QList<PasswordEntry>> m_passwords;
    QStringList m_neverSaveList;
    QString m_masterPasswordHash;
    QString m_salt;
    QString m_encryptionKey;
    QDateTime m_lastSync;
    QSettings m_settings;
    
    static const int MIN_PASSWORD_LENGTH = 8;
    static const int MAX_PASSWORD_LENGTH = 64;
    static const int SALT_LENGTH = 16;
    static const QString CONFIG_FILE;
    static const QString PASSWORDS_FILE;
};

#endif // PASSWORDMANAGER_H 
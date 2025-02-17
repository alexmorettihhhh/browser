#ifndef EXTENSIONMANAGER_H
#define EXTENSIONMANAGER_H

#include <QObject>
#include <QJsonDocument>
#include <QWebEngineScript>
#include <QFileSystemWatcher>
#include <QVersionNumber>

struct ExtensionInfo {
    QString id;
    QString name;
    QString description;
    QString author;
    QVersionNumber version;
    QString homepage;
    QStringList permissions;
    QStringList contentScripts;
    QStringList backgroundScripts;
    QStringList matches;
    QHash<QString, QVariant> settings;
    bool enabled;
    bool hasUpdate;
    QString updateUrl;
    QDateTime installDate;
    QDateTime lastUpdate;
    QString iconPath;
    QHash<QString, QVariant> metadata;
};

class ExtensionManager : public QObject
{
    Q_OBJECT

public:
    explicit ExtensionManager(QObject *parent = nullptr);
    ~ExtensionManager();

    // Управление расширениями
    bool installExtension(const QString &path);
    bool uninstallExtension(const QString &id);
    bool enableExtension(const QString &id, bool enable);
    bool updateExtension(const QString &id);
    
    // Получение информации
    ExtensionInfo getExtensionInfo(const QString &id) const;
    QList<ExtensionInfo> getInstalledExtensions() const;
    QList<ExtensionInfo> getEnabledExtensions() const;
    QStringList getExtensionPermissions(const QString &id) const;
    
    // Настройки расширений
    QVariant getExtensionSetting(const QString &id, const QString &key) const;
    bool setExtensionSetting(const QString &id, const QString &key, const QVariant &value);
    void resetExtensionSettings(const QString &id);
    
    // Управление скриптами
    QList<QWebEngineScript> getContentScripts(const QString &url) const;
    bool injectScript(const QString &id, const QString &script, bool isContent = true);
    bool removeScript(const QString &id, const QString &script);
    
    // Обновления
    bool checkForUpdates();
    QList<QString> getAvailableUpdates() const;
    bool updateAll();
    void setAutoUpdate(bool enable);
    
    // Безопасность
    bool validateExtension(const QString &path) const;
    bool verifyPermissions(const QString &id, const QStringList &permissions) const;
    void revokePermission(const QString &id, const QString &permission);
    
    // Хранилище
    QString getExtensionDataPath(const QString &id) const;
    bool clearExtensionData(const QString &id);
    qint64 getExtensionStorageSize(const QString &id) const;
    
    // Отладка
    bool enableDevMode(bool enable);
    bool reloadExtension(const QString &id);
    QString getExtensionLog(const QString &id) const;
    void clearExtensionLog(const QString &id);

signals:
    void extensionInstalled(const QString &id);
    void extensionUninstalled(const QString &id);
    void extensionEnabled(const QString &id, bool enabled);
    void extensionUpdated(const QString &id);
    void extensionError(const QString &id, const QString &error);
    void updateAvailable(const QString &id);
    void settingsChanged(const QString &id, const QString &key);
    void permissionChanged(const QString &id, const QString &permission);
    void scriptInjected(const QString &id, const QString &script);
    void storageQuotaExceeded(const QString &id);
    void extensionCrashed(const QString &id, const QString &reason);

private slots:
    void handleFileChanged(const QString &path);
    void checkUpdates();
    void monitorExtensions();
    void handleExtensionMessage(const QString &id, const QJsonDocument &message);

private:
    bool loadExtension(const QString &path);
    bool validateManifest(const QJsonDocument &manifest) const;
    void setupExtensionEnvironment(const QString &id);
    void injectContentScripts(const QString &id, const QStringList &scripts);
    void registerBackgroundScripts(const QString &id, const QStringList &scripts);
    void updateExtensionMetadata(const QString &id);
    void cleanupExtension(const QString &id);
    QString generateExtensionId(const QString &path) const;
    bool checkPermissions(const QStringList &permissions) const;
    void saveExtensionState();
    void loadExtensionState();
    
    QHash<QString, ExtensionInfo> m_extensions;
    QHash<QString, QJsonDocument> m_manifests;
    QFileSystemWatcher *m_watcher;
    bool m_devMode;
    bool m_autoUpdate;
    QString m_extensionsPath;
    QHash<QString, QDateTime> m_lastCheck;
    
    static const QString MANIFEST_FILENAME;
    static const QStringList REQUIRED_PERMISSIONS;
    static const int UPDATE_CHECK_INTERVAL = 24 * 60 * 60; // 24 hours
};

#endif // EXTENSIONMANAGER_H 
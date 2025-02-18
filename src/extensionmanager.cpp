#include "extensionmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QProcess>
#include <QNetworkReply>
#include <QNetworkRequest>

const QString ExtensionManager::EXTENSIONS_DIR = "extensions";
const QString ExtensionManager::MANIFEST_FILE = "manifest.json";
const QString ExtensionManager::SETTINGS_FILE = "extensions.json";

ExtensionManager::ExtensionManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    initializeExtensionsDirectory();
    loadSettings();
    loadInstalledExtensions();
}

ExtensionManager::~ExtensionManager()
{
    saveSettings();
}

void ExtensionManager::initializeExtensionsDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_extensionsPath = path + "/" + EXTENSIONS_DIR;
    QDir().mkpath(m_extensionsPath);
}

void ExtensionManager::loadSettings()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + SETTINGS_FILE);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загрузка настроек расширений
            QJsonObject settings = root["settings"].toObject();
            for (auto it = settings.begin(); it != settings.end(); ++it) {
                m_extensionSettings[it.key()] = it.value().toObject().toVariantMap();
            }
            
            // Загрузка состояния расширений
            QJsonObject states = root["states"].toObject();
            for (auto it = states.begin(); it != states.end(); ++it) {
                m_extensionStates[it.key()] = it.value().toBool();
            }
            
            // Загрузка разрешений
            QJsonObject permissions = root["permissions"].toObject();
            for (auto it = permissions.begin(); it != permissions.end(); ++it) {
                QJsonArray perms = it.value().toArray();
                QStringList permList;
                for (const QJsonValue &val : perms) {
                    permList.append(val.toString());
                }
                m_extensionPermissions[it.key()] = permList;
            }
        }
    }
}

void ExtensionManager::saveSettings()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + SETTINGS_FILE);
    
    QJsonObject root;
    
    // Сохранение настроек расширений
    QJsonObject settings;
    for (auto it = m_extensionSettings.begin(); it != m_extensionSettings.end(); ++it) {
        settings[it.key()] = QJsonObject::fromVariantMap(it.value());
    }
    root["settings"] = settings;
    
    // Сохранение состояния расширений
    QJsonObject states;
    for (auto it = m_extensionStates.begin(); it != m_extensionStates.end(); ++it) {
        states[it.key()] = it.value();
    }
    root["states"] = states;
    
    // Сохранение разрешений
    QJsonObject permissions;
    for (auto it = m_extensionPermissions.begin(); it != m_extensionPermissions.end(); ++it) {
        QJsonArray perms;
        for (const QString &perm : it.value()) {
            perms.append(perm);
        }
        permissions[it.key()] = perms;
    }
    root["permissions"] = permissions;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void ExtensionManager::loadInstalledExtensions()
{
    QDir dir(m_extensionsPath);
    QStringList extensions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &extId : extensions) {
        loadExtension(extId);
    }
}

bool ExtensionManager::loadExtension(const QString &id)
{
    QString extPath = m_extensionsPath + "/" + id;
    QFile manifestFile(extPath + "/" + MANIFEST_FILE);
    
    if (!manifestFile.exists() || !manifestFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    manifestFile.close();
    
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject manifest = doc.object();
    Extension ext;
    ext.id = id;
    ext.name = manifest["name"].toString();
    ext.version = manifest["version"].toString();
    ext.description = manifest["description"].toString();
    ext.author = manifest["author"].toString();
    ext.homepage = manifest["homepage"].toString();
    
    // Загрузка разрешений
    QJsonArray permissions = manifest["permissions"].toArray();
    QStringList permList;
    for (const QJsonValue &val : permissions) {
        permList.append(val.toString());
    }
    ext.permissions = permList;
    
    // Загрузка скриптов
    QJsonArray scripts = manifest["content_scripts"].toArray();
    for (const QJsonValue &val : scripts) {
        QJsonObject scriptObj = val.toObject();
        ContentScript script;
        script.matches = scriptObj["matches"].toVariant().toStringList();
        script.js = scriptObj["js"].toVariant().toStringList();
        script.css = scriptObj["css"].toVariant().toStringList();
        ext.contentScripts.append(script);
    }
    
    m_extensions[id] = ext;
    
    // Установка состояния по умолчанию, если не задано
    if (!m_extensionStates.contains(id)) {
        m_extensionStates[id] = true;
    }
    
    emit extensionLoaded(ext);
    return true;
}

bool ExtensionManager::installExtension(const QString &path)
{
    // Проверка архива
    if (!validateExtensionPackage(path)) {
        emit extensionError("Invalid extension package");
        return false;
    }
    
    // Генерация ID расширения
    QString id = generateExtensionId(path);
    QString extPath = m_extensionsPath + "/" + id;
    
    // Распаковка архива
    if (!extractExtensionPackage(path, extPath)) {
        emit extensionError("Failed to extract extension");
        return false;
    }
    
    // Загрузка расширения
    if (!loadExtension(id)) {
        QDir(extPath).removeRecursively();
        emit extensionError("Failed to load extension");
        return false;
    }
    
    emit extensionInstalled(m_extensions[id]);
    saveSettings();
    return true;
}

bool ExtensionManager::uninstallExtension(const QString &id)
{
    if (!m_extensions.contains(id)) {
        return false;
    }
    
    QString extPath = m_extensionsPath + "/" + id;
    
    // Удаление файлов расширения
    if (!QDir(extPath).removeRecursively()) {
        return false;
    }
    
    Extension ext = m_extensions.take(id);
    m_extensionStates.remove(id);
    m_extensionSettings.remove(id);
    m_extensionPermissions.remove(id);
    
    emit extensionUninstalled(id);
    saveSettings();
    return true;
}

bool ExtensionManager::enableExtension(const QString &id, bool enable)
{
    if (!m_extensions.contains(id)) {
        return false;
    }
    
    m_extensionStates[id] = enable;
    emit extensionStateChanged(id, enable);
    saveSettings();
    return true;
}

bool ExtensionManager::isExtensionEnabled(const QString &id) const
{
    return m_extensionStates.value(id, false);
}

QVariantMap ExtensionManager::getExtensionSettings(const QString &id) const
{
    return m_extensionSettings.value(id);
}

void ExtensionManager::setExtensionSetting(const QString &id, const QString &key,
                                         const QVariant &value)
{
    if (!m_extensions.contains(id)) {
        return;
    }
    
    m_extensionSettings[id][key] = value;
    emit extensionSettingChanged(id, key, value);
    saveSettings();
}

QStringList ExtensionManager::getExtensionPermissions(const QString &id) const
{
    return m_extensionPermissions.value(id);
}

bool ExtensionManager::grantPermission(const QString &id, const QString &permission)
{
    if (!m_extensions.contains(id)) {
        return false;
    }
    
    QStringList &perms = m_extensionPermissions[id];
    if (!perms.contains(permission)) {
        perms.append(permission);
        emit extensionPermissionChanged(id, permission, true);
        saveSettings();
    }
    return true;
}

bool ExtensionManager::revokePermission(const QString &id, const QString &permission)
{
    if (!m_extensions.contains(id)) {
        return false;
    }
    
    QStringList &perms = m_extensionPermissions[id];
    if (perms.removeOne(permission)) {
        emit extensionPermissionChanged(id, permission, false);
        saveSettings();
        return true;
    }
    return false;
}

void ExtensionManager::checkForUpdates()
{
    for (const Extension &ext : m_extensions) {
        if (!ext.updateUrl.isEmpty()) {
            QNetworkRequest request(QUrl(ext.updateUrl));
            QNetworkReply *reply = m_networkManager->get(request);
            
            connect(reply, &QNetworkReply::finished, this, [this, reply, ext]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    if (doc.isObject()) {
                        QJsonObject update = doc.object();
                        QString newVersion = update["version"].toString();
                        if (compareVersions(newVersion, ext.version) > 0) {
                            emit updateAvailable(ext.id, newVersion);
                        }
                    }
                }
                reply->deleteLater();
            });
        }
    }
}

bool ExtensionManager::updateExtension(const QString &id)
{
    if (!m_extensions.contains(id)) {
        return false;
    }
    
    const Extension &ext = m_extensions[id];
    if (ext.updateUrl.isEmpty()) {
        return false;
    }
    
    QNetworkRequest request(QUrl(ext.updateUrl));
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            QString packagePath = tempPath + "/" + id + ".crx";
            QFile file(packagePath);
            
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                
                if (installExtension(packagePath)) {
                    emit extensionUpdated(id);
                }
                
                file.remove();
            }
        }
        reply->deleteLater();
    });
    
    return true;
}

QString ExtensionManager::generateExtensionId(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    return hash.result().toHex();
}

bool ExtensionManager::validateExtensionPackage(const QString &path) const
{
    // TODO: Реализовать проверку пакета расширения
    return true;
}

bool ExtensionManager::extractExtensionPackage(const QString &path, const QString &destination) const
{
    // TODO: Реализовать распаковку пакета расширения
    return true;
}

int ExtensionManager::compareVersions(const QString &v1, const QString &v2) const
{
    QStringList parts1 = v1.split('.');
    QStringList parts2 = v2.split('.');
    
    for (int i = 0; i < qMin(parts1.size(), parts2.size()); ++i) {
        int n1 = parts1[i].toInt();
        int n2 = parts2[i].toInt();
        if (n1 != n2) {
            return n1 - n2;
        }
    }
    
    return parts1.size() - parts2.size();
} 
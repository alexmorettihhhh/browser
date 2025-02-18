#include "syncmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QTimer>

const QString SyncManager::SYNC_FILE = "sync_data.json";
const QString SyncManager::DEVICE_FILE = "device_info.json";
const int SyncManager::SYNC_INTERVAL = 300000; // 5 минут

SyncManager::SyncManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_syncTimer(new QTimer(this))
    , m_isSyncing(false)
{
    loadDeviceInfo();
    loadSyncData();
    
    connect(m_syncTimer, &QTimer::timeout, this, &SyncManager::sync);
    
    if (m_settings.autoSync) {
        m_syncTimer->start(SYNC_INTERVAL);
    }
}

SyncManager::~SyncManager()
{
    saveSyncData();
    saveDeviceInfo();
}

void SyncManager::loadDeviceInfo()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + DEVICE_FILE);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            m_deviceInfo.id = root["deviceId"].toString();
            m_deviceInfo.name = root["deviceName"].toString();
            m_deviceInfo.type = root["deviceType"].toString();
            m_deviceInfo.lastSync = QDateTime::fromString(
                root["lastSync"].toString(), Qt::ISODate);
        }
    }
    
    // Генерируем ID устройства, если его нет
    if (m_deviceInfo.id.isEmpty()) {
        m_deviceInfo.id = generateDeviceId();
        m_deviceInfo.name = QSysInfo::machineHostName();
        m_deviceInfo.type = QSysInfo::productType();
        saveDeviceInfo();
    }
}

void SyncManager::saveDeviceInfo()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + DEVICE_FILE);
    
    QJsonObject root;
    root["deviceId"] = m_deviceInfo.id;
    root["deviceName"] = m_deviceInfo.name;
    root["deviceType"] = m_deviceInfo.type;
    root["lastSync"] = m_deviceInfo.lastSync.toString(Qt::ISODate);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void SyncManager::loadSyncData()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(path + "/" + SYNC_FILE);
    
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загрузка настроек синхронизации
            QJsonObject settings = root["settings"].toObject();
            m_settings.autoSync = settings["autoSync"].toBool(true);
            m_settings.syncBookmarks = settings["syncBookmarks"].toBool(true);
            m_settings.syncHistory = settings["syncHistory"].toBool(true);
            m_settings.syncPasswords = settings["syncPasswords"].toBool(true);
            m_settings.syncSettings = settings["syncSettings"].toBool(true);
            m_settings.syncExtensions = settings["syncExtensions"].toBool(true);
            
            // Загрузка состояния синхронизации
            QJsonObject state = root["state"].toObject();
            m_syncState.lastSyncTime = QDateTime::fromString(
                state["lastSyncTime"].toString(), Qt::ISODate);
            m_syncState.lastError = state["lastError"].toString();
            m_syncState.syncInProgress = false;
            
            // Загрузка информации об устройствах
            QJsonArray devices = root["devices"].toArray();
            for (const QJsonValue &val : devices) {
                QJsonObject deviceObj = val.toObject();
                DeviceInfo device;
                device.id = deviceObj["id"].toString();
                device.name = deviceObj["name"].toString();
                device.type = deviceObj["type"].toString();
                device.lastSync = QDateTime::fromString(
                    deviceObj["lastSync"].toString(), Qt::ISODate);
                m_connectedDevices[device.id] = device;
            }
        }
    }
}

void SyncManager::saveSyncData()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/" + SYNC_FILE);
    
    QJsonObject root;
    
    // Сохранение настроек синхронизации
    QJsonObject settings;
    settings["autoSync"] = m_settings.autoSync;
    settings["syncBookmarks"] = m_settings.syncBookmarks;
    settings["syncHistory"] = m_settings.syncHistory;
    settings["syncPasswords"] = m_settings.syncPasswords;
    settings["syncSettings"] = m_settings.syncSettings;
    settings["syncExtensions"] = m_settings.syncExtensions;
    root["settings"] = settings;
    
    // Сохранение состояния синхронизации
    QJsonObject state;
    state["lastSyncTime"] = m_syncState.lastSyncTime.toString(Qt::ISODate);
    state["lastError"] = m_syncState.lastError;
    root["state"] = state;
    
    // Сохранение информации об устройствах
    QJsonArray devices;
    for (const DeviceInfo &device : m_connectedDevices) {
        QJsonObject deviceObj;
        deviceObj["id"] = device.id;
        deviceObj["name"] = device.name;
        deviceObj["type"] = device.type;
        deviceObj["lastSync"] = device.lastSync.toString(Qt::ISODate);
        devices.append(deviceObj);
    }
    root["devices"] = devices;
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void SyncManager::setSyncSettings(const SyncSettings &settings)
{
    m_settings = settings;
    
    if (m_settings.autoSync && !m_syncTimer->isActive()) {
        m_syncTimer->start(SYNC_INTERVAL);
    } else if (!m_settings.autoSync && m_syncTimer->isActive()) {
        m_syncTimer->stop();
    }
    
    saveSyncData();
    emit syncSettingsChanged();
}

SyncSettings SyncManager::getSyncSettings() const
{
    return m_settings;
}

void SyncManager::sync()
{
    if (m_isSyncing) {
        return;
    }
    
    m_isSyncing = true;
    emit syncStarted();
    
    // Подготавливаем данные для синхронизации
    QJsonObject syncData;
    
    if (m_settings.syncBookmarks) {
        syncData["bookmarks"] = prepareBookmarksData();
    }
    
    if (m_settings.syncHistory) {
        syncData["history"] = prepareHistoryData();
    }
    
    if (m_settings.syncPasswords) {
        syncData["passwords"] = preparePasswordsData();
    }
    
    if (m_settings.syncSettings) {
        syncData["settings"] = prepareSettingsData();
    }
    
    if (m_settings.syncExtensions) {
        syncData["extensions"] = prepareExtensionsData();
    }
    
    // Отправляем данные на сервер
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject data;
    data["deviceId"] = m_deviceInfo.id;
    data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["data"] = syncData;
    
    QNetworkReply *reply = m_networkManager->post(request, 
                                                QJsonDocument(data).toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            if (response.isObject()) {
                processServerResponse(response.object());
            }
        } else {
            m_syncState.lastError = reply->errorString();
            emit syncError(m_syncState.lastError);
        }
        
        m_isSyncing = false;
        m_syncState.lastSyncTime = QDateTime::currentDateTime();
        saveSyncData();
        
        emit syncFinished();
        reply->deleteLater();
    });
}

void SyncManager::processServerResponse(const QJsonObject &response)
{
    // Обработка ответа от сервера
    if (response["status"].toString() == "success") {
        // Обновляем локальные данные
        QJsonObject syncedData = response["data"].toObject();
        
        if (m_settings.syncBookmarks && syncedData.contains("bookmarks")) {
            updateBookmarks(syncedData["bookmarks"].toObject());
        }
        
        if (m_settings.syncHistory && syncedData.contains("history")) {
            updateHistory(syncedData["history"].toObject());
        }
        
        if (m_settings.syncPasswords && syncedData.contains("passwords")) {
            updatePasswords(syncedData["passwords"].toObject());
        }
        
        if (m_settings.syncSettings && syncedData.contains("settings")) {
            updateSettings(syncedData["settings"].toObject());
        }
        
        if (m_settings.syncExtensions && syncedData.contains("extensions")) {
            updateExtensions(syncedData["extensions"].toObject());
        }
        
        // Обновляем информацию об устройствах
        QJsonArray devices = response["devices"].toArray();
        m_connectedDevices.clear();
        
        for (const QJsonValue &val : devices) {
            QJsonObject deviceObj = val.toObject();
            DeviceInfo device;
            device.id = deviceObj["id"].toString();
            device.name = deviceObj["name"].toString();
            device.type = deviceObj["type"].toString();
            device.lastSync = QDateTime::fromString(
                deviceObj["lastSync"].toString(), Qt::ISODate);
            m_connectedDevices[device.id] = device;
        }
        
        emit devicesUpdated();
    } else {
        m_syncState.lastError = response["error"].toString();
        emit syncError(m_syncState.lastError);
    }
}

QString SyncManager::generateDeviceId() const
{
    QString systemInfo = QSysInfo::machineHostName() + 
                        QSysInfo::machineUniqueId() +
                        QDateTime::currentDateTime().toString();
    
    return QCryptographicHash::hash(systemInfo.toUtf8(), 
                                   QCryptographicHash::Sha256).toHex();
}

QJsonObject SyncManager::prepareBookmarksData() const
{
    // TODO: Реализовать подготовку данных закладок
    return QJsonObject();
}

QJsonObject SyncManager::prepareHistoryData() const
{
    // TODO: Реализовать подготовку данных истории
    return QJsonObject();
}

QJsonObject SyncManager::preparePasswordsData() const
{
    // TODO: Реализовать подготовку данных паролей
    return QJsonObject();
}

QJsonObject SyncManager::prepareSettingsData() const
{
    // TODO: Реализовать подготовку данных настроек
    return QJsonObject();
}

QJsonObject SyncManager::prepareExtensionsData() const
{
    // TODO: Реализовать подготовку данных расширений
    return QJsonObject();
}

void SyncManager::updateBookmarks(const QJsonObject &data)
{
    // TODO: Реализовать обновление закладок
}

void SyncManager::updateHistory(const QJsonObject &data)
{
    // TODO: Реализовать обновление истории
}

void SyncManager::updatePasswords(const QJsonObject &data)
{
    // TODO: Реализовать обновление паролей
}

void SyncManager::updateSettings(const QJsonObject &data)
{
    // TODO: Реализовать обновление настроек
}

void SyncManager::updateExtensions(const QJsonObject &data)
{
    // TODO: Реализовать обновление расширений
}

QList<DeviceInfo> SyncManager::getConnectedDevices() const
{
    return m_connectedDevices.values();
}

bool SyncManager::isDeviceConnected(const QString &deviceId) const
{
    return m_connectedDevices.contains(deviceId);
}

void SyncManager::disconnectDevice(const QString &deviceId)
{
    if (m_connectedDevices.remove(deviceId) > 0) {
        saveSyncData();
        emit deviceDisconnected(deviceId);
    }
}

void SyncManager::setServerUrl(const QUrl &url)
{
    m_serverUrl = url;
}

QUrl SyncManager::getServerUrl() const
{
    return m_serverUrl;
}

SyncState SyncManager::getSyncState() const
{
    return m_syncState;
}

bool SyncManager::isSyncing() const
{
    return m_isSyncing;
} 
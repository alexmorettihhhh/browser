#include "settings.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent)
    : QDialog(parent)
    , settings(settings)
{
    setWindowTitle("Настройки браузера");
    setMinimumSize(600, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Создаем вкладки
    tabWidget = new QTabWidget(this);
    createGeneralTab();
    createPrivacyTab();
    createNetworkTab();
    createAdvancedTab();
    mainLayout->addWidget(tabWidget);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *saveButton = new QPushButton("Сохранить", this);
    QPushButton *cancelButton = new QPushButton("Отмена", this);
    QPushButton *importButton = new QPushButton("Импорт настроек", this);
    QPushButton *exportButton = new QPushButton("Экспорт настроек", this);
    
    buttonLayout->addWidget(importButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(importButton, &QPushButton::clicked, this, &SettingsDialog::importSettings);
    connect(exportButton, &QPushButton::clicked, this, &SettingsDialog::exportSettings);
}

void SettingsDialog::createGeneralTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Тема
    QGroupBox *themeGroup = new QGroupBox("Внешний вид", tab);
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroup);
    darkModeCheckBox = new QCheckBox("Темная тема", themeGroup);
    darkModeCheckBox->setChecked(settings->value("darkMode", true).toBool());
    themeLayout->addWidget(darkModeCheckBox);

    // Домашняя страница
    QGroupBox *homeGroup = new QGroupBox("Домашняя страница", tab);
    QVBoxLayout *homeLayout = new QVBoxLayout(homeGroup);
    homepageEdit = new QLineEdit(settings->value("homepage", "https://www.google.com").toString(), homeGroup);
    homeLayout->addWidget(homepageEdit);

    // Загрузки
    QGroupBox *downloadGroup = new QGroupBox("Загрузки", tab);
    QVBoxLayout *downloadLayout = new QVBoxLayout(downloadGroup);
    QHBoxLayout *pathLayout = new QHBoxLayout;
    downloadPathEdit = new QLineEdit(settings->value("downloadPath").toString(), downloadGroup);
    QPushButton *browseButton = new QPushButton("Обзор", downloadGroup);
    pathLayout->addWidget(downloadPathEdit);
    pathLayout->addWidget(browseButton);
    downloadLayout->addLayout(pathLayout);

    // Сессия
    QGroupBox *sessionGroup = new QGroupBox("Сессия", tab);
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionGroup);
    restoreSessionCheckBox = new QCheckBox("Восстанавливать предыдущую сессию при запуске", sessionGroup);
    restoreSessionCheckBox->setChecked(settings->value("restoreSession", true).toBool());
    sessionLayout->addWidget(restoreSessionCheckBox);

    layout->addWidget(themeGroup);
    layout->addWidget(homeGroup);
    layout->addWidget(downloadGroup);
    layout->addWidget(sessionGroup);
    layout->addStretch();

    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browseDownloadPath);
    
    tabWidget->addTab(tab, "Общие");
}

void SettingsDialog::createPrivacyTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Безопасность
    QGroupBox *securityGroup = new QGroupBox("Безопасность", tab);
    QVBoxLayout *securityLayout = new QVBoxLayout(securityGroup);
    
    adBlockCheckBox = new QCheckBox("Блокировка рекламы", securityGroup);
    trackingProtectionCheckBox = new QCheckBox("Защита от отслеживания", securityGroup);
    httpsEverywhereCheckBox = new QCheckBox("HTTPS Everywhere", securityGroup);
    
    adBlockCheckBox->setChecked(settings->value("adBlock", true).toBool());
    trackingProtectionCheckBox->setChecked(settings->value("trackingProtection", true).toBool());
    httpsEverywhereCheckBox->setChecked(settings->value("httpsEverywhere", true).toBool());
    
    securityLayout->addWidget(adBlockCheckBox);
    securityLayout->addWidget(trackingProtectionCheckBox);
    securityLayout->addWidget(httpsEverywhereCheckBox);

    // Приватность
    QGroupBox *privacyGroup = new QGroupBox("Приватность", tab);
    QVBoxLayout *privacyLayout = new QVBoxLayout(privacyGroup);
    
    cookiesBlockCheckBox = new QCheckBox("Блокировать сторонние куки", privacyGroup);
    passwordSaveCheckBox = new QCheckBox("Сохранять пароли", privacyGroup);
    formAutofillCheckBox = new QCheckBox("Автозаполнение форм", privacyGroup);
    
    cookiesBlockCheckBox->setChecked(settings->value("blockCookies", false).toBool());
    passwordSaveCheckBox->setChecked(settings->value("savePasswords", true).toBool());
    formAutofillCheckBox->setChecked(settings->value("formAutofill", true).toBool());
    
    privacyLayout->addWidget(cookiesBlockCheckBox);
    privacyLayout->addWidget(passwordSaveCheckBox);
    privacyLayout->addWidget(formAutofillCheckBox);

    // Очистка данных
    QGroupBox *clearGroup = new QGroupBox("Очистка данных", tab);
    QVBoxLayout *clearLayout = new QVBoxLayout(clearGroup);
    QPushButton *clearButton = new QPushButton("Очистить данные браузера", clearGroup);
    clearLayout->addWidget(clearButton);

    layout->addWidget(securityGroup);
    layout->addWidget(privacyGroup);
    layout->addWidget(clearGroup);
    layout->addStretch();

    connect(clearButton, &QPushButton::clicked, this, &SettingsDialog::clearBrowsingData);
    
    tabWidget->addTab(tab, "Приватность");
}

void SettingsDialog::createNetworkTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Прокси
    QGroupBox *proxyGroup = new QGroupBox("Прокси-сервер", tab);
    QVBoxLayout *proxyLayout = new QVBoxLayout(proxyGroup);
    
    proxyEnabledCheckBox = new QCheckBox("Использовать прокси-сервер", proxyGroup);
    proxyEnabledCheckBox->setChecked(settings->value("proxyEnabled", false).toBool());
    
    QGridLayout *proxyGridLayout = new QGridLayout;
    proxyGridLayout->addWidget(new QLabel("Хост:"), 0, 0);
    proxyGridLayout->addWidget(new QLabel("Порт:"), 1, 0);
    proxyGridLayout->addWidget(new QLabel("Пользователь:"), 2, 0);
    proxyGridLayout->addWidget(new QLabel("Пароль:"), 3, 0);
    
    proxyHostEdit = new QLineEdit(settings->value("proxyHost").toString(), proxyGroup);
    proxyPortSpinBox = new QSpinBox(proxyGroup);
    proxyPortSpinBox->setRange(0, 65535);
    proxyPortSpinBox->setValue(settings->value("proxyPort", 8080).toInt());
    proxyUsernameEdit = new QLineEdit(settings->value("proxyUsername").toString(), proxyGroup);
    proxyPasswordEdit = new QLineEdit(settings->value("proxyPassword").toString(), proxyGroup);
    proxyPasswordEdit->setEchoMode(QLineEdit::Password);
    
    proxyGridLayout->addWidget(proxyHostEdit, 0, 1);
    proxyGridLayout->addWidget(proxyPortSpinBox, 1, 1);
    proxyGridLayout->addWidget(proxyUsernameEdit, 2, 1);
    proxyGridLayout->addWidget(proxyPasswordEdit, 3, 1);
    
    proxyLayout->addWidget(proxyEnabledCheckBox);
    proxyLayout->addLayout(proxyGridLayout);

    // User Agent
    QGroupBox *uaGroup = new QGroupBox("User Agent", tab);
    QVBoxLayout *uaLayout = new QVBoxLayout(uaGroup);
    
    userAgentComboBox = new QComboBox(uaGroup);
    userAgentComboBox->addItem("По умолчанию", "");
    userAgentComboBox->addItem("Chrome Windows", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    userAgentComboBox->addItem("Firefox Windows", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0");
    userAgentComboBox->addItem("Safari macOS", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.1 Safari/605.1.15");
    
    int uaIndex = userAgentComboBox->findData(settings->value("userAgent"));
    if (uaIndex != -1) {
        userAgentComboBox->setCurrentIndex(uaIndex);
    }
    
    uaLayout->addWidget(userAgentComboBox);

    layout->addWidget(proxyGroup);
    layout->addWidget(uaGroup);
    layout->addStretch();
    
    tabWidget->addTab(tab, "Сеть");
}

void SettingsDialog::createAdvancedTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Производительность
    QGroupBox *perfGroup = new QGroupBox("Производительность", tab);
    QVBoxLayout *perfLayout = new QVBoxLayout(perfGroup);
    
    javascriptEnabledCheckBox = new QCheckBox("Включить JavaScript", perfGroup);
    imagesEnabledCheckBox = new QCheckBox("Загружать изображения", perfGroup);
    webGLEnabledCheckBox = new QCheckBox("Включить WebGL", perfGroup);
    
    javascriptEnabledCheckBox->setChecked(settings->value("javascriptEnabled", true).toBool());
    imagesEnabledCheckBox->setChecked(settings->value("imagesEnabled", true).toBool());
    webGLEnabledCheckBox->setChecked(settings->value("webGLEnabled", true).toBool());
    
    QHBoxLayout *cacheLayout = new QHBoxLayout;
    cacheLayout->addWidget(new QLabel("Размер кэша (МБ):"));
    cacheSizeMBSpinBox = new QSpinBox(perfGroup);
    cacheSizeMBSpinBox->setRange(50, 10000);
    cacheSizeMBSpinBox->setValue(settings->value("cacheSizeMB", 500).toInt());
    cacheLayout->addWidget(cacheSizeMBSpinBox);
    
    perfLayout->addWidget(javascriptEnabledCheckBox);
    perfLayout->addWidget(imagesEnabledCheckBox);
    perfLayout->addWidget(webGLEnabledCheckBox);
    perfLayout->addLayout(cacheLayout);

    // Движок рендеринга
    QGroupBox *engineGroup = new QGroupBox("Движок рендеринга", tab);
    QVBoxLayout *engineLayout = new QVBoxLayout(engineGroup);
    
    renderingEngineComboBox = new QComboBox(engineGroup);
    renderingEngineComboBox->addItem("Автоматически", "auto");
    renderingEngineComboBox->addItem("Программный", "software");
    renderingEngineComboBox->addItem("OpenGL", "opengl");
    
    QString currentEngine = settings->value("renderingEngine", "auto").toString();
    int engineIndex = renderingEngineComboBox->findData(currentEngine);
    if (engineIndex != -1) {
        renderingEngineComboBox->setCurrentIndex(engineIndex);
    }
    
    engineLayout->addWidget(renderingEngineComboBox);

    layout->addWidget(perfGroup);
    layout->addWidget(engineGroup);
    layout->addStretch();
    
    tabWidget->addTab(tab, "Дополнительно");
}

void SettingsDialog::saveSettings()
{
    // Общие
    settings->setValue("darkMode", darkModeCheckBox->isChecked());
    settings->setValue("homepage", homepageEdit->text());
    settings->setValue("downloadPath", downloadPathEdit->text());
    settings->setValue("restoreSession", restoreSessionCheckBox->isChecked());

    // Приватность
    settings->setValue("adBlock", adBlockCheckBox->isChecked());
    settings->setValue("trackingProtection", trackingProtectionCheckBox->isChecked());
    settings->setValue("httpsEverywhere", httpsEverywhereCheckBox->isChecked());
    settings->setValue("blockCookies", cookiesBlockCheckBox->isChecked());
    settings->setValue("savePasswords", passwordSaveCheckBox->isChecked());
    settings->setValue("formAutofill", formAutofillCheckBox->isChecked());

    // Сеть
    settings->setValue("proxyEnabled", proxyEnabledCheckBox->isChecked());
    settings->setValue("proxyHost", proxyHostEdit->text());
    settings->setValue("proxyPort", proxyPortSpinBox->value());
    settings->setValue("proxyUsername", proxyUsernameEdit->text());
    settings->setValue("proxyPassword", proxyPasswordEdit->text());
    settings->setValue("userAgent", userAgentComboBox->currentData());

    // Дополнительно
    settings->setValue("javascriptEnabled", javascriptEnabledCheckBox->isChecked());
    settings->setValue("imagesEnabled", imagesEnabledCheckBox->isChecked());
    settings->setValue("webGLEnabled", webGLEnabledCheckBox->isChecked());
    settings->setValue("cacheSizeMB", cacheSizeMBSpinBox->value());
    settings->setValue("renderingEngine", renderingEngineComboBox->currentData());

    emit settingsChanged();
    emit themeChanged(darkModeCheckBox->isChecked());
    emit proxySettingsChanged();
    emit userAgentChanged(userAgentComboBox->currentData().toString());

    accept();
}

void SettingsDialog::browseDownloadPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для загрузок",
                                                  downloadPathEdit->text(),
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        downloadPathEdit->setText(dir);
    }
}

void SettingsDialog::clearBrowsingData()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Очистка данных",
                                "Вы уверены, что хотите очистить все данные браузера?\n"
                                "Это действие нельзя отменить.",
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Очистка истории
        settings->remove("history");
        // Очистка кэша
        settings->remove("cache");
        // Очистка куков
        settings->remove("cookies");
        // Очистка паролей
        settings->remove("passwords");
        
        QMessageBox::information(this, "Очистка данных", "Данные браузера успешно очищены.");
    }
}

void SettingsDialog::exportSettings()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт настроек",
                                                  QDir::homePath() + "/browser_settings.json",
                                                  "JSON files (*.json)");
    if (fileName.isEmpty()) {
        return;
    }

    QJsonObject settingsObj;
    foreach(const QString &key, settings->allKeys()) {
        settingsObj[key] = QJsonValue::fromVariant(settings->value(key));
    }

    QJsonDocument doc(settingsObj);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        QMessageBox::information(this, "Экспорт настроек", "Настройки успешно экспортированы.");
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл настроек.");
    }
}

void SettingsDialog::importSettings()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Импорт настроек",
                                                  QDir::homePath(),
                                                  "JSON files (*.json)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject settingsObj = doc.object();

        foreach(const QString &key, settingsObj.keys()) {
            settings->setValue(key, settingsObj[key].toVariant());
        }

        QMessageBox::information(this, "Импорт настроек", "Настройки успешно импортированы.\n"
                                                       "Перезапустите браузер для применения изменений.");
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл настроек.");
    }
} 
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QSettings>
#include <QComboBox>
#include <QFontComboBox>
#include <QColorDialog>
#include <QSlider>
#include <QListWidget>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QSettings *settings, QWidget *parent = nullptr);

signals:
    void settingsChanged();
    void themeChanged(bool isDark);
    void proxySettingsChanged();
    void userAgentChanged(const QString &agent);
    void searchEngineChanged(const QString &engine);
    void downloadPathChanged(const QString &path);
    void fontSettingsChanged();
    void privacySettingsChanged();
    void adBlockSettingsChanged();

private slots:
    void saveSettings();
    void resetToDefaults();
    void importSettings();
    void exportSettings();
    void browseDownloadPath();
    void selectCustomThemeColor();
    void previewFont(const QFont &font);
    void updateSearchEngine(int index);
    void clearBrowsingData();
    void manageCookies();
    void managePasswords();
    void managePermissions();
    void configureAdBlock();
    void testProxy();

private:
    void createGeneralTab();
    void createAppearanceTab();
    void createPrivacyTab();
    void createNetworkTab();
    void createAdvancedTab();
    void createSearchTab();
    void createDownloadsTab();
    void createSecurityTab();
    void createAdBlockTab();
    void loadSettings();
    void applySettings();
    void setupConnections();

    QTabWidget *tabWidget;
    QSettings *settings;

    // Общие настройки
    QLineEdit *homepageEdit;
    QCheckBox *restoreSessionCheckBox;
    QCheckBox *showBookmarksBarCheckBox;
    QCheckBox *showStatusBarCheckBox;
    QComboBox *startupBehaviorCombo;
    QCheckBox *askBeforeClosingCheckBox;

    // Внешний вид
    QCheckBox *darkModeCheckBox;
    QPushButton *customThemeColorButton;
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QComboBox *zoomBehaviorCombo;
    QSlider *uiScaleSlider;
    QCheckBox *smoothScrollingCheckBox;
    QCheckBox *animationsEnabledCheckBox;

    // Поиск
    QComboBox *defaultSearchEngineCombo;
    QListWidget *searchEnginesList;
    QLineEdit *customSearchUrlEdit;
    QCheckBox *searchSuggestionsCheckBox;
    QCheckBox *searchHighlightCheckBox;

    // Загрузки
    QLineEdit *downloadPathEdit;
    QCheckBox *askDownloadPathCheckBox;
    QCheckBox *autoOpenDownloadsCheckBox;
    QSpinBox *maxConcurrentDownloadsSpinBox;
    QComboBox *downloadBehaviorCombo;

    // Приватность
    QCheckBox *doNotTrackCheckBox;
    QCheckBox *blockThirdPartyCookiesCheckBox;
    QComboBox *cookiePolicyCombo;
    QCheckBox *clearDataOnExitCheckBox;
    QCheckBox *sendErrorReportsCheckBox;
    QCheckBox *blockPopupsCheckBox;
    QCheckBox *blockNotificationsCheckBox;

    // Сеть
    QCheckBox *proxyEnabledCheckBox;
    QLineEdit *proxyHostEdit;
    QSpinBox *proxyPortSpinBox;
    QLineEdit *proxyUsernameEdit;
    QLineEdit *proxyPasswordEdit;
    QComboBox *proxyTypeCombo;
    QCheckBox *autoDetectProxyCheckBox;

    // AdBlock
    QCheckBox *adBlockEnabledCheckBox;
    QCheckBox *easyListEnabledCheckBox;
    QListWidget *customRulesList;
    QListWidget *whitelistedSitesList;
    QCheckBox *showAdBlockStatsCheckBox;
    QCheckBox *aggressiveBlockingCheckBox;

    // Расширенные
    QCheckBox *hardwareAccelerationCheckBox;
    QCheckBox *webGLEnabledCheckBox;
    QSpinBox *cacheSizeMBSpinBox;
    QComboBox *renderingEngineCombo;
    QSpinBox *maxMemoryUsageMBSpinBox;
    QCheckBox *experimentalFeaturesCheckBox;
    QComboBox *logLevelCombo;
};

#endif // SETTINGS_H 
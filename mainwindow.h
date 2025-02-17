#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QLineEdit>
#include <QToolBar>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QSettings>
#include <QVector>
#include <QString>
#include <QTabWidget>
#include <QList>
#include <QIcon>
#include "types.h"
#include "downloadmanager.h"
#include "downloadsdialog.h"
#include "readermode.h"
#include "adblocker.h"

struct HistoryEntry {
    QString url;
    QString title;
    QDateTime timestamp;
};

struct Bookmark {
    QString url;
    QString title;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void navigateToUrl();
    void updateUrlBar(const QUrl &url);
    void goBack();
    void goForward();
    void reload();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void addBookmark();
    void showBookmarks();
    void showHistory();
    void findInPage();
    void findNext();
    void findPrevious();
    void clearHistory();
    void showSettings();
    void updateTitle(const QString &title);
    void toggleDarkMode();
    void toggleAdBlock();
    void toggleJavaScript();
    void toggleImages();
    void changeUserAgent();
    void clearBrowsingData();
    void exportBookmarks();
    void importBookmarks();
    void changeDownloadPath();
    void toggleProxy();
    void setCustomProxy();
    void addNewTab(const QUrl &url = QUrl("about:blank"));
    void closeTab(int index);
    void duplicateTab();
    void nextTab();
    void previousTab();
    void tabChanged(int index);
    void setTabTitle(const QString &title);
    void setTabIcon(const QIcon &icon);

private:
    void createMenus();
    void createToolBars();
    void loadSettings();
    void saveSettings();
    void addToHistory(const QString &url, const QString &title);
    void applyStyle();
    void setupWebView();
    void setupTab(QWebEngineView *webView, const QString &title = QString());

    QWebEngineView *webView;
    QWebEngineProfile *webProfile;
    QLineEdit *urlBar;
    QToolBar *toolBar;
    QLineEdit *findBar;
    QMenu *bookmarksMenu;
    QMenu *historyMenu;
    QStatusBar *statusBar;
    QVector<HistoryEntry> history;
    QVector<Bookmark> bookmarks;
    QSettings settings;
    float currentZoom;
    bool darkMode;
    bool adBlockEnabled;
    bool javascriptEnabled;
    bool imagesEnabled;
    QString userAgent;
    QString downloadPath;
    bool proxyEnabled;
    QString proxyHost;
    int proxyPort;
    QString proxyUsername;
    QString proxyPassword;
    QTabWidget *tabWidget;
    QList<QWebEngineView*> webViews;
    QWebEngineView* currentWebView() const;
}; 
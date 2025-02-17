#include "mainwindow.h"
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIcon>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QShortcut>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QNetworkProxy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QSpinBox>
#include <QLabel>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settings("MyBrowser", "Browser")
    , currentZoom(1.0)
    , darkMode(false)
    , adBlockEnabled(false)
    , javascriptEnabled(true)
    , imagesEnabled(true)
    , proxyEnabled(false)
{
    // Создаем главный виджет
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Создаем вертикальный layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    createMenus();
    createToolBars();
    
    // Создаем виджет вкладок
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setDocumentMode(true);
    layout->addWidget(tabWidget);
    
    // Создаем строку поиска
    findBar = new QLineEdit(this);
    findBar->setPlaceholderText("Поиск на странице...");
    findBar->hide();
    layout->addWidget(findBar);
    
    // Создаем статусную строку
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    
    // Подключаем сигналы вкладок
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabChanged);
    
    // Подключаем сигналы поиска
    connect(findBar, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    
    // Устанавливаем горячие клавиши
    new QShortcut(QKeySequence::Find, this, this, &MainWindow::findInPage);
    new QShortcut(QKeySequence::ZoomIn, this, this, &MainWindow::zoomIn);
    new QShortcut(QKeySequence::ZoomOut, this, this, &MainWindow::zoomOut);
    new QShortcut(QKeySequence("Ctrl+0"), this, this, &MainWindow::resetZoom);
    new QShortcut(QKeySequence("Ctrl+D"), this, this, &MainWindow::addBookmark);
    new QShortcut(QKeySequence("Ctrl+Tab"), this, this, &MainWindow::nextTab);
    new QShortcut(QKeySequence("Ctrl+Shift+Tab"), this, this, &MainWindow::previousTab);
    new QShortcut(QKeySequence("Ctrl+T"), this, [this]() { addNewTab(); });
    new QShortcut(QKeySequence("Ctrl+W"), this, [this]() { closeTab(tabWidget->currentIndex()); });
    new QShortcut(QKeySequence("Ctrl+Shift+T"), this, &MainWindow::duplicateTab);
    
    // Загружаем настройки
    loadSettings();
    
    // Создаем первую вкладку
    addNewTab(QUrl(settings.value("homepage", "https://www.google.com").toString()));
    
    // Устанавливаем размер окна
    resize(1024, 768);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::createMenus()
{
    // Меню Файл
    QMenu *fileMenu = menuBar()->addMenu("Файл");
    fileMenu->addAction("Новая вкладка", this, [this]() {
        addNewTab();
    }, QKeySequence("Ctrl+T"));
    fileMenu->addAction("Закрыть вкладку", this, [this]() {
        closeTab(tabWidget->currentIndex());
    }, QKeySequence("Ctrl+W"));
    fileMenu->addAction("Дублировать вкладку", this, &MainWindow::duplicateTab, QKeySequence("Ctrl+Shift+T"));
    fileMenu->addSeparator();
    fileMenu->addAction("Сохранить страницу как...", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) {
            QString fileName = QFileDialog::getSaveFileName(this, "Сохранить страницу",
                QDir::homePath() + "/" + view->title() + ".html",
                "HTML Files (*.html *.htm);;All Files (*)");
            if (!fileName.isEmpty()) {
                view->page()->save(fileName);
            }
        }
    }, QKeySequence("Ctrl+S"));
    fileMenu->addAction("Сохранить как PDF...", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) {
            QString fileName = QFileDialog::getSaveFileName(this, "Сохранить как PDF",
                QDir::homePath() + "/" + view->title() + ".pdf",
                "PDF Files (*.pdf);;All Files (*)");
            if (!fileName.isEmpty()) {
                view->page()->printToPdf(fileName);
            }
        }
    });
    fileMenu->addSeparator();
    fileMenu->addAction("Выход", this, &QWidget::close, QKeySequence("Alt+F4"));
    
    // Меню Правка
    QMenu *editMenu = menuBar()->addMenu("Правка");
    editMenu->addAction("Найти...", this, &MainWindow::findInPage, QKeySequence::Find);
    editMenu->addAction("Найти далее", this, &MainWindow::findNext, QKeySequence::FindNext);
    editMenu->addAction("Найти ранее", this, &MainWindow::findPrevious, QKeySequence::FindPrevious);
    editMenu->addSeparator();
    editMenu->addAction("Копировать", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) view->page()->triggerAction(QWebEnginePage::Copy);
    }, QKeySequence::Copy);
    editMenu->addAction("Вставить", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) view->page()->triggerAction(QWebEnginePage::Paste);
    }, QKeySequence::Paste);
    editMenu->addAction("Вырезать", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) view->page()->triggerAction(QWebEnginePage::Cut);
    }, QKeySequence::Cut);
    editMenu->addAction("Выделить все", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) view->page()->triggerAction(QWebEnginePage::SelectAll);
    }, QKeySequence::SelectAll);
    
    // Меню Вид
    QMenu *viewMenu = menuBar()->addMenu("Вид");
    viewMenu->addAction("Увеличить", this, &MainWindow::zoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction("Уменьшить", this, &MainWindow::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction("Сбросить масштаб", this, &MainWindow::resetZoom, QKeySequence("Ctrl+0"));
    viewMenu->addSeparator();
    viewMenu->addAction("Режим чтения", this, [this]() {
        QWebEngineView *view = currentWebView();
        if (view) {
            ReaderMode *reader = new ReaderMode(this);
            reader->setContent(view->page()->toHtml(), view->url());
            reader->show();
        }
    }, QKeySequence("Ctrl+Alt+R"));
    viewMenu->addSeparator();
    viewMenu->addAction("Темная тема", this, &MainWindow::toggleDarkMode, QKeySequence("Ctrl+Alt+D"))
        ->setCheckable(true);
    
    // Меню Закладки
    bookmarksMenu = menuBar()->addMenu("Закладки");
    bookmarksMenu->addAction("Добавить закладку", this, &MainWindow::addBookmark, QKeySequence("Ctrl+D"));
    bookmarksMenu->addAction("Показать закладки", this, &MainWindow::showBookmarks, QKeySequence("Ctrl+Shift+B"));
    bookmarksMenu->addSeparator();
    bookmarksMenu->addAction("Импорт закладок...", this, &MainWindow::importBookmarks);
    bookmarksMenu->addAction("Экспорт закладок...", this, &MainWindow::exportBookmarks);
    bookmarksMenu->addSeparator();
    
    // Меню История
    historyMenu = menuBar()->addMenu("История");
    historyMenu->addAction("Показать историю", this, &MainWindow::showHistory, QKeySequence("Ctrl+H"));
    historyMenu->addAction("Очистить историю", this, &MainWindow::clearHistory);
    historyMenu->addSeparator();
    
    // Меню Инструменты
    QMenu *toolsMenu = menuBar()->addMenu("Инструменты");
    toolsMenu->addAction("Блокировка рекламы", this, &MainWindow::toggleAdBlock)
        ->setCheckable(true);
    toolsMenu->addAction("JavaScript", this, &MainWindow::toggleJavaScript)
        ->setCheckable(true);
    toolsMenu->addAction("Загрузка изображений", this, &MainWindow::toggleImages)
        ->setCheckable(true);
    toolsMenu->addSeparator();
    toolsMenu->addAction("User Agent...", this, &MainWindow::changeUserAgent);
    toolsMenu->addAction("Прокси...", this, &MainWindow::setCustomProxy);
    toolsMenu->addAction("Папка загрузки...", this, &MainWindow::changeDownloadPath);
    toolsMenu->addSeparator();
    toolsMenu->addAction("Очистить данные браузера...", this, &MainWindow::clearBrowsingData);
    
    // Меню Настройки
    QMenu *settingsMenu = menuBar()->addMenu("Настройки");
    settingsMenu->addAction("Настройки", this, &MainWindow::showSettings, QKeySequence("Ctrl+,"));
}

void MainWindow::createToolBars()
{
    toolBar = addToolBar("Navigation");
    toolBar->setMovable(false);
    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    
    // Кнопки навигации
    QPushButton *backButton = new QPushButton(QIcon(":/icons/back.png"), "", this);
    QPushButton *forwardButton = new QPushButton(QIcon(":/icons/forward.png"), "", this);
    QPushButton *reloadButton = new QPushButton(QIcon(":/icons/reload.png"), "", this);
    QPushButton *homeButton = new QPushButton(QIcon(":/icons/home.png"), "", this);
    
    backButton->setToolTip("Назад (Alt+Left)");
    forwardButton->setToolTip("Вперед (Alt+Right)");
    reloadButton->setToolTip("Обновить (F5)");
    homeButton->setToolTip("Домашняя страница");
    
    toolBar->addWidget(backButton);
    toolBar->addWidget(forwardButton);
    toolBar->addWidget(reloadButton);
    toolBar->addWidget(homeButton);
    
    // Адресная строка
    urlBar = new QLineEdit(this);
    urlBar->setPlaceholderText("Введите адрес или поисковый запрос");
    urlBar->setClearButtonEnabled(true);
    toolBar->addWidget(urlBar);
    
    // Кнопки управления
    QPushButton *bookmarkButton = new QPushButton(QIcon(":/icons/bookmark.png"), "", this);
    QPushButton *historyButton = new QPushButton(QIcon(":/icons/history.png"), "", this);
    QPushButton *settingsButton = new QPushButton(QIcon(":/icons/settings.png"), "", this);
    QPushButton *readerButton = new QPushButton(QIcon(":/icons/reader.png"), "", this);
    
    bookmarkButton->setToolTip("Добавить в закладки (Ctrl+D)");
    historyButton->setToolTip("История (Ctrl+H)");
    settingsButton->setToolTip("Настройки (Ctrl+,)");
    readerButton->setToolTip("Режим чтения (Ctrl+Alt+R)");
    
    toolBar->addWidget(bookmarkButton);
    toolBar->addWidget(historyButton);
    toolBar->addWidget(settingsButton);
    toolBar->addWidget(readerButton);
    
    // Подключаем сигналы
    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (QWebEngineView *view = currentWebView())
            view->back();
    });
    
    connect(forwardButton, &QPushButton::clicked, this, [this]() {
        if (QWebEngineView *view = currentWebView())
            view->forward();
    });
    
    connect(reloadButton, &QPushButton::clicked, this, [this]() {
        if (QWebEngineView *view = currentWebView())
            view->reload();
    });
    
    connect(homeButton, &QPushButton::clicked, this, [this]() {
        if (QWebEngineView *view = currentWebView())
            view->setUrl(QUrl(settings.value("homepage", "https://www.google.com").toString()));
    });
    
    connect(urlBar, &QLineEdit::returnPressed, this, [this]() {
        QString url = urlBar->text();
        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            // Если это не URL, выполняем поиск в Google
            if (!url.contains(".") || url.contains(" ")) {
                url = "https://www.google.com/search?q=" + QUrl::toPercentEncoding(url);
            } else {
                url = "https://" + url;
            }
        }
        if (QWebEngineView *view = currentWebView())
            view->setUrl(QUrl(url));
    });
    
    connect(bookmarkButton, &QPushButton::clicked, this, &MainWindow::addBookmark);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(readerButton, &QPushButton::clicked, this, [this]() {
        if (QWebEngineView *view = currentWebView()) {
            ReaderMode *reader = new ReaderMode(this);
            reader->setContent(view->page()->toHtml(), view->url());
            reader->show();
        }
    });
}

void MainWindow::navigateToUrl()
{
    QString url = urlBar->text();
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        url = "https://" + url;
    }
    webView->setUrl(QUrl(url));
}

void MainWindow::updateUrlBar(const QUrl &url)
{
    urlBar->setText(url.toString());
    addToHistory(url.toString(), webView->title());
}

void MainWindow::updateTitle(const QString &title)
{
    setWindowTitle(title + " - MyBrowser");
}

void MainWindow::goBack()
{
    webView->back();
}

void MainWindow::goForward()
{
    webView->forward();
}

void MainWindow::reload()
{
    webView->reload();
}

void MainWindow::zoomIn()
{
    currentZoom += 0.1;
    webView->setZoomFactor(currentZoom);
}

void MainWindow::zoomOut()
{
    currentZoom -= 0.1;
    if (currentZoom < 0.1) currentZoom = 0.1;
    webView->setZoomFactor(currentZoom);
}

void MainWindow::resetZoom()
{
    currentZoom = 1.0;
    webView->setZoomFactor(currentZoom);
}

void MainWindow::addBookmark()
{
    QString title = webView->title();
    QString url = webView->url().toString();
    
    Bookmark bookmark{url, title};
    bookmarks.append(bookmark);
    
    QAction *action = new QAction(title, this);
    connect(action, &QAction::triggered, this, [this, url]() {
        webView->setUrl(QUrl(url));
    });
    bookmarksMenu->addAction(action);
    
    settings.setValue("bookmarks", QVariant::fromValue(bookmarks));
}

void MainWindow::showHistory()
{
    QDialog dialog(this);
    dialog.setWindowTitle("История");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    for (const auto &entry : history) {
        QPushButton *button = new QPushButton(entry.title + "\n" + entry.url, &dialog);
        connect(button, &QPushButton::clicked, this, [this, entry]() {
            webView->setUrl(QUrl(entry.url));
        });
        layout->addWidget(button);
    }
    
    dialog.exec();
}

void MainWindow::clearHistory()
{
    history.clear();
    settings.remove("history");
}

void MainWindow::findInPage()
{
    findBar->show();
    findBar->setFocus();
}

void MainWindow::findNext()
{
    webView->findText(findBar->text());
}

void MainWindow::findPrevious()
{
    webView->findText(findBar->text(), QWebEnginePage::FindBackward);
}

void MainWindow::showSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Настройки");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLineEdit *homepageEdit = new QLineEdit(settings.value("homepage").toString(), &dialog);
    layout->addWidget(new QLabel("Домашняя страница:", &dialog));
    layout->addWidget(homepageEdit);
    
    QPushButton *saveButton = new QPushButton("Сохранить", &dialog);
    connect(saveButton, &QPushButton::clicked, this, [this, homepageEdit, &dialog]() {
        settings.setValue("homepage", homepageEdit->text());
        dialog.accept();
    });
    layout->addWidget(saveButton);
    
    dialog.exec();
}

void MainWindow::loadSettings()
{
    // Загружаем настройки интерфейса
    darkMode = settings.value("darkMode", false).toBool();
    currentZoom = settings.value("zoom", 1.0).toDouble();
    
    // Загружаем настройки браузера
    adBlockEnabled = settings.value("adBlock", false).toBool();
    javascriptEnabled = settings.value("javascript", true).toBool();
    imagesEnabled = settings.value("images", true).toBool();
    userAgent = settings.value("userAgent", webProfile->httpUserAgent()).toString();
    downloadPath = settings.value("downloadPath", QDir::homePath() + "/Downloads").toString();
    
    // Загружаем настройки прокси
    proxyEnabled = settings.value("proxyEnabled", false).toBool();
    proxyHost = settings.value("proxyHost", "").toString();
    proxyPort = settings.value("proxyPort", 8080).toInt();
    proxyUsername = settings.value("proxyUsername", "").toString();
    proxyPassword = settings.value("proxyPassword", "").toString();
    
    // Применяем настройки
    webView->setZoomFactor(currentZoom);
    webProfile->setHttpUserAgent(userAgent);
    webView->page()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, javascriptEnabled);
    webView->page()->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, imagesEnabled);
    
    if (adBlockEnabled) {
        webProfile->setUrlRequestInterceptor(new AdBlocker(this));
    }
    
    if (proxyEnabled) {
        toggleProxy();
    }
    
    applyStyle();
    
    // Загружаем историю
    QVariant historyVar = settings.value("history");
    if (historyVar.isValid()) {
        history = historyVar.value<QList<HistoryItem>>();
    }
    
    // Загружаем закладки
    QVariant bookmarksVar = settings.value("bookmarks");
    if (bookmarksVar.isValid()) {
        bookmarks = bookmarksVar.value<QList<Bookmark>>();
        for (const auto &bookmark : bookmarks) {
            QAction *action = new QAction(bookmark.title, this);
            connect(action, &QAction::triggered, this, [this, bookmark]() {
                webView->setUrl(QUrl(bookmark.url));
            });
            bookmarksMenu->addAction(action);
        }
    }
}

void MainWindow::saveSettings()
{
    settings.setValue("darkMode", darkMode);
    settings.setValue("zoom", currentZoom);
    settings.setValue("adBlock", adBlockEnabled);
    settings.setValue("javascript", javascriptEnabled);
    settings.setValue("images", imagesEnabled);
    settings.setValue("userAgent", userAgent);
    settings.setValue("downloadPath", downloadPath);
    settings.setValue("proxyEnabled", proxyEnabled);
    settings.setValue("proxyHost", proxyHost);
    settings.setValue("proxyPort", proxyPort);
    settings.setValue("proxyUsername", proxyUsername);
    settings.setValue("proxyPassword", proxyPassword);
    settings.setValue("history", QVariant::fromValue(history));
    settings.setValue("bookmarks", QVariant::fromValue(bookmarks));
    settings.sync();
}

void MainWindow::addToHistory(const QString &url, const QString &title)
{
    HistoryEntry entry{url, title, QDateTime::currentDateTime()};
    history.append(entry);
    
    // Ограничиваем историю до 1000 записей
    while (history.size() > 1000) {
        history.removeFirst();
    }
    
    settings.setValue("history", QVariant::fromValue(history));
}

void MainWindow::toggleDarkMode()
{
    darkMode = !darkMode;
    settings.setValue("darkMode", darkMode);
    applyStyle();
}

void MainWindow::toggleAdBlock()
{
    adBlockEnabled = !adBlockEnabled;
    settings.setValue("adBlock", adBlockEnabled);
    webProfile->setUrlRequestInterceptor(adBlockEnabled ? new AdBlocker(this) : nullptr);
}

void MainWindow::toggleJavaScript()
{
    javascriptEnabled = !javascriptEnabled;
    settings.setValue("javascript", javascriptEnabled);
    webView->page()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, javascriptEnabled);
}

void MainWindow::toggleImages()
{
    imagesEnabled = !imagesEnabled;
    settings.setValue("images", imagesEnabled);
    webView->page()->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, imagesEnabled);
}

void MainWindow::changeUserAgent()
{
    QString newAgent = QInputDialog::getText(this, "Изменить User Agent",
                                           "Введите новый User Agent:",
                                           QLineEdit::Normal,
                                           userAgent);
    if (!newAgent.isEmpty()) {
        userAgent = newAgent;
        settings.setValue("userAgent", userAgent);
        webProfile->setHttpUserAgent(userAgent);
    }
}

void MainWindow::clearBrowsingData()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Очистка данных",
                                "Вы уверены, что хотите очистить все данные браузера?\n"
                                "Это действие нельзя отменить.",
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        webProfile->clearAllVisitedLinks();
        webProfile->clearHttpCache();
        webProfile->cookieStore()->deleteAllCookies();
        history.clear();
        settings.remove("history");
        QMessageBox::information(this, "Очистка данных", "Данные браузера успешно очищены.");
    }
}

void MainWindow::exportBookmarks()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Экспорт закладок", QDir::homePath() + "/bookmarks.json", "JSON files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonArray bookmarksArray;
            for (const auto &bookmark : bookmarks) {
                QJsonObject bookmarkObj;
                bookmarkObj["url"] = bookmark.url;
                bookmarkObj["title"] = bookmark.title;
                bookmarksArray.append(bookmarkObj);
            }
            
            QJsonDocument doc(bookmarksArray);
            file.write(doc.toJson());
            QMessageBox::information(this, "Экспорт закладок", "Закладки успешно экспортированы.");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл закладок.");
        }
    }
}

void MainWindow::importBookmarks()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Импорт закладок", QDir::homePath(), "JSON files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonArray bookmarksArray = doc.array();
            
            for (const auto &bookmarkRef : bookmarksArray) {
                QJsonObject bookmarkObj = bookmarkRef.toObject();
                Bookmark bookmark;
                bookmark.url = bookmarkObj["url"].toString();
                bookmark.title = bookmarkObj["title"].toString();
                bookmarks.append(bookmark);
                
                QAction *action = new QAction(bookmark.title, this);
                connect(action, &QAction::triggered, this, [this, bookmark]() {
                    webView->setUrl(QUrl(bookmark.url));
                });
                bookmarksMenu->addAction(action);
            }
            
            settings.setValue("bookmarks", QVariant::fromValue(bookmarks));
            QMessageBox::information(this, "Импорт закладок", "Закладки успешно импортированы.");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл закладок.");
        }
    }
}

void MainWindow::changeDownloadPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для загрузок",
                                                  downloadPath,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        downloadPath = dir;
        settings.setValue("downloadPath", downloadPath);
    }
}

void MainWindow::toggleProxy()
{
    proxyEnabled = !proxyEnabled;
    settings.setValue("proxyEnabled", proxyEnabled);
    
    if (proxyEnabled) {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(proxyHost);
        proxy.setPort(proxyPort);
        if (!proxyUsername.isEmpty()) {
            proxy.setUser(proxyUsername);
            proxy.setPassword(proxyPassword);
        }
        QNetworkProxy::setApplicationProxy(proxy);
    } else {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
}

void MainWindow::setCustomProxy()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Настройки прокси");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLineEdit *hostEdit = new QLineEdit(proxyHost, &dialog);
    QSpinBox *portSpinBox = new QSpinBox(&dialog);
    portSpinBox->setRange(0, 65535);
    portSpinBox->setValue(proxyPort);
    QLineEdit *userEdit = new QLineEdit(proxyUsername, &dialog);
    QLineEdit *passEdit = new QLineEdit(proxyPassword, &dialog);
    passEdit->setEchoMode(QLineEdit::Password);
    
    layout->addWidget(new QLabel("Хост:", &dialog));
    layout->addWidget(hostEdit);
    layout->addWidget(new QLabel("Порт:", &dialog));
    layout->addWidget(portSpinBox);
    layout->addWidget(new QLabel("Пользователь:", &dialog));
    layout->addWidget(userEdit);
    layout->addWidget(new QLabel("Пароль:", &dialog));
    layout->addWidget(passEdit);
    
    QPushButton *saveButton = new QPushButton("Сохранить", &dialog);
    connect(saveButton, &QPushButton::clicked, &dialog, [&]() {
        proxyHost = hostEdit->text();
        proxyPort = portSpinBox->value();
        proxyUsername = userEdit->text();
        proxyPassword = passEdit->text();
        
        settings.setValue("proxyHost", proxyHost);
        settings.setValue("proxyPort", proxyPort);
        settings.setValue("proxyUsername", proxyUsername);
        settings.setValue("proxyPassword", proxyPassword);
        
        if (proxyEnabled) {
            toggleProxy(); // Применяем новые настройки
        }
        dialog.accept();
    });
    
    layout->addWidget(saveButton);
    dialog.exec();
}

void MainWindow::applyStyle()
{
    if (darkMode) {
        setStyleSheet(R"(
            QMainWindow {
                background-color: #1a1a1a;
                color: #ffffff;
            }
            QMenuBar {
                background-color: #2d2d2d;
                color: #ffffff;
            }
            QMenuBar::item:selected {
                background-color: #3d3d3d;
            }
            QMenu {
                background-color: #2d2d2d;
                color: #ffffff;
                border: 1px solid #3d3d3d;
            }
            QMenu::item:selected {
                background-color: #3d3d3d;
            }
            QToolBar {
                background-color: #2d2d2d;
                border: none;
            }
            QLineEdit {
                background-color: #2d2d2d;
                color: #ffffff;
                border: 1px solid #3d3d3d;
                padding: 5px;
            }
            QPushButton {
                background-color: #2d2d2d;
                color: #ffffff;
                border: 1px solid #3d3d3d;
                padding: 5px 10px;
            }
            QPushButton:hover {
                background-color: #3d3d3d;
            }
            QStatusBar {
                background-color: #2d2d2d;
                color: #ffffff;
            }
        )");
    } else {
        setStyleSheet("");
    }
}

void MainWindow::setupWebView()
{
    // Создаем профиль
    webProfile = new QWebEngineProfile("MyBrowser", this);
    webProfile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    webProfile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    webProfile->setHttpUserAgent(settings.value("userAgent", webProfile->httpUserAgent()).toString());

    // Настраиваем веб-страницу
    QWebEnginePage *page = new QWebEnginePage(webProfile, webView);
    webView->setPage(page);

    // Настраиваем параметры страницы
    QWebEngineSettings *settings = page->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, javascriptEnabled);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, imagesEnabled);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, false);
    settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);

    // Подключаем обработчики событий
    connect(webView, &QWebEngineView::loadStarted, this, [this]() {
        statusBar->showMessage("Загрузка...");
    });

    connect(webView, &QWebEngineView::loadProgress, this, [this](int progress) {
        statusBar->showMessage(QString("Загрузка: %1%").arg(progress));
    });

    connect(webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            statusBar->showMessage("Загрузка завершена", 3000);
        } else {
            statusBar->showMessage("Ошибка загрузки", 3000);
        }
    });

    // Добавляем контекстное меню
    webView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(webView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu *menu = new QMenu(this);
        
        menu->addAction("Назад", this, &MainWindow::goBack, QKeySequence::Back);
        menu->addAction("Вперед", this, &MainWindow::goForward, QKeySequence::Forward);
        menu->addAction("Обновить", this, &MainWindow::reload, QKeySequence::Refresh);
        menu->addSeparator();
        
        menu->addAction("Сохранить страницу", this, [this]() {
            QString fileName = QFileDialog::getSaveFileName(this, "Сохранить страницу",
                QDir::homePath() + "/" + webView->title() + ".html",
                "HTML Files (*.html *.htm);;All Files (*)");
            if (!fileName.isEmpty()) {
                webView->page()->save(fileName);
            }
        });
        
        menu->addAction("Сохранить как PDF", this, [this]() {
            QString fileName = QFileDialog::getSaveFileName(this, "Сохранить как PDF",
                QDir::homePath() + "/" + webView->title() + ".pdf",
                "PDF Files (*.pdf);;All Files (*)");
            if (!fileName.isEmpty()) {
                webView->page()->printToPdf(fileName);
            }
        });
        
        menu->addSeparator();
        menu->addAction("Добавить в закладки", this, &MainWindow::addBookmark);
        menu->addAction("Режим чтения", this, [this]() {
            // Создаем и показываем окно режима чтения
            ReaderMode *reader = new ReaderMode(this);
            reader->setContent(webView->page()->toHtml(), webView->url());
            reader->show();
        });
        
        menu->addSeparator();
        menu->addAction("Просмотреть исходный код", this, [this]() {
            webView->page()->toHtml([](const QString &html) {
                QDialog *dialog = new QDialog;
                dialog->setWindowTitle("Исходный код");
                dialog->resize(800, 600);
                
                QVBoxLayout *layout = new QVBoxLayout(dialog);
                QTextEdit *textEdit = new QTextEdit(dialog);
                textEdit->setReadOnly(true);
                textEdit->setPlainText(html);
                layout->addWidget(textEdit);
                
                dialog->exec();
                dialog->deleteLater();
            });
        });
        
        menu->addAction("Информация о странице", this, [this]() {
            QString info = QString(
                "URL: %1\n"
                "Заголовок: %2\n"
                "Защищенное соединение: %3"
            ).arg(webView->url().toString())
             .arg(webView->title())
             .arg(webView->url().scheme() == "https" ? "Да" : "Нет");
            
            QMessageBox::information(this, "Информация о странице", info);
        });
        
        menu->exec(webView->mapToGlobal(pos));
        menu->deleteLater();
    });
}

void MainWindow::addNewTab(const QUrl &url)
{
    QWebEngineView *webView = new QWebEngineView(this);
    setupTab(webView);
    webView->setUrl(url);
    int index = tabWidget->addTab(webView, "Новая вкладка");
    tabWidget->setCurrentIndex(index);
    webViews.append(webView);
}

void MainWindow::closeTab(int index)
{
    if (tabWidget->count() > 1) {
        QWebEngineView *webView = qobject_cast<QWebEngineView*>(tabWidget->widget(index));
        webViews.removeOne(webView);
        tabWidget->removeTab(index);
        delete webView;
    } else {
        close(); // Закрываем окно, если это последняя вкладка
    }
}

void MainWindow::duplicateTab()
{
    QWebEngineView *currentView = currentWebView();
    if (currentView) {
        QWebEngineView *newView = new QWebEngineView(this);
        setupTab(newView);
        newView->setUrl(currentView->url());
        int index = tabWidget->addTab(newView, currentView->title());
        tabWidget->setCurrentIndex(index);
        webViews.append(newView);
    }
}

void MainWindow::nextTab()
{
    int next = tabWidget->currentIndex() + 1;
    if (next >= tabWidget->count()) {
        next = 0;
    }
    tabWidget->setCurrentIndex(next);
}

void MainWindow::previousTab()
{
    int prev = tabWidget->currentIndex() - 1;
    if (prev < 0) {
        prev = tabWidget->count() - 1;
    }
    tabWidget->setCurrentIndex(prev);
}

void MainWindow::tabChanged(int index)
{
    if (index >= 0) {
        QWebEngineView *view = qobject_cast<QWebEngineView*>(tabWidget->widget(index));
        if (view) {
            urlBar->setText(view->url().toString());
            setWindowTitle(view->title() + " - MyBrowser");
        }
    }
}

void MainWindow::setTabTitle(const QString &title)
{
    int index = tabWidget->currentIndex();
    if (index >= 0) {
        tabWidget->setTabText(index, title);
        setWindowTitle(title + " - MyBrowser");
    }
}

void MainWindow::setTabIcon(const QIcon &icon)
{
    int index = tabWidget->currentIndex();
    if (index >= 0) {
        tabWidget->setTabIcon(index, icon);
    }
}

QWebEngineView* MainWindow::currentWebView() const
{
    return qobject_cast<QWebEngineView*>(tabWidget->currentWidget());
}

void MainWindow::setupTab(QWebEngineView *webView, const QString &title)
{
    // Настраиваем профиль и страницу
    QWebEnginePage *page = new QWebEnginePage(webProfile, webView);
    webView->setPage(page);

    // Подключаем сигналы
    connect(webView, &QWebEngineView::titleChanged, this, [this, webView](const QString &title) {
        int index = tabWidget->indexOf(webView);
        if (index >= 0) {
            tabWidget->setTabText(index, title);
            if (webView == currentWebView()) {
                setWindowTitle(title + " - MyBrowser");
            }
        }
    });

    connect(webView, &QWebEngineView::iconChanged, this, [this, webView](const QIcon &icon) {
        int index = tabWidget->indexOf(webView);
        if (index >= 0) {
            tabWidget->setTabIcon(index, icon);
        }
    });

    connect(webView, &QWebEngineView::urlChanged, this, [this, webView](const QUrl &url) {
        if (webView == currentWebView()) {
            urlBar->setText(url.toString());
            addToHistory(url.toString(), webView->title());
        }
    });

    // Добавляем контекстное меню
    webView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(webView, &QWidget::customContextMenuRequested, this, [this, webView](const QPoint &pos) {
        QMenu *menu = new QMenu(this);
        
        menu->addAction("Назад", webView, &QWebEngineView::back, QKeySequence::Back);
        menu->addAction("Вперед", webView, &QWebEngineView::forward, QKeySequence::Forward);
        menu->addAction("Обновить", webView, &QWebEngineView::reload, QKeySequence::Refresh);
        menu->addSeparator();
        
        menu->addAction("Новая вкладка", this, [this]() {
            addNewTab(QUrl("about:blank"));
        }, QKeySequence::AddTab);
        
        menu->addAction("Дублировать вкладку", this, &MainWindow::duplicateTab);
        menu->addAction("Закрыть вкладку", this, [this, webView]() {
            int index = tabWidget->indexOf(webView);
            if (index >= 0) {
                closeTab(index);
            }
        }, QKeySequence::Close);
        
        menu->addSeparator();
        menu->addAction("Добавить в закладки", this, &MainWindow::addBookmark);
        menu->addAction("Режим чтения", this, [this, webView]() {
            ReaderMode *reader = new ReaderMode(this);
            reader->setContent(webView->page()->toHtml(), webView->url());
            reader->show();
        });
        
        menu->exec(webView->mapToGlobal(pos));
        menu->deleteLater();
    });
} 
#include <QtTest>
#include <QWebEngineView>
#include <QWebEnginePage>
#include "mainwindow.h"
#include "browser.h"

class MainTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBrowserCreation();
    void testTabManagement();
    void testNavigation();
    void testAddressBar();
    void testBookmarkBar();
    void testContextMenu();
    void testShortcuts();
    void testSettings();
    void testWindowState();
    void cleanupTestCase();

private:
    MainWindow *window;
    Browser *browser;
};

void MainTest::initTestCase()
{
    window = new MainWindow();
    browser = window->getBrowser();
}

void MainTest::testBrowserCreation()
{
    QVERIFY(window != nullptr);
    QVERIFY(browser != nullptr);
    QVERIFY(window->isVisible());
}

void MainTest::testTabManagement()
{
    // Тестируем создание новой вкладки
    int initialCount = browser->tabCount();
    browser->newTab();
    QCOMPARE(browser->tabCount(), initialCount + 1);
    
    // Тестируем закрытие вкладки
    browser->closeTab(browser->currentIndex());
    QCOMPARE(browser->tabCount(), initialCount);
    
    // Тестируем дублирование вкладки
    browser->duplicateTab(browser->currentIndex());
    QCOMPARE(browser->tabCount(), initialCount + 1);
    
    // Тестируем перемещение вкладки
    int currentIndex = browser->currentIndex();
    browser->moveTab(currentIndex, 0);
    QCOMPARE(browser->currentIndex(), 0);
}

void MainTest::testNavigation()
{
    QWebEngineView *view = browser->currentView();
    QVERIFY(view != nullptr);
    
    // Тестируем загрузку страницы
    view->setUrl(QUrl("https://example.com"));
    QTRY_VERIFY_WITH_TIMEOUT(view->url().host() == "example.com", 5000);
    
    // Тестируем навигацию назад/вперед
    view->setUrl(QUrl("https://test.com"));
    QTRY_VERIFY_WITH_TIMEOUT(view->url().host() == "test.com", 5000);
    
    browser->back();
    QTRY_VERIFY_WITH_TIMEOUT(view->url().host() == "example.com", 5000);
    
    browser->forward();
    QTRY_VERIFY_WITH_TIMEOUT(view->url().host() == "test.com", 5000);
}

void MainTest::testAddressBar()
{
    // Тестируем ввод URL
    browser->setUrl(QUrl("https://example.com"));
    QCOMPARE(browser->url().host(), "example.com");
    
    // Тестируем поиск
    browser->search("test query");
    QVERIFY(browser->url().toString().contains("test+query"));
    
    // Тестируем автодополнение
    auto suggestions = browser->getUrlSuggestions("exam");
    QVERIFY(suggestions.contains("example.com"));
}

void MainTest::testBookmarkBar()
{
    // Тестируем видимость панели закладок
    browser->setBookmarkBarVisible(true);
    QVERIFY(browser->isBookmarkBarVisible());
    
    // Тестируем добавление закладки на панель
    browser->addBookmarkToBar("Test Bookmark", QUrl("https://test.com"));
    QVERIFY(browser->hasBookmarkInBar(QUrl("https://test.com")));
    
    // Тестируем удаление закладки с панели
    browser->removeBookmarkFromBar(QUrl("https://test.com"));
    QVERIFY(!browser->hasBookmarkInBar(QUrl("https://test.com")));
}

void MainTest::testContextMenu()
{
    QWebEngineView *view = browser->currentView();
    
    // Тестируем контекстное меню страницы
    QPoint pos(10, 10);
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos);
    
    // Проверяем создание меню
    QMenu *menu = view->createContextMenu();
    QVERIFY(menu != nullptr);
    delete menu;
}

void MainTest::testShortcuts()
{
    // Тестируем горячие клавиши
    QTest::keyClick(window, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(browser->tabCount(), 2);
    
    QTest::keyClick(window, Qt::Key_W, Qt::ControlModifier);
    QCOMPARE(browser->tabCount(), 1);
    
    QTest::keyClick(window, Qt::Key_L, Qt::ControlModifier);
    QVERIFY(window->isAddressBarFocused());
}

void MainTest::testSettings()
{
    // Тестируем сохранение настроек
    browser->setSetting("homepage", "https://example.com");
    QCOMPARE(browser->getSetting("homepage").toString(), "https://example.com");
    
    // Тестируем загрузку настроек
    browser->loadSettings();
    QCOMPARE(browser->getSetting("homepage").toString(), "https://example.com");
    
    // Тестируем сброс настроек
    browser->resetSettings();
    QVERIFY(browser->getSetting("homepage").toString() != "https://example.com");
}

void MainTest::testWindowState()
{
    // Тестируем полноэкранный режим
    window->setFullScreen(true);
    QVERIFY(window->isFullScreen());
    
    window->setFullScreen(false);
    QVERIFY(!window->isFullScreen());
    
    // Тестируем максимизацию
    window->showMaximized();
    QVERIFY(window->isMaximized());
    
    // Тестируем минимизацию
    window->showMinimized();
    QVERIFY(window->isMinimized());
    
    // Возвращаем нормальное состояние
    window->showNormal();
    QVERIFY(!window->isMaximized() && !window->isMinimized());
}

void MainTest::cleanupTestCase()
{
    delete window;
}

QTEST_MAIN(MainTest)
#include "main_test.moc" 
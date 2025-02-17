#include <QtTest>
#include "historymanager.h"

class HistoryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testAddVisit();
    void testRemoveVisit();
    void testClearHistory();
    void testSearchHistory();
    void testMostVisited();
    void testRecentlyVisited();
    void testMetadata();
    void cleanupTestCase();

private:
    HistoryManager *history;
    const QString testUrl = "https://test.com";
    const QString testTitle = "Test Page";
};

void HistoryTest::initTestCase()
{
    history = new HistoryManager(this);
}

void HistoryTest::testAddVisit()
{
    QVERIFY(history->addVisit(testUrl, testTitle));
    
    auto visits = history->getRecentlyVisited(1);
    QCOMPARE(visits.size(), 1);
    QCOMPARE(visits[0].url, testUrl);
    QCOMPARE(visits[0].title, testTitle);
}

void HistoryTest::testRemoveVisit()
{
    auto visits = history->getRecentlyVisited(1);
    QVERIFY(!visits.isEmpty());
    
    qint64 id = visits[0].id;
    QVERIFY(history->removeVisit(id));
    
    visits = history->getRecentlyVisited(1);
    QVERIFY(visits.isEmpty());
}

void HistoryTest::testClearHistory()
{
    history->addVisit(testUrl, testTitle);
    QVERIFY(!history->getRecentlyVisited(1).isEmpty());
    
    QVERIFY(history->clearHistory());
    QVERIFY(history->getRecentlyVisited(1).isEmpty());
}

void HistoryTest::testSearchHistory()
{
    history->addVisit("https://example.com", "Example Site");
    history->addVisit("https://test.com", "Test Site");
    
    auto results = history->searchHistory("example");
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].title, "Example Site");
}

void HistoryTest::testMostVisited()
{
    history->clearHistory();
    
    // Добавляем несколько посещений
    for(int i = 0; i < 3; i++) {
        history->addVisit(testUrl, testTitle);
    }
    history->addVisit("https://example.com", "Example");
    
    auto mostVisited = history->getMostVisited(2);
    QCOMPARE(mostVisited.size(), 2);
    QCOMPARE(mostVisited[0].url, testUrl);
    QCOMPARE(mostVisited[0].visitCount, 3);
}

void HistoryTest::testRecentlyVisited()
{
    history->clearHistory();
    
    QStringList urls = {"https://test1.com", "https://test2.com", "https://test3.com"};
    for(const auto &url : urls) {
        history->addVisit(url, "Test");
    }
    
    auto recent = history->getRecentlyVisited(3);
    QCOMPARE(recent.size(), 3);
    QCOMPARE(recent[0].url, urls[2]); // Последний добавленный должен быть первым
}

void HistoryTest::testMetadata()
{
    history->clearHistory();
    history->addVisit(testUrl, testTitle);
    
    auto visits = history->getRecentlyVisited(1);
    qint64 id = visits[0].id;
    
    QVERIFY(history->addMetadata(id, "test_key", "test_value"));
    QCOMPARE(history->getMetadata(id, "test_key").toString(), "test_value");
}

void HistoryTest::cleanupTestCase()
{
    history->clearHistory();
    delete history;
}

QTEST_MAIN(HistoryTest)
#include "history_test.moc" 
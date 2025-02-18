#include <QtTest>
#include "adblocker.h"

class AdBlockTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasicBlocking();
    void testCustomRules();
    void testWhitelist();
    void testFilterLists();
    void testStatistics();
    void testElementHiding();
    void cleanupTestCase();

private:
    AdBlocker *adblock;
    const QString testUrl = "https://ads.example.com/banner.jpg";
    const QString testDomain = "example.com";
};

void AdBlockTest::initTestCase()
{
    adblock = new AdBlocker(this);
    adblock->setEnabled(true);
}

void AdBlockTest::testBasicBlocking()
{
    QVERIFY(adblock->shouldBlock(QUrl(testUrl), "https://example.com"));
    QVERIFY(!adblock->shouldBlock(QUrl("https://example.com/image.jpg"), "https://example.com"));
}

void AdBlockTest::testCustomRules()
{
    QString customRule = "||custom-ads.com^";
    QVERIFY(adblock->addCustomRule(customRule));
    
    QVERIFY(adblock->shouldBlock(QUrl("https://custom-ads.com/ad.js"), "https://example.com"));
    
    QVERIFY(adblock->removeCustomRule(customRule));
    QVERIFY(!adblock->shouldBlock(QUrl("https://custom-ads.com/ad.js"), "https://example.com"));
}

void AdBlockTest::testWhitelist()
{
    QVERIFY(adblock->addToWhitelist(testDomain));
    QVERIFY(!adblock->shouldBlock(QUrl(testUrl), "https://" + testDomain));
    
    QVERIFY(adblock->removeFromWhitelist(testDomain));
    QVERIFY(adblock->shouldBlock(QUrl(testUrl), "https://" + testDomain));
}

void AdBlockTest::testFilterLists()
{
    QString listUrl = "https://easylist.to/easylist/easylist.txt";
    QVERIFY(adblock->addFilterList(listUrl));
    QVERIFY(adblock->enableFilterList(listUrl, true));
    
    // Проверяем, что правила из списка работают
    QVERIFY(adblock->shouldBlock(QUrl("https://doubleclick.net/ad.js"), "https://example.com"));
    
    QVERIFY(adblock->removeFilterList(listUrl));
}

void AdBlockTest::testStatistics()
{
    adblock->resetStatistics();
    
    // Генерируем несколько блокировок
    adblock->shouldBlock(QUrl(testUrl), "https://example.com");
    adblock->shouldBlock(QUrl("https://ads.test.com/ad.js"), "https://test.com");
    
    QVERIFY(adblock->getBlockedCount() > 0);
    
    auto topDomains = adblock->getTopBlockedDomains(2);
    QVERIFY(!topDomains.isEmpty());
}

void AdBlockTest::testElementHiding()
{
    QString testUrl = "https://example.com";
    
    // Проверяем CSS правила
    QString css = adblock->getCssRules(testUrl);
    QVERIFY(css.contains(".ad"));
    QVERIFY(css.contains("#banner"));
    
    // Проверяем JavaScript правила
    QString js = adblock->getJsRules(testUrl);
    QVERIFY(!js.isEmpty());
}

void AdBlockTest::cleanupTestCase()
{
    delete adblock;
}

QTEST_MAIN(AdBlockTest)
#include "adblock_test.moc" 
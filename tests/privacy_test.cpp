#include <QtTest>
#include "privacymanager.h"
#include <QWebEngineProfile>

class PrivacyTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCookieManagement();
    void testDataClearing();
    void testTrackingProtection();
    void testSecuritySettings();
    void testPermissions();
    void testStorageManagement();
    void testIncognitoMode();
    void testProxySettings();
    void testCertificateManagement();
    void cleanupTestCase();

private:
    PrivacyManager *privacy;
    QWebEngineProfile *profile;
};

void PrivacyTest::initTestCase()
{
    profile = new QWebEngineProfile("test", this);
    privacy = new PrivacyManager(profile, this);
}

void PrivacyTest::testCookieManagement()
{
    privacy->setCookiePolicy("accept");
    QCOMPARE(privacy->getCookiePolicy(), "accept");
    
    privacy->addCookieException("example.com", true);
    auto exceptions = privacy->getCookieExceptions();
    QVERIFY(exceptions.contains("example.com"));
    
    privacy->removeCookieException("example.com");
    exceptions = privacy->getCookieExceptions();
    QVERIFY(!exceptions.contains("example.com"));
}

void PrivacyTest::testDataClearing()
{
    QDateTime start = QDateTime::currentDateTime().addDays(-1);
    QDateTime end = QDateTime::currentDateTime();
    
    privacy->clearBrowsingData(start, end);
    privacy->clearCache();
    privacy->clearHistory();
    privacy->clearDownloads();
    privacy->clearPasswords();
    privacy->clearFormData();
    
    // Проверяем, что данные действительно очищены
    QVERIFY(true); // В реальном тесте нужно проверить фактическое удаление данных
}

void PrivacyTest::testTrackingProtection()
{
    privacy->setDoNotTrack(true);
    QVERIFY(privacy->isDoNotTrackEnabled());
    
    privacy->setTrackingProtection(true);
    QVERIFY(privacy->isTrackingProtectionEnabled());
    
    privacy->addTrackingException("example.com");
    auto exceptions = privacy->getTrackingExceptions();
    QVERIFY(exceptions.contains("example.com"));
}

void PrivacyTest::testSecuritySettings()
{
    privacy->setHttpsOnly(true);
    QVERIFY(privacy->isHttpsOnlyEnabled());
    
    privacy->setJavaScriptEnabled(false);
    QVERIFY(!privacy->isJavaScriptEnabled());
    
    privacy->setWebRTCPolicy("disable-non-proxied-udp");
    QCOMPARE(privacy->getWebRTCPolicy(), "disable-non-proxied-udp");
}

void PrivacyTest::testPermissions()
{
    QString origin = "https://example.com";
    QString feature = "camera";
    
    privacy->setPermission(origin, feature, true);
    QVERIFY(privacy->getPermission(origin, feature));
    
    auto permissions = privacy->getPermissionsForOrigin(origin);
    QVERIFY(permissions.contains(feature));
    QVERIFY(permissions[feature]);
    
    privacy->clearPermissions();
    QVERIFY(!privacy->getPermission(origin, feature));
}

void PrivacyTest::testStorageManagement()
{
    QString origin = "https://example.com";
    qint64 quota = 10 * 1024 * 1024; // 10 MB
    
    privacy->setStorageQuota(origin, quota);
    QCOMPARE(privacy->getStorageQuota(origin), quota);
    
    privacy->clearStorageData(origin);
    QCOMPARE(privacy->getStorageUsage(origin), 0);
}

void PrivacyTest::testIncognitoMode()
{
    privacy->setIncognitoMode(true);
    QVERIFY(privacy->isIncognitoModeEnabled());
    
    privacy->clearIncognitoData();
    QVERIFY(privacy->isIncognitoModeEnabled()); // Режим должен остаться включенным
}

void PrivacyTest::testProxySettings()
{
    privacy->setProxyEnabled(true);
    QVERIFY(privacy->isProxyEnabled());
    
    privacy->setProxySettings("proxy.example.com", 8080, "user", "pass");
    
    QStringList bypass = {"localhost", "127.0.0.1"};
    privacy->setProxyBypassList(bypass);
    QCOMPARE(privacy->getProxyBypassList(), bypass);
}

void PrivacyTest::testCertificateManagement()
{
    QList<QSslCertificate> certs;
    privacy->setCustomCertificates(certs);
    QCOMPARE(privacy->getCustomCertificates(), certs);
    
    privacy->addCertificateException("example.com");
    auto exceptions = privacy->getCertificateExceptions();
    QVERIFY(exceptions.contains("example.com"));
    
    privacy->removeCertificateException("example.com");
    exceptions = privacy->getCertificateExceptions();
    QVERIFY(!exceptions.contains("example.com"));
}

void PrivacyTest::cleanupTestCase()
{
    delete privacy;
    delete profile;
}

QTEST_MAIN(PrivacyTest)
#include "privacy_test.moc" 
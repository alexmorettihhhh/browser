#include <QtTest>
#include "certificatemanager.h"
#include <QSslCertificate>
#include <QSslKey>

class CertificateTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCertificateImport();
    void testCertificateExport();
    void testCertificateTrust();
    void testCertificateVerification();
    void testCertificateStorage();
    void testKeyManagement();
    void testCertificateChain();
    void testCertificateInfo();
    void testErrorHandling();
    void cleanupTestCase();

private:
    CertificateManager *certManager;
    const QString testCertPath = "test_cert.pem";
    const QString testKeyPath = "test_key.pem";
};

void CertificateTest::initTestCase()
{
    certManager = new CertificateManager(this);
}

void CertificateTest::testCertificateImport()
{
    // Тестируем импорт сертификата из файла
    QSslCertificate cert = QSslCertificate::fromPath(testCertPath).first();
    QVERIFY(!cert.isNull());
    
    qint64 certId = certManager->importCertificate(cert);
    QVERIFY(certId > 0);
    
    // Тестируем импорт сертификата из данных
    QByteArray certData = cert.toPem();
    certId = certManager->importCertificateFromData(certData);
    QVERIFY(certId > 0);
}

void CertificateTest::testCertificateExport()
{
    // Получаем сертификат для экспорта
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    
    // Тестируем экспорт в файл
    QVERIFY(certManager->exportCertificate(certs[0].id, "exported_cert.pem"));
    
    // Тестируем экспорт в данные
    QByteArray exportedData = certManager->exportCertificateToData(certs[0].id);
    QVERIFY(!exportedData.isEmpty());
}

void CertificateTest::testCertificateTrust()
{
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    qint64 certId = certs[0].id;
    
    // Тестируем установку доверия
    QVERIFY(certManager->trustCertificate(certId));
    QVERIFY(certManager->isCertificateTrusted(certId));
    
    // Тестируем отзыв доверия
    QVERIFY(certManager->untrustCertificate(certId));
    QVERIFY(!certManager->isCertificateTrusted(certId));
}

void CertificateTest::testCertificateVerification()
{
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    qint64 certId = certs[0].id;
    
    // Тестируем проверку срока действия
    QVERIFY(certManager->isValid(certId));
    
    // Тестируем проверку отзыва
    auto revocationStatus = certManager->checkRevocation(certId);
    QVERIFY(revocationStatus.contains("status"));
    
    // Тестируем проверку цепочки доверия
    auto chainStatus = certManager->verifyChain(certId);
    QVERIFY(chainStatus.contains("valid"));
}

void CertificateTest::testCertificateStorage()
{
    // Тестируем сохранение в хранилище
    QVERIFY(certManager->saveCertificateStore());
    
    // Тестируем загрузку из хранилища
    QVERIFY(certManager->loadCertificateStore());
    
    // Тестируем резервное копирование
    QVERIFY(certManager->backupCertificateStore("backup.dat"));
    
    // Тестируем восстановление
    QVERIFY(certManager->restoreCertificateStore("backup.dat"));
}

void CertificateTest::testKeyManagement()
{
    // Тестируем импорт закрытого ключа
    QSslKey key = QSslKey::fromPath(testKeyPath, QSsl::Rsa);
    QVERIFY(!key.isNull());
    
    qint64 keyId = certManager->importPrivateKey(key, "test-password");
    QVERIFY(keyId > 0);
    
    // Тестируем связывание ключа с сертификатом
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    QVERIFY(certManager->associateKeyWithCertificate(keyId, certs[0].id));
    
    // Тестируем получение ключа
    QSslKey retrievedKey = certManager->getPrivateKey(keyId, "test-password");
    QVERIFY(!retrievedKey.isNull());
}

void CertificateTest::testCertificateChain()
{
    // Тестируем построение цепочки
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    
    auto chain = certManager->buildCertificateChain(certs[0].id);
    QVERIFY(!chain.isEmpty());
    
    // Тестируем проверку цепочки
    auto validation = certManager->validateCertificateChain(chain);
    QVERIFY(validation.contains("valid"));
    
    // Тестируем экспорт цепочки
    QVERIFY(certManager->exportCertificateChain(certs[0].id, "chain.pem"));
}

void CertificateTest::testCertificateInfo()
{
    auto certs = certManager->getCertificates();
    QVERIFY(!certs.isEmpty());
    qint64 certId = certs[0].id;
    
    // Тестируем получение информации
    auto info = certManager->getCertificateInfo(certId);
    QVERIFY(info.contains("subject"));
    QVERIFY(info.contains("issuer"));
    QVERIFY(info.contains("validFrom"));
    QVERIFY(info.contains("validTo"));
    
    // Тестируем получение отпечатка
    QString fingerprint = certManager->getCertificateFingerprint(certId);
    QVERIFY(!fingerprint.isEmpty());
    
    // Тестируем проверку использования
    auto usages = certManager->getCertificateUsages(certId);
    QVERIFY(!usages.isEmpty());
}

void CertificateTest::testErrorHandling()
{
    // Тестируем импорт недействительного сертификата
    QVERIFY(!certManager->importCertificateFromData(QByteArray()));
    QVERIFY(!certManager->getLastError().isEmpty());
    
    // Тестируем доступ к несуществующему сертификату
    QVERIFY(!certManager->getCertificateInfo(-1).contains("subject"));
    QVERIFY(!certManager->getLastError().isEmpty());
    
    // Тестируем импорт с неверным паролем
    QSslKey key = QSslKey::fromPath(testKeyPath, QSsl::Rsa);
    QVERIFY(!certManager->importPrivateKey(key, "wrong-password"));
    QVERIFY(!certManager->getLastError().isEmpty());
}

void CertificateTest::cleanupTestCase()
{
    // Удаляем тестовые файлы
    QFile::remove("exported_cert.pem");
    QFile::remove("chain.pem");
    QFile::remove("backup.dat");
    
    delete certManager;
}

QTEST_MAIN(CertificateTest)
#include "certificate_test.moc" 
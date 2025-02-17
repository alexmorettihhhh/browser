#include <QtTest>
#include "readermode.h"
#include <QTextToSpeech>
#include <QPdfWriter>
#include <QTranslator>

class ReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testContentManagement();
    void testDisplaySettings();
    void testNavigation();
    void testSearch();
    void testExport();
    void testTextToSpeech();
    void testAnnotations();
    void testTranslation();
    void cleanupTestCase();

private:
    ReaderMode *reader;
    const QString testHtml = "<html><body><h1>Test Title</h1><p>Test content</p></body></html>";
    const QString testUrl = "https://example.com/article";
};

void ReaderTest::initTestCase()
{
    reader = new ReaderMode(this);
}

void ReaderTest::testContentManagement()
{
    QVERIFY(reader->setContent(testHtml, testUrl));
    QCOMPARE(reader->getContent(), testHtml);
    QCOMPARE(reader->getUrl(), testUrl);
    
    reader->clear();
    QVERIFY(reader->getContent().isEmpty());
}

void ReaderTest::testDisplaySettings()
{
    // Тестируем настройки темы
    reader->setTheme("dark");
    QCOMPARE(reader->getTheme(), "dark");
    
    // Тестируем настройки шрифта
    reader->setFontFamily("Arial");
    QCOMPARE(reader->getFontFamily(), "Arial");
    
    reader->setFontSize(16);
    QCOMPARE(reader->getFontSize(), 16);
    
    // Тестируем настройки макета
    reader->setLineHeight(1.5);
    QCOMPARE(reader->getLineHeight(), 1.5);
    
    reader->setMargins(20);
    QCOMPARE(reader->getMargins(), 20);
    
    reader->setColumnCount(2);
    QCOMPARE(reader->getColumnCount(), 2);
    
    reader->setTextAlignment("justify");
    QCOMPARE(reader->getTextAlignment(), "justify");
}

void ReaderTest::testNavigation()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем прокрутку
    reader->scrollToPosition(100);
    QCOMPARE(reader->getScrollPosition(), 100);
    
    // Тестируем навигацию по якорям
    reader->scrollToAnchor("test-anchor");
    
    // Тестируем проверку наличия следующей/предыдущей страницы
    QVERIFY(!reader->hasNextPage());
    QVERIFY(!reader->hasPreviousPage());
}

void ReaderTest::testSearch()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем поиск текста
    QVERIFY(reader->findText("Test content"));
    QVERIFY(!reader->findText("nonexistent text"));
    
    reader->clearSelection();
    
    QString selectedText = reader->getSelectedText();
    QVERIFY(selectedText.isEmpty());
}

void ReaderTest::testExport()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем экспорт в PDF
    QString pdfPath = "test.pdf";
    QVERIFY(reader->saveAsPdf(pdfPath));
    
    // Тестируем экспорт в HTML
    QString htmlPath = "test.html";
    QVERIFY(reader->saveAsHtml(htmlPath));
    
    // Тестируем экспорт в текст
    QString textPath = "test.txt";
    QVERIFY(reader->saveAsText(textPath));
    
    // Тестируем печать
    QVERIFY(reader->print());
}

void ReaderTest::testTextToSpeech()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем управление речью
    QVERIFY(reader->startSpeaking());
    QVERIFY(reader->isSpeaking());
    
    reader->pauseSpeaking();
    QVERIFY(!reader->isSpeaking());
    
    reader->resumeSpeaking();
    QVERIFY(reader->isSpeaking());
    
    reader->stopSpeaking();
    QVERIFY(!reader->isSpeaking());
    
    // Тестируем настройки речи
    reader->setSpeechRate(1.5);
    QCOMPARE(reader->getSpeechRate(), 1.5);
    
    reader->setSpeechPitch(1.2);
    QCOMPARE(reader->getSpeechPitch(), 1.2);
    
    reader->setSpeechVolume(0.8);
    QCOMPARE(reader->getSpeechVolume(), 0.8);
}

void ReaderTest::testAnnotations()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем добавление заметок
    int noteId = reader->addNote("Test note", 100, 200);
    QVERIFY(noteId > 0);
    
    // Тестируем получение заметок
    auto notes = reader->getNotes();
    QCOMPARE(notes.size(), 1);
    QCOMPARE(notes[0].text, "Test note");
    
    // Тестируем удаление заметок
    QVERIFY(reader->removeNote(noteId));
    notes = reader->getNotes();
    QVERIFY(notes.isEmpty());
    
    // Тестируем выделения
    int highlightId = reader->addHighlight(100, 200, "yellow");
    QVERIFY(highlightId > 0);
    
    auto highlights = reader->getHighlights();
    QCOMPARE(highlights.size(), 1);
    
    QVERIFY(reader->removeHighlight(highlightId));
    highlights = reader->getHighlights();
    QVERIFY(highlights.isEmpty());
}

void ReaderTest::testTranslation()
{
    reader->setContent(testHtml, testUrl);
    
    // Тестируем перевод текста
    QString sourceText = "Test content";
    QString targetLanguage = "ru";
    
    QString translatedText = reader->translateText(sourceText, targetLanguage);
    QVERIFY(!translatedText.isEmpty());
    QVERIFY(translatedText != sourceText);
    
    // Тестируем перевод всей страницы
    QVERIFY(reader->translatePage(targetLanguage));
    
    // Тестируем определение языка
    QString detectedLanguage = reader->detectLanguage(sourceText);
    QCOMPARE(detectedLanguage, "en");
}

void ReaderTest::cleanupTestCase()
{
    delete reader;
    
    // Удаляем тестовые файлы
    QFile::remove("test.pdf");
    QFile::remove("test.html");
    QFile::remove("test.txt");
}

QTEST_MAIN(ReaderTest)
#include "reader_test.moc" 
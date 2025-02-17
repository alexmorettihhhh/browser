#include <QtTest>
#include "bookmarkmanager.h"

class BookmarkTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testAddBookmark();
    void testRemoveBookmark();
    void testUpdateBookmark();
    void testFolders();
    void testTags();
    void testSearch();
    void testMetadata();
    void cleanupTestCase();

private:
    BookmarkManager *bookmarks;
    const QString testUrl = "https://test.com";
    const QString testTitle = "Test Page";
};

void BookmarkTest::initTestCase()
{
    bookmarks = new BookmarkManager(this);
}

void BookmarkTest::testAddBookmark()
{
    qint64 id = bookmarks->addBookmark(testUrl, testTitle);
    QVERIFY(id > 0);
    
    BookmarkItem item = bookmarks->getBookmark(id);
    QCOMPARE(item.url, testUrl);
    QCOMPARE(item.title, testTitle);
}

void BookmarkTest::testRemoveBookmark()
{
    qint64 id = bookmarks->addBookmark(testUrl, testTitle);
    QVERIFY(bookmarks->removeBookmark(id));
    
    BookmarkItem item = bookmarks->getBookmark(id);
    QVERIFY(item.id == 0); // Должен вернуть пустой элемент
}

void BookmarkTest::testUpdateBookmark()
{
    qint64 id = bookmarks->addBookmark(testUrl, testTitle);
    
    QHash<QString, QVariant> properties;
    properties["title"] = "Updated Title";
    properties["description"] = "Test Description";
    
    QVERIFY(bookmarks->updateBookmark(id, properties));
    
    BookmarkItem item = bookmarks->getBookmark(id);
    QCOMPARE(item.title, "Updated Title");
    QCOMPARE(item.description, "Test Description");
}

void BookmarkTest::testFolders()
{
    qint64 folderId = bookmarks->createFolder("Test Folder");
    QVERIFY(folderId > 0);
    
    qint64 bookmarkId = bookmarks->addBookmark(testUrl, testTitle);
    QVERIFY(bookmarks->moveBookmark(bookmarkId, "Test Folder"));
    
    auto folderBookmarks = bookmarks->getBookmarksByFolder("Test Folder");
    QCOMPARE(folderBookmarks.size(), 1);
    QCOMPARE(folderBookmarks[0].id, bookmarkId);
}

void BookmarkTest::testTags()
{
    qint64 id = bookmarks->addBookmark(testUrl, testTitle);
    
    QVERIFY(bookmarks->addTag(id, "test_tag"));
    QVERIFY(bookmarks->addTag(id, "another_tag"));
    
    auto taggedBookmarks = bookmarks->getBookmarksByTag("test_tag");
    QCOMPARE(taggedBookmarks.size(), 1);
    QCOMPARE(taggedBookmarks[0].id, id);
    
    QVERIFY(bookmarks->removeTag(id, "test_tag"));
    taggedBookmarks = bookmarks->getBookmarksByTag("test_tag");
    QVERIFY(taggedBookmarks.isEmpty());
}

void BookmarkTest::testSearch()
{
    bookmarks->addBookmark("https://example.com", "Example Site");
    bookmarks->addBookmark("https://test.com", "Test Site");
    
    auto results = bookmarks->searchBookmarks("example");
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].title, "Example Site");
}

void BookmarkTest::testMetadata()
{
    qint64 id = bookmarks->addBookmark(testUrl, testTitle);
    
    QVERIFY(bookmarks->setMetadata(id, "test_key", "test_value"));
    QCOMPARE(bookmarks->getMetadata(id, "test_key").toString(), "test_value");
}

void BookmarkTest::cleanupTestCase()
{
    delete bookmarks;
}

QTEST_MAIN(BookmarkTest)
#include "bookmark_test.moc" 
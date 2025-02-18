# API Reference

## Основные классы

### MainWindow
Главное окно браузера, управляет вкладками и основным интерфейсом.

### HistoryManager
Управление историей просмотров.

### BookmarkManager
Управление закладками.

### DownloadManager
Управление загрузками.

### AdBlocker
Блокировка рекламы и трекеров.

### PrivacyManager
Управление приватностью и безопасностью.

### ExtensionManager
Управление расширениями.

### CertificateManager
Управление сертификатами.

### SyncManager
Синхронизация данных.

### ReaderMode
Режим чтения.

## Сигналы и слоты

Каждый класс предоставляет набор сигналов и слотов для взаимодействия:

```cpp
// Пример сигналов HistoryManager
signals:
    void visitAdded(const HistoryItem &item);
    void visitRemoved(qint64 id);
    void historyCleared();

// Пример слотов MainWindow
public slots:
    void navigateToUrl();
    void updateUrlBar(const QUrl &url);
    void addBookmark();
```

## Структуры данных

```cpp
struct HistoryItem {
    qint64 id;
    QString url;
    QString title;
    QDateTime timestamp;
    int visitCount;
};

struct BookmarkItem {
    qint64 id;
    QString url;
    QString title;
    QString folder;
    QDateTime added;
};
```

## Примеры использования

### Добавление новой вкладки
```cpp
MainWindow *window = new MainWindow();
window->addNewTab(QUrl("https://www.google.com"));
```

### Управление историей
```cpp
HistoryManager *history = new HistoryManager();
history->addVisit("https://example.com", "Example Site");
QList<HistoryItem> items = history->getRecentlyVisited(10);
```

### Управление закладками
```cpp
BookmarkManager *bookmarks = new BookmarkManager();
qint64 id = bookmarks->addBookmark("https://example.com", "Example");
bookmarks->createFolder("Важное");
bookmarks->moveBookmark(id, "Важное");
``` 
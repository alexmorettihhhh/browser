#ifndef READERMODE_H
#define READERMODE_H

#include <QWidget>
#include <QWebEnginePage>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QTextToSpeech>
#include <QPdfWriter>
#include <QTranslator>

class ReaderMode : public QWidget
{
    Q_OBJECT

public:
    explicit ReaderMode(QWidget *parent = nullptr);
    ~ReaderMode();

    // Основные операции
    void setContent(const QString &html, const QUrl &baseUrl);
    void setUrl(const QUrl &url);
    QString getContent() const;
    void clear();
    
    // Настройки отображения
    void setTheme(const QString &theme);
    void setFont(const QString &family, int size);
    void setLineHeight(int height);
    void setMargins(int margin);
    void setColumnCount(int count);
    void setTextAlignment(Qt::Alignment alignment);
    
    // Навигация
    void scrollToPosition(int position);
    void scrollToAnchor(const QString &anchor);
    int getCurrentPosition() const;
    bool hasNextPage() const;
    bool hasPreviousPage() const;
    void nextPage();
    void previousPage();
    
    // Поиск
    void findText(const QString &text, QWebEnginePage::FindFlags flags = {});
    void clearSelection();
    bool hasSelection() const;
    QString getSelectedText() const;
    
    // Экспорт
    bool saveAsPdf(const QString &filename);
    bool saveAsHtml(const QString &filename);
    bool saveAsText(const QString &filename);
    bool printContent();
    
    // Дополнительные функции
    void startTextToSpeech();
    void stopTextToSpeech();
    void pauseTextToSpeech();
    void translateContent(const QString &targetLanguage);
    void addBookmark(int position);
    void removeBookmark(int position);
    QList<int> getBookmarks() const;
    
    // Аннотации
    void addHighlight(const QString &text, const QString &color);
    void addNote(const QString &text, const QString &note);
    void removeHighlight(const QString &text);
    void removeNote(const QString &text);
    QHash<QString, QString> getHighlights() const;
    QHash<QString, QString> getNotes() const;

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void toggleDarkMode();
    void toggleFullscreen();
    void toggleTableOfContents();
    void showSettings();

signals:
    void contentLoaded();
    void loadError(const QString &error);
    void positionChanged(int position);
    void selectionChanged();
    void bookmarkAdded(int position);
    void bookmarkRemoved(int position);
    void highlightAdded(const QString &text);
    void noteAdded(const QString &text);
    void themeChanged(const QString &theme);
    void fontChanged(const QString &family, int size);
    void readingProgressChanged(int percent);
    void textToSpeechStatusChanged(bool active);
    void translationCompleted();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void handleLinkClicked(const QUrl &url);
    void updateReadingProgress();
    void handleTextToSpeechStateChanged(QTextToSpeech::State state);
    void processTranslation(const QString &text);
    void saveReaderState();
    void restoreReaderState();

private:
    void createToolBar();
    void setupTextBrowser();
    QString processContent(const QString &html);
    QString injectStyles();
    QString extractMainContent(const QString &html);
    void updateLayout();
    void applyTheme(const QString &theme);
    void initTextToSpeech();
    void initTranslator();
    QString generateTableOfContents();
    void saveAnnotations();
    void loadAnnotations();
    
    QTextBrowser *m_textBrowser;
    QToolBar *m_toolbar;
    QComboBox *m_themeCombo;
    QComboBox *m_fontCombo;
    QSlider *m_fontSizeSlider;
    QSlider *m_lineHeightSlider;
    QSlider *m_marginSlider;
    QTextToSpeech *m_tts;
    QTranslator *m_translator;
    
    QString m_originalContent;
    QUrl m_baseUrl;
    double m_zoomFactor;
    bool m_darkMode;
    int m_columnCount;
    QList<int> m_bookmarks;
    QHash<QString, QString> m_highlights;
    QHash<QString, QString> m_notes;
    QHash<QString, QVariant> m_settings;
    
    static const QStringList SUPPORTED_THEMES;
    static const QStringList SUPPORTED_FONTS;
    static const int MIN_FONT_SIZE = 12;
    static const int MAX_FONT_SIZE = 32;
};

#endif // READERMODE_H 
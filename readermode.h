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

class ReaderMode : public QWidget
{
    Q_OBJECT

public:
    explicit ReaderMode(QWidget *parent = nullptr);
    void setContent(const QString &html, const QUrl &baseUrl);

private slots:
    void changeFontSize(int size);
    void changeTheme(int index);
    void changeFont(int index);
    void adjustLineHeight(int height);
    void adjustMargins(int margin);

private:
    QTextBrowser *m_textBrowser;
    QComboBox *m_themeCombo;
    QComboBox *m_fontCombo;
    QSlider *m_fontSizeSlider;
    QSlider *m_lineHeightSlider;
    QSlider *m_marginSlider;

    void createToolBar();
    void applyTheme(const QString &theme);
    QString processContent(const QString &html);
    QString injectStyles();
}; 
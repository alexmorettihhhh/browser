#include "readermode.h"
#include <QRegularExpression>
#include <QFontDatabase>
#include <QVBoxLayout>
#include <QPrinter>
#include <QPrintDialog>

ReaderMode::ReaderMode(QWidget *parent)
    : QWidget(parent)
    , m_webView(new QWebEngineView(this))
    , m_fontSize(16)
    , m_theme("light")
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    setLayout(layout);
}

ReaderMode::~ReaderMode()
{
}

void ReaderMode::setContent(const QString &html, const QString &baseUrl)
{
    m_originalContent = html;
    m_baseUrl = baseUrl;
    updateStyle();
}

void ReaderMode::setFontSize(int size)
{
    m_fontSize = size;
    updateStyle();
}

void ReaderMode::setTheme(const QString &theme)
{
    m_theme = theme;
    updateStyle();
}

void ReaderMode::print()
{
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_webView->page()->print(&printer, [](bool) {});
    }
}

void ReaderMode::zoomIn()
{
    m_webView->setZoomFactor(m_webView->zoomFactor() * 1.2);
}

void ReaderMode::zoomOut()
{
    m_webView->setZoomFactor(m_webView->zoomFactor() / 1.2);
}

void ReaderMode::resetZoom()
{
    m_webView->setZoomFactor(1.0);
}

void ReaderMode::updateStyle()
{
    QString styledHtml = injectReaderStyle(m_originalContent);
    m_webView->setHtml(styledHtml, QUrl(m_baseUrl));
}

QString ReaderMode::injectReaderStyle(const QString &html)
{
    QString backgroundColor = m_theme == "dark" ? "#1a1a1a" : "#ffffff";
    QString textColor = m_theme == "dark" ? "#ffffff" : "#000000";
    QString linkColor = m_theme == "dark" ? "#66b3ff" : "#0066cc";

    QString style = QString(R"(
        <style>
            body {
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
                font-size: %1px;
                line-height: 1.6;
                max-width: 800px;
                margin: 0 auto;
                padding: 20px;
                background-color: %2;
                color: %3;
            }
            a {
                color: %4;
                text-decoration: none;
            }
            a:hover {
                text-decoration: underline;
            }
            img {
                max-width: 100%;
                height: auto;
                display: block;
                margin: 20px auto;
            }
            h1, h2, h3, h4, h5, h6 {
                line-height: 1.3;
                margin-top: 1.5em;
                margin-bottom: 0.5em;
            }
            p {
                margin: 1em 0;
            }
            pre, code {
                background-color: %5;
                padding: 0.2em 0.4em;
                border-radius: 3px;
                font-family: 'Courier New', Courier, monospace;
            }
        </style>
    )").arg(
        QString::number(m_fontSize),
        backgroundColor,
        textColor,
        linkColor,
        m_theme == "dark" ? "#2d2d2d" : "#f5f5f5"
    );

    // Вставляем стили перед закрывающим тегом </head>
    if (html.contains("</head>", Qt::CaseInsensitive)) {
        return html.replace("</head>", style + "</head>", Qt::CaseInsensitive);
    } else {
        return "<html><head>" + style + "</head><body>" + html + "</body></html>";
    }
}

void ReaderMode::createToolBar()
{
    QToolBar *toolbar = new QToolBar(this);
    layout()->addWidget(toolbar);

    // Темы
    toolbar->addWidget(new QLabel("Тема: "));
    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItems({"Светлая", "Темная", "Сепия"});
    toolbar->addWidget(m_themeCombo);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReaderMode::changeTheme);

    toolbar->addSeparator();

    // Шрифты
    toolbar->addWidget(new QLabel("Шрифт: "));
    m_fontCombo = new QComboBox(this);
    m_fontCombo->addItems({"Open Sans", "Roboto", "Merriweather", "Georgia", "Arial"});
    toolbar->addWidget(m_fontCombo);
    connect(m_fontCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReaderMode::changeFont);

    toolbar->addSeparator();

    // Размер шрифта
    toolbar->addWidget(new QLabel("Размер: "));
    m_fontSizeSlider = new QSlider(Qt::Horizontal, this);
    m_fontSizeSlider->setRange(12, 24);
    m_fontSizeSlider->setValue(16);
    m_fontSizeSlider->setFixedWidth(100);
    toolbar->addWidget(m_fontSizeSlider);
    connect(m_fontSizeSlider, &QSlider::valueChanged,
            this, &ReaderMode::changeFontSize);

    toolbar->addSeparator();

    // Высота строки
    toolbar->addWidget(new QLabel("Интервал: "));
    m_lineHeightSlider = new QSlider(Qt::Horizontal, this);
    m_lineHeightSlider->setRange(100, 200);
    m_lineHeightSlider->setValue(150);
    m_lineHeightSlider->setFixedWidth(100);
    toolbar->addWidget(m_lineHeightSlider);
    connect(m_lineHeightSlider, &QSlider::valueChanged,
            this, &ReaderMode::adjustLineHeight);

    toolbar->addSeparator();

    // Отступы
    toolbar->addWidget(new QLabel("Отступы: "));
    m_marginSlider = new QSlider(Qt::Horizontal, this);
    m_marginSlider->setRange(0, 40);
    m_marginSlider->setValue(20);
    m_marginSlider->setFixedWidth(100);
    toolbar->addWidget(m_marginSlider);
    connect(m_marginSlider, &QSlider::valueChanged,
            this, &ReaderMode::adjustMargins);
}

void ReaderMode::changeFontSize(int size)
{
    m_fontSize = size;
    updateStyle();
}

void ReaderMode::changeTheme(int index)
{
    QString style;
    switch (index) {
    case 0: // Светлая
        style = "background-color: #ffffff; color: #000000;";
        break;
    case 1: // Темная
        style = "background-color: #1a1a1a; color: #ffffff;";
        break;
    case 2: // Сепия
        style = "background-color: #f4ecd8; color: #5b4636;";
        break;
    }
    m_theme = style.split(";").first().split(":").last().trimmed();
    updateStyle();
}

void ReaderMode::changeFont(int index)
{
    updateStyle();
}

void ReaderMode::adjustLineHeight(int height)
{
    updateStyle();
}

void ReaderMode::adjustMargins(int margin)
{
    updateStyle();
} 
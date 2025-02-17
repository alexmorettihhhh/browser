#include "readermode.h"
#include <QRegularExpression>
#include <QFontDatabase>

ReaderMode::ReaderMode(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    createToolBar();

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenLinks(false);
    m_textBrowser->setOpenExternalLinks(true);
    layout->addWidget(m_textBrowser);

    // Применяем начальные настройки
    changeFontSize(16);
    changeTheme(0);
    changeFont(0);
    adjustLineHeight(150);
    adjustMargins(20);
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

void ReaderMode::setContent(const QString &html, const QUrl &baseUrl)
{
    QString processedHtml = processContent(html);
    m_textBrowser->setHtml(processedHtml);
    m_textBrowser->setSource(baseUrl);
}

QString ReaderMode::processContent(const QString &html)
{
    // Удаляем ненужные элементы
    QString content = html;
    content.remove(QRegularExpression("<script.*?</script>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<style.*?</style>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<header.*?</header>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<footer.*?</footer>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<nav.*?</nav>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<aside.*?</aside>", QRegularExpression::DotMatchesEverythingOption));
    content.remove(QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption));

    // Добавляем наши стили
    return QString("<html><head><style>%1</style></head><body>%2</body></html>")
            .arg(injectStyles())
            .arg(content);
}

QString ReaderMode::injectStyles()
{
    return QString(R"(
        body {
            font-family: %1;
            font-size: %2px;
            line-height: %3%;
            margin: 0 %4px;
            padding: 20px;
            max-width: 800px;
            margin: 0 auto;
        }
        img {
            max-width: 100%;
            height: auto;
        }
        a {
            color: #0066cc;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
        h1, h2, h3, h4, h5, h6 {
            line-height: 1.2;
            margin-top: 1.5em;
            margin-bottom: 0.5em;
        }
        p {
            margin: 1em 0;
        }
    )")
    .arg(m_fontCombo->currentText())
    .arg(m_fontSizeSlider->value())
    .arg(m_lineHeightSlider->value())
    .arg(m_marginSlider->value());
}

void ReaderMode::changeFontSize(int size)
{
    m_textBrowser->setHtml(m_textBrowser->toHtml());
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
    m_textBrowser->setStyleSheet(style);
}

void ReaderMode::changeFont(int index)
{
    m_textBrowser->setHtml(m_textBrowser->toHtml());
}

void ReaderMode::adjustLineHeight(int height)
{
    m_textBrowser->setHtml(m_textBrowser->toHtml());
}

void ReaderMode::adjustMargins(int margin)
{
    m_textBrowser->setHtml(m_textBrowser->toHtml());
} 
#include "downloadsdialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDir>
#include <QHeaderView>

DownloadsDialog::DownloadsDialog(DownloadManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setWindowTitle("Загрузки");
    setMinimumSize(600, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Создаем представление для списка загрузок
    m_view = new QTreeView(this);
    m_view->setModel(m_manager->model());
    m_view->setAlternatingRowColors(true);
    m_view->setRootIsDecorated(false);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSortingEnabled(true);
    m_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(m_view);

    // Создаем кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    m_openFolderButton = new QPushButton("Открыть папку загрузок", this);
    m_clearButton = new QPushButton("Очистить завершенные", this);

    buttonLayout->addWidget(m_openFolderButton);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    // Подключаем сигналы
    connect(m_manager, &DownloadManager::downloadProgress,
            this, &DownloadsDialog::handleDownloadProgress);
    connect(m_manager, &DownloadManager::downloadCompleted,
            this, &DownloadsDialog::handleDownloadCompleted);
    connect(m_manager, &DownloadManager::downloadError,
            this, &DownloadsDialog::handleDownloadError);
    connect(m_openFolderButton, &QPushButton::clicked,
            this, &DownloadsDialog::openDownloadsFolder);
    connect(m_clearButton, &QPushButton::clicked,
            this, &DownloadsDialog::clearCompletedDownloads);
}

void DownloadsDialog::handleDownloadProgress(const QString &fileName, qint64 received, qint64 total)
{
    // Обновление происходит автоматически через модель
}

void DownloadsDialog::handleDownloadCompleted(const QString &fileName)
{
    QMessageBox::information(this, "Загрузка завершена",
                           QString("Файл %1 успешно загружен").arg(fileName));
}

void DownloadsDialog::handleDownloadError(const QString &fileName, const QString &error)
{
    QMessageBox::warning(this, "Ошибка загрузки",
                        QString("Ошибка при загрузке файла %1:\n%2").arg(fileName, error));
}

void DownloadsDialog::openDownloadsFolder()
{
    QString path = m_manager->model()->index(0, 0).data(Qt::UserRole).toString();
    if (path.isEmpty()) {
        path = QDir::homePath() + "/Downloads";
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void DownloadsDialog::clearCompletedDownloads()
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_view->model());
    if (!model) return;

    for (int i = model->rowCount() - 1; i >= 0; --i) {
        if (model->index(i, 4).data().toString() == "Завершено") {
            model->removeRow(i);
        }
    }
} 
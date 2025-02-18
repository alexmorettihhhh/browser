#ifndef DOWNLOADSDIALOG_H
#define DOWNLOADSDIALOG_H

#include <QDialog>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include "downloadmanager.h"

class DownloadsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadsDialog(DownloadManager *manager, QWidget *parent = nullptr);

private slots:
    void handleDownloadProgress(const QString &fileName, qint64 received, qint64 total);
    void handleDownloadCompleted(const QString &fileName);
    void handleDownloadError(const QString &fileName, const QString &error);
    void openDownloadsFolder();
    void clearCompletedDownloads();

private:
    DownloadManager *m_manager;
    QTreeView *m_view;
    QPushButton *m_openFolderButton;
    QPushButton *m_clearButton;
}; 
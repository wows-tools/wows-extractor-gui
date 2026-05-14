#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>

extern "C" {
#include "wows-depack.h"
}

class ArchiveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ArchiveWidget(QWidget *parent = nullptr);

    WOWS_CONTEXT *context() const { return m_context; }
    void setContext(WOWS_CONTEXT *ctx);

private slots:
    void onSelectOutputDir();
    void onExtractSelected();
    void onStopExtraction();
    void onSelectionChanged();
    void onRefreshTree();
    void onSearchFiles();
    void onItemExpanded(QTreeWidgetItem *item);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    WOWS_CONTEXT *m_context;
    bool          m_stopRequested;
    bool          m_extracting;
    QTreeWidget  *fileTree;
    QPushButton  *selectOutputDirButton;
    QPushButton  *extractButton;
    QPushButton  *stopButton;
    QPushButton  *refreshButton;
    QPushButton  *searchButton;
    QLineEdit    *outputDirPath;
    QLineEdit    *searchPattern;
    QProgressBar *progressBar;
    QLabel       *statusLabel;

    void setupUI();
    void populateFileTree();
    void populateDir(const QString &path, QTreeWidgetItem *parent);
    void collectFilePaths(const QString &path, QStringList &files);
    void extractFileList(const QStringList &files);
    void updateStatus(const QString &message);
    int  countFilesRecursive(const QString &path);
    QTreeWidgetItem *navigateToPath(const QString &path);
};

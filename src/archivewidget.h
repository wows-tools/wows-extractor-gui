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
    ~ArchiveWidget();

    WOWS_CONTEXT *context() const { return m_context; }

private slots:
    void onSelectIndexDir();
    void onSelectOutputDir();
    void onExtractSelected();
    void onRefreshTree();
    void onSearchFiles();

private:
    WOWS_CONTEXT *m_context;
    QTreeWidget  *fileTree;
    QPushButton  *selectIndexDirButton;
    QPushButton  *selectOutputDirButton;
    QPushButton  *extractButton;
    QPushButton  *refreshButton;
    QPushButton  *searchButton;
    QLineEdit    *indexDirPath;
    QLineEdit    *outputDirPath;
    QLineEdit    *searchPattern;
    QProgressBar *progressBar;
    QLabel       *statusLabel;

    void setupUI();
    void populateFileTree();
    void updateStatus(const QString &message);
    void clearContext();
};

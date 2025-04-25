#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QFileDialog>

extern "C" {
#include "wows-depack.h"
}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectIndexDir();
    void onSelectOutputDir();
    void onExtractSelected();
    void onRefreshTree();
    void onSearchFiles();

private:
    Ui::MainWindow *ui;
    WOWS_CONTEXT *context;
    QTreeWidget *fileTree;
    QPushButton *selectIndexDirButton;
    QPushButton *selectOutputDirButton;
    QPushButton *extractButton;
    QPushButton *refreshButton;
    QPushButton *searchButton;
    QLineEdit *indexDirPath;
    QLineEdit *outputDirPath;
    QLineEdit *searchPattern;
    QProgressBar *progressBar;
    QLabel *statusLabel;

    void setupUI();
    void populateFileTree();
    void updateStatus(const QString &message);
    void clearContext();
};

#endif // MAINWINDOW_H 
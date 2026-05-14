#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

extern "C" {
#include "wows-depack.h"
}

class ArchiveWidget;
class ShipViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void gameDataLoaded();

private slots:
    void onBrowseGameDir();
    void onLoadGameDir();

private:
    /* global game dir bar */
    QLineEdit    *gameDirEdit;
    QPushButton  *browseButton;
    QPushButton  *loadButton;
    QLabel       *loadStatus;

    QTabWidget       *tabs;
    ArchiveWidget    *archiveWidget;
    ShipViewerWidget *shipViewerWidget;

    WOWS_CONTEXT *m_context;

    void setupUI();
    void clearContext();
    static QString findLatestIdxDir(const QString &gameDir);
};

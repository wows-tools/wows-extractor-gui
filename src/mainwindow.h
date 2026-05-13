#pragma once
#include <QMainWindow>
#include <QTabWidget>

class ArchiveWidget;
class ShipViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTabWidget      *tabs;
    ArchiveWidget   *archiveWidget;
    ShipViewerWidget *shipViewerWidget;

    void setupUI();
};

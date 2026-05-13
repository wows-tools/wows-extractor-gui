#include "mainwindow.h"
#include "archivewidget.h"
#include "shipviewerwidget.h"

#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    setWindowTitle("WoWs Ship Browser");
    resize(1280, 800);

    tabs = new QTabWidget(this);
    archiveWidget = new ArchiveWidget(this);
    shipViewerWidget = new ShipViewerWidget(this);

    tabs->addTab(archiveWidget, "Archive Browser");
    tabs->addTab(shipViewerWidget, "Ship Viewer");

    setCentralWidget(tabs);

    /* when archive context is loaded, share it with ship viewer */
    connect(tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 1 && archiveWidget->context())
            shipViewerWidget->setArchiveContext(archiveWidget->context());
    });
}

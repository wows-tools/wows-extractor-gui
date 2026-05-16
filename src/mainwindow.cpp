#include "mainwindow.h"
#include "archivewidget.h"
#include "shipviewerwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QWidget>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_context(nullptr) {
    setupUI();

    /* restore last-used game dir — defer so the window renders before blocking load */
    QSettings s("wows-tools", "wows-extractor-gui");
    QString saved = s.value("gameDir").toString();
    if (!saved.isEmpty()) {
        gameDirEdit->setText(saved);
        loadStatus->setText("Loading…");
        loadStatus->setStyleSheet("color: #888;");
        browseButton->setEnabled(false);
        loadButton->setEnabled(false);
        QTimer::singleShot(0, this, &MainWindow::onLoadGameDir);
    } else {
        QTimer::singleShot(0, this, [this] { emit gameDataLoaded(); });
    }
}

MainWindow::~MainWindow() {
    clearContext();
}

void MainWindow::setupUI() {
    setWindowTitle(QStringLiteral("WoWs Extractor"));
    resize(1280, 800);

    /* ── central widget with vertical layout ── */
    QWidget *central = new QWidget(this);
    QVBoxLayout *vlay = new QVBoxLayout(central);
    vlay->setContentsMargins(6, 6, 6, 0);
    vlay->setSpacing(4);

    /* ── global game-directory bar ── */
    QHBoxLayout *dirBar = new QHBoxLayout();
    dirBar->addWidget(new QLabel("Game Directory:"));
    gameDirEdit = new QLineEdit();
    gameDirEdit->setPlaceholderText("Path to World of Warships installation…");
    dirBar->addWidget(gameDirEdit, 1);
    browseButton = new QPushButton("Browse…");
    browseButton->setFixedWidth(80);
    loadButton = new QPushButton("Load");
    loadButton->setFixedWidth(60);
    loadStatus = new QLabel("Not loaded");
    loadStatus->setStyleSheet("color: #888;");
    dirBar->addWidget(browseButton);
    dirBar->addWidget(loadButton);
    dirBar->addWidget(loadStatus);
    vlay->addLayout(dirBar);

    /* ── tabs ── */
    tabs = new QTabWidget(central);
    archiveWidget = new ArchiveWidget(this);
    shipViewerWidget = new ShipViewerWidget(this);
    tabs->addTab(shipViewerWidget, "Ship Viewer");
    tabs->addTab(archiveWidget, "Resource Explorer");
    vlay->addWidget(tabs, 1);

    setCentralWidget(central);

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseGameDir);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadGameDir);
    connect(gameDirEdit, &QLineEdit::returnPressed, this, &MainWindow::onLoadGameDir);
}

void MainWindow::onBrowseGameDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select World of Warships Directory", gameDirEdit->text());
    if (!dir.isEmpty()) {
        gameDirEdit->setText(dir);
        onLoadGameDir();
    }
}

/* Returns the path to the idx dir of the highest-numbered build, or "" on failure. */
QString MainWindow::findLatestIdxDir(const QString &gameDir) {
    QDir binDir(gameDir + "/bin");
    if (!binDir.exists())
        return {};

    const QStringList entries = binDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int maxBuild = -1;
    for (const QString &e : entries) {
        bool ok;
        int n = e.toInt(&ok);
        if (ok && n > maxBuild)
            maxBuild = n;
    }
    if (maxBuild < 0)
        return {};

    QString idxDir = gameDir + "/bin/" + QString::number(maxBuild) + "/idx";
    if (!QDir(idxDir).exists())
        return {};
    return idxDir;
}

void MainWindow::onLoadGameDir() {
    QString gameDir = gameDirEdit->text().trimmed();
    if (gameDir.isEmpty())
        return;

    browseButton->setEnabled(false);
    loadButton->setEnabled(false);

    auto finish = [this] {
        browseButton->setEnabled(true);
        loadButton->setEnabled(true);
        emit gameDataLoaded();
    };

    /* find the idx directory inside the latest build */
    QString idxDir = findLatestIdxDir(gameDir);
    if (idxDir.isEmpty()) {
        loadStatus->setText("No bin/<build>/idx found");
        loadStatus->setStyleSheet("color: red;");
        finish();
        return;
    }

    clearContext();
    m_context = wows_init_context(WOWS_NO_DEBUG);
    if (!m_context) {
        loadStatus->setText("Context init failed");
        loadStatus->setStyleSheet("color: red;");
        finish();
        return;
    }

    int ret = wows_parse_index_dir(idxDir.toUtf8().constData(), m_context);
    if (ret != 0) {
        loadStatus->setText(QString("Error: %1").arg(wows_error_string(ret, m_context)));
        loadStatus->setStyleSheet("color: red;");
        clearContext();
        finish();
        return;
    }

    /* push context to both tabs */
    archiveWidget->setContext(m_context);
    shipViewerWidget->setGameDirAndContext(gameDir, m_context);

    loadStatus->setText(
        QString("Loaded (build %1)")
            .arg(QDir(idxDir).dirName() == "idx" ? QDir(idxDir).absolutePath().section('/', -2, -2) : "?"));
    loadStatus->setStyleSheet("color: green;");

    /* persist for next launch */
    QSettings("wows-tools", "wows-extractor-gui").setValue("gameDir", gameDir);

    finish();
}

void MainWindow::clearContext() {
    if (m_context) {
        wows_free_context(m_context);
        m_context = nullptr;
    }
}

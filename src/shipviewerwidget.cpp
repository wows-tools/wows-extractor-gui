#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "shipviewerwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDir>
#include <QStandardPaths>
#include <QGroupBox>
#include <QApplication>
#include <set>

#include <cstdio>
#include <cstring>

ShipViewerWidget::ShipViewerWidget(QWidget *parent)
    : QWidget(parent)
    , m_archiveCtx(nullptr)
{
    setupUI();
}

ShipViewerWidget::~ShipViewerWidget() {}

void ShipViewerWidget::setArchiveContext(WOWS_CONTEXT *ctx)
{
    m_archiveCtx = ctx;
}

void ShipViewerWidget::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    /* ── left panel ─────────────────────────────── */
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setMinimumWidth(280);
    leftPanel->setMaximumWidth(340);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    /* game dir */
    QGroupBox *dirGroup = new QGroupBox("Game Directory", leftPanel);
    QVBoxLayout *dirLayout = new QVBoxLayout(dirGroup);
    gameDirPath = new QLineEdit(dirGroup);
    gameDirPath->setPlaceholderText("Path to extracted game content...");
    selectGameDirButton = new QPushButton("Browse...", dirGroup);
    loadShipsButton = new QPushButton("Load Ships", dirGroup);
    dirLayout->addWidget(gameDirPath);
    QHBoxLayout *dirBtnRow = new QHBoxLayout();
    dirBtnRow->addWidget(selectGameDirButton);
    dirBtnRow->addWidget(loadShipsButton);
    dirLayout->addLayout(dirBtnRow);
    leftLayout->addWidget(dirGroup);

    /* filters */
    QGroupBox *filterGroup = new QGroupBox("Filters", leftPanel);
    QVBoxLayout *filterLayout = new QVBoxLayout(filterGroup);
    searchLine = new QLineEdit(filterGroup);
    searchLine->setPlaceholderText("Search ship name...");
    nationFilter = new QComboBox(filterGroup);
    nationFilter->addItem("All Nations");
    typeFilter = new QComboBox(filterGroup);
    typeFilter->addItem("All Types");
    filterLayout->addWidget(new QLabel("Search:"));
    filterLayout->addWidget(searchLine);
    filterLayout->addWidget(new QLabel("Nation:"));
    filterLayout->addWidget(nationFilter);
    filterLayout->addWidget(new QLabel("Type:"));
    filterLayout->addWidget(typeFilter);
    leftLayout->addWidget(filterGroup);

    /* ship list */
    shipList = new QListWidget(leftPanel);
    leftLayout->addWidget(new QLabel("Ships:"));
    leftLayout->addWidget(shipList, 1);

    /* ── right panel ────────────────────────────── */
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    /* 3D viewer */
    viewer = new QQuickWidget(QUrl("qrc:/qml/viewer3d.qml"), rightPanel);
    viewer->setResizeMode(QQuickWidget::SizeRootObjectToView);
    viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewer->setAttribute(Qt::WA_AlwaysStackOnTop);
    rightLayout->addWidget(viewer, 1);

    /* export options */
    QGroupBox *exportGroup = new QGroupBox("Export Options", rightPanel);
    QHBoxLayout *exportLayout = new QHBoxLayout(exportGroup);
    withTurretsCheck = new QCheckBox("Turrets", exportGroup);
    withTurretsCheck->setChecked(true);
    withTexturesCheck = new QCheckBox("Textures", exportGroup);
    withTexturesCheck->setChecked(true);
    withDamageCheck = new QCheckBox("Damage geo", exportGroup);
    texSizeSpin = new QSpinBox(exportGroup);
    texSizeSpin->setRange(128, 4096);
    texSizeSpin->setValue(2048);
    texSizeSpin->setSingleStep(512);
    exportButton = new QPushButton("Export GLB...", exportGroup);
    exportLayout->addWidget(withTurretsCheck);
    exportLayout->addWidget(withTexturesCheck);
    exportLayout->addWidget(withDamageCheck);
    exportLayout->addWidget(new QLabel("Max Tex:"));
    exportLayout->addWidget(texSizeSpin);
    exportLayout->addStretch();
    exportLayout->addWidget(exportButton);
    rightLayout->addWidget(exportGroup);

    progressBar = new QProgressBar(rightPanel);
    progressBar->setRange(0, 0); /* indeterminate */
    progressBar->setVisible(false);
    statusLabel = new QLabel("Select a game directory and load ships.", rightPanel);
    rightLayout->addWidget(progressBar);
    rightLayout->addWidget(statusLabel);

    /* splitter */
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    /* connections */
    connect(selectGameDirButton, &QPushButton::clicked, this, &ShipViewerWidget::onSelectGameDir);
    connect(loadShipsButton, &QPushButton::clicked, this, &ShipViewerWidget::onLoadShips);
    connect(shipList, &QListWidget::itemDoubleClicked, this, &ShipViewerWidget::onShipSelected);
    connect(nationFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ShipViewerWidget::onNationFilterChanged);
    connect(typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ShipViewerWidget::onTypeFilterChanged);
    connect(exportButton, &QPushButton::clicked, this, &ShipViewerWidget::onExportShip);
    connect(searchLine, &QLineEdit::textChanged, this, &ShipViewerWidget::onSearchTextChanged);
}

void ShipViewerWidget::onSelectGameDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Game Directory");
    if (!dir.isEmpty())
        gameDirPath->setText(dir);
}

void ShipViewerWidget::extractGameParamsIfNeeded()
{
    QString gameDir = gameDirPath->text();
    if (gameDir == m_currentGameDir && !m_currentGameparamsPath.isEmpty())
        return;

    m_currentGameparamsPath.clear();
    m_currentAssetsBinPath.clear();
    m_currentGameDir = gameDir;

    if (m_archiveCtx) {
        /* extract GameParams.data from the archive to a temp file */
        int count = 0;
        char **results = nullptr;
        QByteArray pattern(".*GameParams\\.data");
        if (wows_search(m_archiveCtx, pattern.data(), WOWS_SEARCH_FULL_PATH,
                        &count, &results) == 0 && count > 0) {
            QTemporaryFile *tf = new QTemporaryFile(
                QDir::tempPath() + "/wows_gameparams_XXXXXX.data", this);
            if (tf->open()) {
                QByteArray archivePath(results[0]);
                FILE *fp = fdopen(tf->handle(), "wb");
                if (fp) {
                    if (wows_extract_file_fp(m_archiveCtx, archivePath.data(), fp) == 0)
                        m_currentGameparamsPath = tf->fileName();
                    fflush(fp);
                    /* don't fclose – QTemporaryFile owns the fd */
                }
            }
            for (int i = 0; i < count; i++) free(results[i]);
            free(results);
        }

        /* extract assets.bin */
        count = 0;
        results = nullptr;
        QByteArray abPattern(".*assets\\.bin");
        if (wows_search(m_archiveCtx, abPattern.data(), WOWS_SEARCH_FULL_PATH,
                        &count, &results) == 0 && count > 0) {
            QTemporaryFile *tf = new QTemporaryFile(
                QDir::tempPath() + "/wows_assets_XXXXXX.bin", this);
            if (tf->open()) {
                QByteArray archivePath(results[0]);
                FILE *fp = fdopen(tf->handle(), "wb");
                if (fp) {
                    if (wows_extract_file_fp(m_archiveCtx, archivePath.data(), fp) == 0)
                        m_currentAssetsBinPath = tf->fileName();
                    fflush(fp);
                }
            }
            for (int i = 0; i < count; i++) free(results[i]);
            free(results);
        }
    } else {
        /* filesystem: find GameParams.data under gameDir */
        std::string gp = wows_stitch_find_game_file(gameDir.toStdString(), "GameParams.data");
        if (!gp.empty()) m_currentGameparamsPath = QString::fromStdString(gp);
        std::string ab = wows_stitch_find_game_file(gameDir.toStdString(), "assets.bin");
        if (!ab.empty()) m_currentAssetsBinPath = QString::fromStdString(ab);
    }
}

void ShipViewerWidget::onLoadShips()
{
    if (gameDirPath->text().isEmpty() && !m_archiveCtx) {
        QMessageBox::warning(this, "Error", "Please select a game directory or load an archive first.");
        return;
    }

    updateStatus("Loading ships...");
    progressBar->setVisible(true);
    QApplication::processEvents();

    extractGameParamsIfNeeded();

    if (m_currentGameparamsPath.isEmpty()) {
        progressBar->setVisible(false);
        updateStatus("GameParams.data not found. Check game directory.");
        return;
    }

    m_ships.clear();
    Py_Initialize();
    bool ok = wows_list_ships(m_currentGameparamsPath.toUtf8().constData(), m_ships);
    Py_Finalize();

    if (!ok) {
        progressBar->setVisible(false);
        updateStatus("Failed to load ships from GameParams.data");
        return;
    }

    /* rebuild filter dropdowns */
    std::set<std::string> nations, types;
    for (const auto &s : m_ships) {
        nations.insert(s.nation);
        types.insert(s.type);
    }

    nationFilter->blockSignals(true);
    typeFilter->blockSignals(true);
    nationFilter->clear();
    typeFilter->clear();
    nationFilter->addItem("All Nations");
    typeFilter->addItem("All Types");
    for (const auto &n : nations) nationFilter->addItem(QString::fromStdString(n));
    for (const auto &t : types) typeFilter->addItem(QString::fromStdString(t));
    nationFilter->blockSignals(false);
    typeFilter->blockSignals(false);

    populateShipList();
    progressBar->setVisible(false);
    updateStatus(QString("Loaded %1 ships.").arg(m_ships.size()));
}

void ShipViewerWidget::populateShipList()
{
    applyFilters();
}

void ShipViewerWidget::applyFilters()
{
    shipList->clear();
    QString nationSel = nationFilter->currentIndex() > 0 ? nationFilter->currentText() : "";
    QString typeSel = typeFilter->currentIndex() > 0 ? typeFilter->currentText() : "";
    QString search = searchLine->text().toLower();

    for (const auto &s : m_ships) {
        if (!nationSel.isEmpty() && QString::fromStdString(s.nation) != nationSel)
            continue;
        if (!typeSel.isEmpty() && QString::fromStdString(s.type) != typeSel)
            continue;
        QString key = QString::fromStdString(s.key);
        if (!search.isEmpty() && !key.toLower().contains(search))
            continue;
        QString display = QString("%1 (%2, %3)")
                              .arg(key)
                              .arg(QString::fromStdString(s.nation))
                              .arg(QString::fromStdString(s.type));
        QListWidgetItem *item = new QListWidgetItem(display, shipList);
        item->setData(Qt::UserRole, key);
    }
}

void ShipViewerWidget::onNationFilterChanged() { applyFilters(); }
void ShipViewerWidget::onTypeFilterChanged() { applyFilters(); }
void ShipViewerWidget::onSearchTextChanged(const QString &) { applyFilters(); }

wows_file_provider_t ShipViewerWidget::makeFileProvider()
{
    if (!m_archiveCtx) return nullptr;
    WOWS_CONTEXT *ctx = m_archiveCtx;
    return [ctx](const std::string &path) -> std::vector<uint8_t> {
        char *buf = nullptr;
        size_t size = 0;
        FILE *f = open_memstream(&buf, &size);
        if (!f) return {};
        QByteArray ap = ("/" + QString::fromStdString(path)).toUtf8();
        int ret = wows_extract_file_fp(ctx, ap.data(), f);
        fclose(f);
        if (ret != 0 || !buf) { free(buf); return {}; }
        std::vector<uint8_t> result(reinterpret_cast<uint8_t *>(buf),
                                    reinterpret_cast<uint8_t *>(buf) + size);
        free(buf);
        return result;
    };
}

void ShipViewerWidget::onShipSelected(QListWidgetItem *item)
{
    if (!item) return;
    std::string shipKey = item->data(Qt::UserRole).toString().toStdString();
    loadAndDisplayShip(shipKey);
}

void ShipViewerWidget::loadAndDisplayShip(const std::string &shipKey)
{
    updateStatus(QString("Loading %1...").arg(QString::fromStdString(shipKey)));
    progressBar->setVisible(true);
    QApplication::processEvents();

    wows_ship_export_options opts;
    opts.with_turrets = withTurretsCheck->isChecked();
    opts.with_textures = false; /* skip textures for preview */
    opts.exclude_damage = !withDamageCheck->isChecked();
    opts.max_tex_size = texSizeSpin->value();
    if (!m_currentGameparamsPath.isEmpty())
        opts.gameparams_path = m_currentGameparamsPath.toStdString();
    if (!m_currentAssetsBinPath.isEmpty())
        opts.wows_assets_bin_path = m_currentAssetsBinPath.toStdString();

    std::vector<uint8_t> glbData;
    bool ok = wows_stitch_export_ship_to_glb_mem(
        m_currentGameDir.toStdString(),
        shipKey,
        glbData,
        opts,
        makeFileProvider()
    );

    if (!ok) {
        progressBar->setVisible(false);
        updateStatus(QString("Failed to load ship: %1").arg(QString::fromStdString(shipKey)));
        return;
    }

    /* write GLB to temp file for Qt Quick 3D */
    m_tempGlb.reset(new QTemporaryFile(QDir::tempPath() + "/wows_preview_XXXXXX.glb"));
    if (!m_tempGlb->open()) {
        progressBar->setVisible(false);
        updateStatus("Failed to create temp file for preview");
        return;
    }
    m_tempGlb->write(reinterpret_cast<const char *>(glbData.data()),
                     static_cast<qint64>(glbData.size()));
    m_tempGlb->flush();
    QString glbPath = "file:///" + m_tempGlb->fileName();

    /* update the QML viewer */
    if (QQuickItem *root = viewer->rootObject()) {
        root->setProperty("modelSource", glbPath);
    }

    progressBar->setVisible(false);
    updateStatus(QString("Displaying %1 (%2 KB)")
                     .arg(QString::fromStdString(shipKey))
                     .arg(glbData.size() / 1024));
}

void ShipViewerWidget::onExportShip()
{
    QListWidgetItem *item = shipList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Error", "Please select a ship first.");
        return;
    }
    std::string shipKey = item->data(Qt::UserRole).toString().toStdString();

    QString outPath = QFileDialog::getSaveFileName(
        this, "Export Ship GLB", QString::fromStdString(shipKey) + ".glb",
        "GLB files (*.glb)");
    if (outPath.isEmpty()) return;

    updateStatus(QString("Exporting %1...").arg(QString::fromStdString(shipKey)));
    progressBar->setVisible(true);
    QApplication::processEvents();

    wows_ship_export_options opts;
    opts.with_turrets = withTurretsCheck->isChecked();
    opts.with_textures = withTexturesCheck->isChecked();
    opts.exclude_damage = !withDamageCheck->isChecked();
    opts.max_tex_size = texSizeSpin->value();
    if (!m_currentGameparamsPath.isEmpty())
        opts.gameparams_path = m_currentGameparamsPath.toStdString();
    if (!m_currentAssetsBinPath.isEmpty())
        opts.wows_assets_bin_path = m_currentAssetsBinPath.toStdString();

    bool ok = wows_stitch_export_ship(
        m_currentGameDir.toStdString(), shipKey, outPath.toStdString(), opts);

    progressBar->setVisible(false);
    if (ok)
        updateStatus(QString("Exported to %1").arg(outPath));
    else
        updateStatus(QString("Export failed for %1").arg(QString::fromStdString(shipKey)));
}

void ShipViewerWidget::updateStatus(const QString &message)
{
    statusLabel->setText(message);
}

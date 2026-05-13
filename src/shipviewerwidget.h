#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QQuickWidget>
#include <QSplitter>
#include <QTemporaryFile>
#include <QTimer>

#include <vector>
#include <string>
#include <functional>
#include <memory>

extern "C" {
#include "wows-depack.h"
}

#include "wows-model-exporter.h"
#include "wows-game-params.h"

class ShipViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShipViewerWidget(QWidget *parent = nullptr);
    ~ShipViewerWidget();

    void setArchiveContext(WOWS_CONTEXT *ctx);

private slots:
    void onSelectGameDir();
    void onLoadShips();
    void onShipSelected(QListWidgetItem *item);
    void onNationFilterChanged();
    void onTypeFilterChanged();
    void onExportShip();
    void onSearchTextChanged(const QString &text);

private:
    /* left panel */
    QLineEdit    *gameDirPath;
    QPushButton  *selectGameDirButton;
    QPushButton  *loadShipsButton;
    QLineEdit    *searchLine;
    QComboBox    *nationFilter;
    QComboBox    *typeFilter;
    QListWidget  *shipList;

    /* right panel: 3D viewer */
    QQuickWidget *viewer;

    /* bottom: export options */
    QCheckBox  *withTurretsCheck;
    QCheckBox  *withTexturesCheck;
    QCheckBox  *withDamageCheck;
    QSpinBox   *texSizeSpin;
    QPushButton *exportButton;
    QProgressBar *progressBar;
    QLabel      *statusLabel;

    /* state */
    WOWS_CONTEXT *m_archiveCtx;
    std::vector<wows_ship_entry> m_ships;
    std::unique_ptr<QTemporaryFile> m_tempGlb;
    QString m_currentGameDir;
    QString m_currentGameparamsPath;
    QString m_currentAssetsBinPath;

    void setupUI();
    void populateShipList();
    void applyFilters();
    void loadAndDisplayShip(const std::string &shipKey);
    void extractGameParamsIfNeeded();
    void updateStatus(const QString &message);

    wows_file_provider_t makeFileProvider();
};

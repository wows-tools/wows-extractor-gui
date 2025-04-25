#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , context(nullptr)
{
    setupUI();
}

MainWindow::~MainWindow()
{
    clearContext();
    delete ui;
}

void MainWindow::setupUI()
{
    ui->setupUi(this);
    setWindowTitle("WoWs Archive Extractor");

    // Create main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Create input directory selection
    QHBoxLayout *indexDirLayout = new QHBoxLayout();
    indexDirPath = new QLineEdit(this);
    selectIndexDirButton = new QPushButton("Select Index Directory", this);
    indexDirLayout->addWidget(new QLabel("Index Directory:"));
    indexDirLayout->addWidget(indexDirPath);
    indexDirLayout->addWidget(selectIndexDirButton);

    // Create output directory selection
    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    outputDirPath = new QLineEdit(this);
    selectOutputDirButton = new QPushButton("Select Output Directory", this);
    outputDirLayout->addWidget(new QLabel("Output Directory:"));
    outputDirLayout->addWidget(outputDirPath);
    outputDirLayout->addWidget(selectOutputDirButton);

    // Create search controls
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchPattern = new QLineEdit(this);
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(new QLabel("Search Pattern:"));
    searchLayout->addWidget(searchPattern);
    searchLayout->addWidget(searchButton);

    // Create file tree
    fileTree = new QTreeWidget(this);
    fileTree->setHeaderLabel("Files");
    fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Create buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    extractButton = new QPushButton("Extract Selected", this);
    refreshButton = new QPushButton("Refresh", this);
    buttonLayout->addWidget(extractButton);
    buttonLayout->addWidget(refreshButton);

    // Create progress bar and status label
    progressBar = new QProgressBar(this);
    statusLabel = new QLabel(this);

    // Add all widgets to main layout
    mainLayout->addLayout(indexDirLayout);
    mainLayout->addLayout(outputDirLayout);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(fileTree);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    // Connect signals and slots
    connect(selectIndexDirButton, &QPushButton::clicked, this, &MainWindow::onSelectIndexDir);
    connect(selectOutputDirButton, &QPushButton::clicked, this, &MainWindow::onSelectOutputDir);
    connect(extractButton, &QPushButton::clicked, this, &MainWindow::onExtractSelected);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshTree);
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchFiles);

    updateStatus("Ready");
}

void MainWindow::onSelectIndexDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Index Directory");
    if (!dir.isEmpty()) {
        indexDirPath->setText(dir);
        clearContext();
        context = wows_init_context(WOWS_NO_DEBUG);
        if (context) {
            int result = wows_parse_index_dir(dir.toUtf8().constData(), context);
            if (result == 0) {
                populateFileTree();
                updateStatus("Index directory loaded successfully");
            } else {
                updateStatus(QString("Error loading index directory: %1").arg(wows_error_string(result, context)));
            }
        }
    }
}

void MainWindow::onSelectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!dir.isEmpty()) {
        outputDirPath->setText(dir);
    }
}

void MainWindow::onExtractSelected()
{
    if (!context || outputDirPath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select both index and output directories");
        return;
    }

    QList<QTreeWidgetItem*> selectedItems = fileTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select files to extract");
        return;
    }

    progressBar->setMaximum(selectedItems.size());
    progressBar->setValue(0);

    for (QTreeWidgetItem *item : selectedItems) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        QString outputPath = QDir(outputDirPath->text()).filePath(filePath);
        
        // Create a copy of the strings that we can modify
        QByteArray filePathBytes = filePath.toUtf8();
        QByteArray outputPathBytes = outputPath.toUtf8();
        
        int result = wows_extract_file(context, 
            filePathBytes.data(), 
            outputPathBytes.data());
            
        if (result != 0) {
            updateStatus(QString("Error extracting %1: %2").arg(filePath).arg(wows_error_string(result, context)));
        }
        
        progressBar->setValue(progressBar->value() + 1);
    }

    updateStatus("Extraction completed");
}

void MainWindow::onRefreshTree()
{
    if (context) {
        populateFileTree();
        updateStatus("File tree refreshed");
    }
}

void MainWindow::onSearchFiles()
{
    if (!context) {
        QMessageBox::warning(this, "Error", "Please select an index directory first");
        return;
    }

    QString pattern = searchPattern->text();
    if (pattern.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a search pattern");
        return;
    }

    int result_count;
    char **results;
    QByteArray patternBytes = pattern.toUtf8();
    int result = wows_search(context, patternBytes.data(), WOWS_SEARCH_FULL_PATH, &result_count, &results);
    
    if (result == 0 && result_count > 0) {
        fileTree->clear();
        for (int i = 0; i < result_count; i++) {
            QTreeWidgetItem *item = new QTreeWidgetItem(fileTree);
            QString path = QString::fromUtf8(results[i]);
            item->setText(0, path);
            item->setData(0, Qt::UserRole, path);
            free(results[i]);
        }
        free(results);
        updateStatus(QString("Found %1 matches").arg(result_count));
    } else {
        updateStatus("No matches found");
    }
}

void MainWindow::populateFileTree()
{
    if (!context) {
        updateStatus("Error: No archive loaded");
        return;
    }

    // Clear existing items
    fileTree->clear();

    // Get root directory contents
    int result_count = 0;
    char **results = nullptr;
    const char *root_path = "/";
    int ret = wows_readdir(context, const_cast<char*>(root_path), &result_count, &results);
    
    if (ret != 0) {
        updateStatus(QString("Error reading directory: %1").arg(wows_error_string(ret, context)));
        return;
    }

    // Create root item
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(fileTree);
    rootItem->setText(0, "/");
    rootItem->setData(0, Qt::UserRole, "/");
    rootItem->setIcon(0, QIcon::fromTheme("folder"));

    // Add all items to the tree
    for (int i = 0; i < result_count; i++) {
        QString entry = QString::fromUtf8(results[i]);
        bool isDir = entry.endsWith('/');
        if (isDir) {
            entry.chop(1); // Remove trailing slash
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(rootItem);
        item->setText(0, entry);
        item->setData(0, Qt::UserRole, "/" + entry);
        item->setIcon(0, QIcon::fromTheme(isDir ? "folder" : "text-x-generic"));
    }

    // Free the results array
    for (int i = 0; i < result_count; i++) {
        free(results[i]);
    }
    free(results);

    // Expand the root item
    rootItem->setExpanded(true);
    updateStatus("File tree populated successfully");
}

void MainWindow::updateStatus(const QString &message)
{
    statusLabel->setText(message);
}

void MainWindow::clearContext()
{
    if (context) {
        wows_free_context(context);
        context = nullptr;
    }
} 
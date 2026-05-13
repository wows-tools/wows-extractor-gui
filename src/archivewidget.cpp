#include "archivewidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QIcon>

ArchiveWidget::ArchiveWidget(QWidget *parent)
    : QWidget(parent), m_context(nullptr)
{
    setupUI();
}

ArchiveWidget::~ArchiveWidget()
{
    clearContext();
}

void ArchiveWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *indexDirLayout = new QHBoxLayout();
    indexDirPath = new QLineEdit(this);
    selectIndexDirButton = new QPushButton("Select Index Directory", this);
    indexDirLayout->addWidget(new QLabel("Index Directory:"));
    indexDirLayout->addWidget(indexDirPath);
    indexDirLayout->addWidget(selectIndexDirButton);

    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    outputDirPath = new QLineEdit(this);
    selectOutputDirButton = new QPushButton("Select Output Directory", this);
    outputDirLayout->addWidget(new QLabel("Output Directory:"));
    outputDirLayout->addWidget(outputDirPath);
    outputDirLayout->addWidget(selectOutputDirButton);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchPattern = new QLineEdit(this);
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(new QLabel("Search Pattern:"));
    searchLayout->addWidget(searchPattern);
    searchLayout->addWidget(searchButton);

    fileTree = new QTreeWidget(this);
    fileTree->setHeaderLabel("Files");
    fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    extractButton = new QPushButton("Extract Selected", this);
    refreshButton = new QPushButton("Refresh", this);
    buttonLayout->addWidget(extractButton);
    buttonLayout->addWidget(refreshButton);

    progressBar = new QProgressBar(this);
    statusLabel = new QLabel(this);

    mainLayout->addLayout(indexDirLayout);
    mainLayout->addLayout(outputDirLayout);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(fileTree);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(statusLabel);

    connect(selectIndexDirButton, &QPushButton::clicked, this, &ArchiveWidget::onSelectIndexDir);
    connect(selectOutputDirButton, &QPushButton::clicked, this, &ArchiveWidget::onSelectOutputDir);
    connect(extractButton, &QPushButton::clicked, this, &ArchiveWidget::onExtractSelected);
    connect(refreshButton, &QPushButton::clicked, this, &ArchiveWidget::onRefreshTree);
    connect(searchButton, &QPushButton::clicked, this, &ArchiveWidget::onSearchFiles);

    updateStatus("Ready");
}

void ArchiveWidget::onSelectIndexDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Index Directory");
    if (dir.isEmpty()) return;
    indexDirPath->setText(dir);
    clearContext();
    m_context = wows_init_context(WOWS_NO_DEBUG);
    if (!m_context) return;
    int result = wows_parse_index_dir(dir.toUtf8().constData(), m_context);
    if (result == 0) {
        populateFileTree();
        updateStatus("Index directory loaded successfully");
    } else {
        updateStatus(QString("Error loading index directory: %1")
                         .arg(wows_error_string(result, m_context)));
    }
}

void ArchiveWidget::onSelectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!dir.isEmpty())
        outputDirPath->setText(dir);
}

void ArchiveWidget::onExtractSelected()
{
    if (!m_context || outputDirPath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select both index and output directories");
        return;
    }
    QList<QTreeWidgetItem *> selectedItems = fileTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select files to extract");
        return;
    }
    progressBar->setMaximum(selectedItems.size());
    progressBar->setValue(0);
    for (QTreeWidgetItem *item : selectedItems) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        QString outputPath = QDir(outputDirPath->text()).filePath(filePath);
        QByteArray fp = filePath.toUtf8();
        QByteArray op = outputPath.toUtf8();
        int result = wows_extract_file(m_context, fp.data(), op.data());
        if (result != 0)
            updateStatus(QString("Error extracting %1: %2")
                             .arg(filePath)
                             .arg(wows_error_string(result, m_context)));
        progressBar->setValue(progressBar->value() + 1);
    }
    updateStatus("Extraction completed");
}

void ArchiveWidget::onRefreshTree()
{
    if (m_context) {
        populateFileTree();
        updateStatus("File tree refreshed");
    }
}

void ArchiveWidget::onSearchFiles()
{
    if (!m_context) {
        QMessageBox::warning(this, "Error", "Please select an index directory first");
        return;
    }
    QString pattern = searchPattern->text();
    if (pattern.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a search pattern");
        return;
    }
    int result_count = 0;
    char **results = nullptr;
    QByteArray patternBytes = pattern.toUtf8();
    int result = wows_search(m_context, patternBytes.data(), WOWS_SEARCH_FULL_PATH,
                             &result_count, &results);
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

void ArchiveWidget::populateFileTree()
{
    fileTree->clear();
    int result_count = 0;
    char **results = nullptr;
    const char *root_path = "/";
    int ret = wows_readdir(m_context, const_cast<char *>(root_path), &result_count, &results);
    if (ret != 0) {
        updateStatus(QString("Error reading directory: %1")
                         .arg(wows_error_string(ret, m_context)));
        return;
    }
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(fileTree);
    rootItem->setText(0, "/");
    rootItem->setData(0, Qt::UserRole, "/");
    rootItem->setIcon(0, QIcon::fromTheme("folder"));
    for (int i = 0; i < result_count; i++) {
        QString entry = QString::fromUtf8(results[i]);
        bool isDir = entry.endsWith('/');
        if (isDir) entry.chop(1);
        QTreeWidgetItem *item = new QTreeWidgetItem(rootItem);
        item->setText(0, entry);
        item->setData(0, Qt::UserRole, "/" + entry);
        item->setIcon(0, QIcon::fromTheme(isDir ? "folder" : "text-x-generic"));
        free(results[i]);
    }
    free(results);
    rootItem->setExpanded(true);
    updateStatus("File tree populated successfully");
}

void ArchiveWidget::updateStatus(const QString &message)
{
    statusLabel->setText(message);
}

void ArchiveWidget::clearContext()
{
    if (m_context) {
        wows_free_context(m_context);
        m_context = nullptr;
    }
}

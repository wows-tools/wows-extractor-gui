#include "archivewidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QIcon>

ArchiveWidget::ArchiveWidget(QWidget *parent)
    : QWidget(parent), m_context(nullptr), m_stopRequested(false), m_extracting(false)
{
    setupUI();
}

void ArchiveWidget::setContext(WOWS_CONTEXT *ctx)
{
    m_context = ctx;
    if (m_context)
        populateFileTree();
    else
        fileTree->clear();
    updateStatus(m_context ? "Archive loaded." : "No archive loaded.");
}

void ArchiveWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    outputDirPath = new QLineEdit(this);
    selectOutputDirButton = new QPushButton("Browse…", this);
    outputDirLayout->addWidget(new QLabel("Output Directory:"));
    outputDirLayout->addWidget(outputDirPath, 1);
    outputDirLayout->addWidget(selectOutputDirButton);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchPattern = new QLineEdit(this);
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(new QLabel("Search Pattern:"));
    searchLayout->addWidget(searchPattern, 1);
    searchLayout->addWidget(searchButton);

    fileTree = new QTreeWidget(this);
    fileTree->setHeaderLabel("Files");
    fileTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    extractButton = new QPushButton("Extract Selected", this);
    stopButton    = new QPushButton("Stop", this);
    refreshButton = new QPushButton("Refresh", this);
    stopButton->setEnabled(false);
    buttonLayout->addWidget(extractButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();

    progressBar = new QProgressBar(this);
    statusLabel = new QLabel("Load a game directory first.", this);

    mainLayout->addLayout(outputDirLayout);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(fileTree, 1);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(statusLabel);

    outputDirPath->setText(QSettings("wows-tools", "wows-depack-ui").value("outputDir").toString());
    connect(outputDirPath,         &QLineEdit::editingFinished,        this, [this]() {
        QSettings("wows-tools", "wows-depack-ui").setValue("outputDir", outputDirPath->text());
    });
    connect(selectOutputDirButton, &QPushButton::clicked,             this, &ArchiveWidget::onSelectOutputDir);
    connect(extractButton,         &QPushButton::clicked,             this, &ArchiveWidget::onExtractSelected);
    connect(stopButton,            &QPushButton::clicked,             this, &ArchiveWidget::onStopExtraction);
    connect(refreshButton,         &QPushButton::clicked,             this, &ArchiveWidget::onRefreshTree);
    connect(searchButton,          &QPushButton::clicked,             this, &ArchiveWidget::onSearchFiles);
    connect(fileTree,              &QTreeWidget::itemExpanded,        this, &ArchiveWidget::onItemExpanded);
    connect(fileTree,              &QTreeWidget::itemSelectionChanged,this, &ArchiveWidget::onSelectionChanged);
    connect(fileTree,              &QTreeWidget::itemDoubleClicked,   this, &ArchiveWidget::onItemDoubleClicked);
}

void ArchiveWidget::onSelectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
    if (!dir.isEmpty()) {
        outputDirPath->setText(dir);
        QSettings("wows-tools", "wows-depack-ui").setValue("outputDir", dir);
    }
}

void ArchiveWidget::onExtractSelected()
{
    if (m_extracting)
        return;
    if (!m_context) {
        QMessageBox::warning(this, "Error", "No archive loaded — use the game directory bar above.");
        return;
    }
    if (outputDirPath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select an output directory.");
        return;
    }
    QList<QTreeWidgetItem *> selectedItems = fileTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select files to extract.");
        return;
    }

    QString outDir = outputDirPath->text();

    // Flatten selection into individual file paths (recurses into directories)
    QStringList allFiles;
    for (QTreeWidgetItem *item : selectedItems) {
        if (item->data(0, Qt::UserRole + 1).toBool())
            collectFilePaths(item->data(0, Qt::UserRole).toString(), allFiles);
        else
            allFiles.append(item->data(0, Qt::UserRole).toString());
    }

    extractFileList(allFiles);
}

void ArchiveWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (m_extracting)
        return;
    if (!m_context)
        return;
    if (outputDirPath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select an output directory.");
        return;
    }
    QStringList files;
    if (item->data(0, Qt::UserRole + 1).toBool())
        collectFilePaths(item->data(0, Qt::UserRole).toString(), files);
    else
        files.append(item->data(0, Qt::UserRole).toString());
    extractFileList(files);
}

void ArchiveWidget::extractFileList(const QStringList &allFiles)
{
    QString outDir = outputDirPath->text();

    m_stopRequested = false;
    m_extracting = true;
    extractButton->setEnabled(false);
    stopButton->setEnabled(true);

    progressBar->setMaximum(allFiles.size());
    progressBar->setValue(0);
    int errors = 0;

    for (const QString &archivePath : allFiles) {
        if (m_stopRequested) {
            updateStatus(QString("Stopped — %1 / %2 file(s) extracted.")
                             .arg(progressBar->value())
                             .arg(allFiles.size()));
            break;
        }

        QString rel = archivePath.startsWith('/') ? archivePath.mid(1) : archivePath;
        QString outputPath = QDir(outDir).filePath(rel);
        QDir().mkpath(QFileInfo(outputPath).absolutePath());
        QByteArray fp = archivePath.toUtf8();
        QByteArray op = outputPath.toUtf8();

        updateStatus(QString("Extracting %1 / %2 — %3")
                         .arg(progressBar->value() + 1)
                         .arg(allFiles.size())
                         .arg(archivePath));

        int result = wows_extract_file(m_context, fp.data(), op.data());
        if (result != 0) {
            errors++;
            updateStatus(QString("Error extracting %1: %2")
                             .arg(archivePath)
                             .arg(wows_error_string(result, m_context)));
        }
        progressBar->setValue(progressBar->value() + 1);
        QApplication::processEvents();
    }

    m_extracting = false;
    extractButton->setEnabled(true);
    stopButton->setEnabled(false);

    if (!m_stopRequested) {
        if (errors == 0)
            updateStatus(QString("Extracted %1 file(s) successfully.").arg(allFiles.size()));
        else
            updateStatus(QString("Extraction done — %1 error(s).").arg(errors));
    }
}

void ArchiveWidget::onStopExtraction()
{
    m_stopRequested = true;
}

void ArchiveWidget::onSelectionChanged()
{
    if (!m_context)
        return;
    QList<QTreeWidgetItem *> selectedItems = fileTree->selectedItems();
    if (selectedItems.isEmpty()) {
        updateStatus("No files selected.");
        return;
    }
    int total = 0;
    for (QTreeWidgetItem *item : selectedItems) {
        if (item->data(0, Qt::UserRole + 1).toBool())
            total += countFilesRecursive(item->data(0, Qt::UserRole).toString());
        else
            total++;
    }
    updateStatus(QString("%1 file(s) selected.").arg(total));
}

void ArchiveWidget::collectFilePaths(const QString &path, QStringList &files)
{
    char **entries = nullptr;
    int entryCount = 0;
    QByteArray pathBytes = path.toUtf8();
    if (wows_readdir(m_context, pathBytes.data(), &entryCount, &entries) != 0)
        return;

    for (int i = 0; i < entryCount; i++) {
        QString name = QString::fromUtf8(entries[i]);
        QString full = (path == "/") ? "/" + name : path + "/" + name;
        WOWS_STAT st{};
        QByteArray fp = full.toUtf8();
        if (wows_stat_path(m_context, fp.data(), &st) == 0) {
            if (st.type == 0)
                collectFilePaths(full, files);
            else
                files.append(full);
        }
        free(entries[i]);
    }
    free(entries);
}

int ArchiveWidget::countFilesRecursive(const QString &path)
{
    int count = 0;
    char **entries = nullptr;
    int entryCount = 0;
    QByteArray pathBytes = path.toUtf8();
    if (wows_readdir(m_context, pathBytes.data(), &entryCount, &entries) != 0)
        return 0;

    for (int i = 0; i < entryCount; i++) {
        QString name = QString::fromUtf8(entries[i]);
        QString full = (path == "/") ? "/" + name : path + "/" + name;
        WOWS_STAT st{};
        QByteArray fp = full.toUtf8();
        if (wows_stat_path(m_context, fp.data(), &st) == 0)
            count += (st.type == 0) ? countFilesRecursive(full) : 1;
        free(entries[i]);
    }
    free(entries);
    return count;
}

void ArchiveWidget::onRefreshTree()
{
    if (m_context) {
        populateFileTree();
        updateStatus("File tree refreshed.");
    }
}

void ArchiveWidget::onSearchFiles()
{
    if (!m_context) {
        QMessageBox::warning(this, "Error", "No archive loaded — use the game directory bar above.");
        return;
    }
    QString input = searchPattern->text();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a search pattern.");
        return;
    }

    // Case-insensitive substring match: wrap user input with (?i).* … .*
    QString pattern = "(?i).*" + input + ".*";

    int result_count = 0;
    char **results = nullptr;
    QByteArray patternBytes = pattern.toUtf8();
    int ret = wows_search(m_context, patternBytes.data(), WOWS_SEARCH_FULL_PATH,
                          &result_count, &results);
    if (ret != 0 || result_count == 0) {
        updateStatus("No matches found.");
        return;
    }

    QString firstPath = QString::fromUtf8(results[0]);
    for (int i = 0; i < result_count; i++)
        free(results[i]);
    free(results);

    // Restore normal tree view if a previous search replaced it
    if (fileTree->topLevelItemCount() == 0 ||
        fileTree->topLevelItem(0)->data(0, Qt::UserRole).toString() != "/")
        populateFileTree();

    QTreeWidgetItem *item = navigateToPath(firstPath);
    if (item) {
        fileTree->clearSelection();
        item->setSelected(true);
        fileTree->scrollToItem(item);
        updateStatus(QString("Found %1 match(es) — first: %2").arg(result_count).arg(firstPath));
    } else {
        updateStatus(QString("Found %1 match(es) but could not navigate to first result.").arg(result_count));
    }
}

QTreeWidgetItem *ArchiveWidget::navigateToPath(const QString &path)
{
    if (fileTree->topLevelItemCount() == 0)
        return nullptr;

    QTreeWidgetItem *current = fileTree->topLevelItem(0); // the "/" root
    const QStringList parts = path.split('/', Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        // Force-load children if this directory only has a placeholder
        onItemExpanded(current);
        current->setExpanded(true);

        QTreeWidgetItem *found = nullptr;
        for (int i = 0; i < current->childCount(); i++) {
            if (current->child(i)->text(0) == part) {
                found = current->child(i);
                break;
            }
        }
        if (!found)
            return nullptr;
        current = found;
    }

    return current;
}

void ArchiveWidget::populateFileTree()
{
    fileTree->clear();
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(fileTree);
    rootItem->setText(0, "/");
    rootItem->setData(0, Qt::UserRole, QString("/"));
    rootItem->setData(0, Qt::UserRole + 1, true); /* isDir */
    rootItem->setIcon(0, QIcon::fromTheme("folder"));
    populateDir("/", rootItem);
    rootItem->setExpanded(true);
}

/* isDir flag is stored in UserRole+1 as bool; placeholder child has empty text */
void ArchiveWidget::populateDir(const QString &path, QTreeWidgetItem *parent)
{
    int count = 0;
    char **entries = nullptr;
    QByteArray pathBytes = path.toUtf8();
    int ret = wows_readdir(m_context, pathBytes.data(), &count, &entries);
    if (ret != 0) {
        updateStatus(QString("Error reading '%1': %2").arg(path).arg(wows_error_string(ret, m_context)));
        return;
    }

    /* sort: dirs first, then files, each group alphabetically */
    struct Entry { QString name; QString path; bool isDir; };
    QVector<Entry> items;
    items.reserve(count);

    for (int i = 0; i < count; i++) {
        QString name = QString::fromUtf8(entries[i]);
        QString full = (path == "/") ? "/" + name : path + "/" + name;
        WOWS_STAT st{};
        QByteArray fp = full.toUtf8();
        bool isDir = false;
        if (wows_stat_path(m_context, fp.data(), &st) == 0)
            isDir = (st.type == 0); /* WOWS_INODE_TYPE_DIR == 0 */
        items.push_back({name, full, isDir});
        free(entries[i]);
    }
    free(entries);

    std::sort(items.begin(), items.end(), [](const Entry &a, const Entry &b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        return a.name.toLower() < b.name.toLower();
    });

    for (const auto &e : items) {
        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, e.name);
        item->setData(0, Qt::UserRole,     e.path);
        item->setData(0, Qt::UserRole + 1, e.isDir);
        item->setIcon(0, QIcon::fromTheme(e.isDir ? "folder" : "text-x-generic"));
        if (e.isDir)
            new QTreeWidgetItem(item); /* placeholder so Qt shows expand arrow */
    }
}

void ArchiveWidget::onItemExpanded(QTreeWidgetItem *item)
{
    if (!item->data(0, Qt::UserRole + 1).toBool())
        return;
    /* loaded when there's exactly one placeholder (empty-text) child */
    if (item->childCount() != 1 || !item->child(0)->text(0).isEmpty())
        return;
    delete item->takeChild(0);
    populateDir(item->data(0, Qt::UserRole).toString(), item);
}

void ArchiveWidget::updateStatus(const QString &message)
{
    statusLabel->setText(message);
}

#include "arete/widgets/LogsPage.h"
#include "arete/logging/Logger.h"
#include "arete/theme/Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QDateTime>
#include <QCheckBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QTimer>

namespace arete::widgets {

class LogsPage::Private
{
public:
    logging::Logger* logger = nullptr;
    QTableWidget* table = nullptr;
    QComboBox* levelFilter = nullptr;
    QLineEdit* searchBox = nullptr;
    QPushButton* clearButton = nullptr;
    QPushButton* exportButton = nullptr;
    QLabel* countLabel = nullptr;
    QTimer* refreshTimer = nullptr;
    QVector<logging::LogEntry> entries;
    int lastCount = -1;
};


LogsPage::~LogsPage() = default;

LogsPage::LogsPage(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    // Toolbar row
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    auto* heading = new QLabel(QStringLiteral("Logs"), this);
    heading->setProperty("heading", true);
    heading->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    toolbar->addWidget(heading);
    toolbar->addStretch();

    toolbar->addWidget(new QLabel(QStringLiteral("Level:"), this));
    d->levelFilter = new QComboBox(this);
    d->levelFilter->addItem(QStringLiteral("All"));
    d->levelFilter->addItem(QStringLiteral("Warnings+"));
    d->levelFilter->addItem(QStringLiteral("Failures+"));
    d->levelFilter->addItem(QStringLiteral("Errors only"));
    toolbar->addWidget(d->levelFilter);

    d->searchBox = new QLineEdit(this);
    d->searchBox->setPlaceholderText(QStringLiteral("Search logs…"));
    d->searchBox->setClearButtonEnabled(true);
    d->searchBox->setFixedWidth(220);
    toolbar->addWidget(d->searchBox);

    d->exportButton = new QPushButton(QStringLiteral("Export"), this);
    toolbar->addWidget(d->exportButton);

    d->clearButton = new QPushButton(QStringLiteral("Clear"), this);
    toolbar->addWidget(d->clearButton);

    rootLayout->addLayout(toolbar);

    // Table
    d->table = new QTableWidget(this);
    d->table->setColumnCount(5);
    d->table->setHorizontalHeaderLabels({QStringLiteral("Time"), QStringLiteral("Level"),
                                         QStringLiteral("Category"), QStringLiteral("Message"),
                                         QStringLiteral("Location")});
    d->table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    d->table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d->table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    d->table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    d->table->verticalHeader()->setVisible(false);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->table->setAlternatingRowColors(true);
    d->table->setStyleSheet(QStringLiteral(
        "QTableWidget { gridline-color: #353535; background-color: #111111; }"
        "QTableWidget::item:selected { background-color: #3A3A3A; color: #D0D0D0; }"
        "QTableWidget::item { padding: 3px 6px; }"));
    rootLayout->addWidget(d->table, 1);

    // Footer
    auto* footer = new QHBoxLayout;
    d->countLabel = new QLabel(QStringLiteral("0 entries"), this);
    d->countLabel->setStyleSheet(QStringLiteral("color: #A0A0A0;"));
    footer->addWidget(d->countLabel);
    footer->addStretch();
    rootLayout->addLayout(footer);

    // Log level colors
    connect(d->levelFilter, &QComboBox::currentIndexChanged, this, [this](int) { refreshEntries(); });
    connect(d->searchBox, &QLineEdit::textChanged, this, [this](const QString&) { refreshEntries(); });

    connect(d->clearButton, &QPushButton::clicked, this, [this]() {
        if (d->logger) {
            d->logger->clear();
            d->entries.clear();
            refreshEntries();
            emit logsCleared();
        }
    });

    connect(d->exportButton, &QPushButton::clicked, this, [this]() {
        if (!d->logger) return;
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export Logs"),
            QStringLiteral("arete-logs.json"), QStringLiteral("JSON Files (*.json)"));
        if (path.isEmpty()) return;
        if (d->logger->exportToFile(path)) {
            emit logsExported(path);
        }
    });

    // Periodic refresh (keeps the view live without heavy signal coupling)
    d->refreshTimer = new QTimer(this);
    d->refreshTimer->setInterval(1000);
    connect(d->refreshTimer, &QTimer::timeout, this, [this]() { refreshEntries(); });
    d->refreshTimer->start();

    setLogger(logging::Logger::instancePtr());
    refreshEntries();
}

void LogsPage::setLogger(logging::Logger* logger)
{
    d->logger = logger;
    if (logger) {
        connect(logger->model(), &logging::LogModel::entriesChanged,
                this, [this]() { refreshEntries(); }, Qt::UniqueConnection);
    }
    refreshEntries();
}

void LogsPage::setCategoryFilter(const QString& category, bool enabled)
{
    if (d->logger) {
        d->logger->setCategoryFilter(category, enabled);
    }
    refreshEntries();
}

void LogsPage::showOnlyWarnings(bool on)
{
    d->levelFilter->setCurrentIndex(on ? 1 : 0);
}

void LogsPage::showOnlyErrors(bool on)
{
    d->levelFilter->setCurrentIndex(on ? 3 : 0);
}

void LogsPage::clearAll()
{
    if (d->logger) {
        d->logger->clear();
    }
    d->entries.clear();
    refreshEntries();
    emit logsCleared();
}

int LogsPage::entryCount() const
{
    return d->entries.size();
}

void LogsPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshEntries();
}

void LogsPage::refreshEntries()
{
    if (!d->logger) return;

    const auto all = d->logger->model()->entries();
    const int levelFilter = d->levelFilter ? d->levelFilter->currentIndex() : 0;
    const QString query = d->searchBox ? d->searchBox->text().trimmed().toLower() : QString();

    d->entries.clear();
    d->entries.reserve(all.size());
    for (const auto& entry : all) {
        switch (levelFilter) {
            case 1: if (entry.level < logging::Level::Warning) continue; break;
            case 2: if (entry.level < logging::Level::Error) continue; break;
            case 3: if (entry.level != logging::Level::Error && entry.level != logging::Level::Critical) continue; break;
            default: break;
        }
        if (!query.isEmpty()) {
            if (!entry.message.toLower().contains(query) &&
                !entry.category.toLower().contains(query)) {
                continue;
            }
        }
        d->entries.append(entry);
    }

    d->table->setRowCount(0);
    d->table->setRowCount(d->entries.size());
    for (int row = 0; row < d->entries.size(); ++row) {
        const auto& entry = d->entries[row];
        auto* timeItem = new QTableWidgetItem(entry.timestamp.toString(QStringLiteral("hh:mm:ss")));
        auto* levelItem = new QTableWidgetItem([&entry]() {
            switch (entry.level) {
                case logging::Level::Trace: return QStringLiteral("TRACE");
                case logging::Level::Debug: return QStringLiteral("DEBUG");
                case logging::Level::Info: return QStringLiteral("INFO");
                case logging::Level::Warning: return QStringLiteral("WARN");
                case logging::Level::Error: return QStringLiteral("ERROR");
                case logging::Level::Critical: return QStringLiteral("CRIT");
            }
            return QStringLiteral("INFO");
        }());
        auto* categoryItem = new QTableWidgetItem(entry.category);
        auto* messageItem = new QTableWidgetItem(entry.message);
        auto* locationItem = new QTableWidgetItem(
            entry.function.isEmpty() ? QString() :
            QStringLiteral("%1:%2").arg(QFileInfo(entry.function).fileName()).arg(entry.line));

        QColor levelColor;
        switch (entry.level) {
            case logging::Level::Trace:
            case logging::Level::Debug: levelColor = QColor(0x8A, 0x8A, 0x8A); break;
            case logging::Level::Info: levelColor = QColor(0xA0, 0xA0, 0xA0); break;
            case logging::Level::Warning: levelColor = QColor(0xC4, 0xA0, 0x50); break;
            case logging::Level::Error: levelColor = QColor(0xC4, 0x50, 0x50); break;
            case logging::Level::Critical: levelColor = QColor(0xFF, 0x60, 0x60); break;
        }
        levelItem->setForeground(levelColor);
        messageItem->setForeground(levelColor == QColor(0xC4, 0x50, 0x50) ||
                                   levelColor == QColor(0xFF, 0x60, 0x60)
                                       ? levelColor
                                       : QColor(0xA0, 0xA0, 0xA0));

        d->table->setItem(row, 0, timeItem);
        d->table->setItem(row, 1, levelItem);
        d->table->setItem(row, 2, categoryItem);
        d->table->setItem(row, 3, messageItem);
        d->table->setItem(row, 4, locationItem);
    }

    if (d->countLabel) {
        d->countLabel->setText(QStringLiteral("%1 entries").arg(d->entries.size()));
    }
}

} // namespace arete::widgets
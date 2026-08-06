#pragma once

#include <QWidget>
#include <QTableView>
#include <QListWidget>
#include <QComboBox>
#include <QLabel>
#include <memory>

class QAbstractItemModel;
class QSortFilterProxyModel;

namespace arete::logging {
class Logger;
}

namespace arete::widgets {

// A page that displays the application's log entries.
// Provides filters (Info / Warnings / Failures / Errors) and clearing.
class LogsPage : public QWidget
{
    Q_OBJECT

public:
    explicit LogsPage(QWidget* parent = nullptr);
    ~LogsPage() override;

    // Set the logger whose entries are displayed (defaults to the global logger)
    void setLogger(logging::Logger* logger);

    void setCategoryFilter(const QString& category, bool enabled);

    // Filter controls
    void showOnlyWarnings(bool on);
    void showOnlyErrors(bool on);
    void clearAll();

    [[nodiscard]] int entryCount() const;

signals:
    void logsCleared();
    void logsExported(const QString& filePath);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void refreshEntries();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::widgets
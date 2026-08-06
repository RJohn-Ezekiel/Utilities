#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QVector>
#include <functional>
#include <memory>

namespace arete::widgets {

struct Command
{
    QString id;
    QString title;
    QString detail;              // shown on the right (e.g. shortcut)
    QString group;               // optional grouping
    std::function<void()> action;
};

class CommandListModel;

// A modal command palette (Ctrl+P / Ctrl+K style).
// Supports filtering, arrow navigation, and Enter to execute.
class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override;

    void setCommands(const QVector<Command>& commands);
    void addCommand(const Command& command);
    void clearCommands();

    // Optional: free-form results from the app's universal search
    using SearchProvider = std::function<QVector<Command>(const QString& query)>;
    void setSearchProvider(SearchProvider provider);

    // Returns the executed command id, or empty if dismissed
    [[nodiscard]] QString execPalette();

    // Static convenience
    static QString run(QWidget* parent, const QVector<Command>& commands,
                       const QString& title = QString());

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void applyFilter(const QString& query);
    void executeSelected();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::widgets
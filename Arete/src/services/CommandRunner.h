#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace arete::widgets { struct Command; }

namespace arete {
namespace app { class AppContext; }
namespace services {

// Interprets palette-style command lines ("start work", "play 'song'",
// "volume 60", "open logos john 3", ...) and executes them against the
// shell and the vendored engines. Also supplies the static palette
// command catalog with aliases and remembers history.
class CommandRunner : public QObject
{
    Q_OBJECT

public:
    explicit CommandRunner(app::AppContext* context, QObject* parent = nullptr);

    // Static palette entries (with aliases) for the command palette.
    [[nodiscard]] QVector<arete::widgets::Command> paletteCommands();

    // Parses and executes an arbitrary command line.
    [[nodiscard]] bool run(const QString& commandLine, QString* error = nullptr);

    [[nodiscard]] QStringList history() const;
    void addToHistory(const QString& command);

private:
    void setupCommands();
    bool runQuoted(const QString& commandLine, QString* error);
    void completeTaskAndOfferNext(qint64 taskId, const QString& taskTitle);

    app::AppContext* m_context;
    QStringList m_history;
};

} // namespace services
} // namespace arete

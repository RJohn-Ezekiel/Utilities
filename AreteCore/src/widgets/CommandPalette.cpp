#include "arete/widgets/CommandPalette.h"
#include "arete/search/SearchUtils.h"
#include "arete/theme/Theme.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QPainter>

namespace arete::widgets {

class CommandPalette::Private
{
public:
    QLineEdit* input = nullptr;
    QListWidget* list = nullptr;
    QVector<Command> commands;
    CommandPalette::SearchProvider provider;
    QVector<Command> filtered;
    QString executedId;
};


CommandPalette::~CommandPalette() = default;

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    setModal(true);
    setWindowTitle(QStringLiteral("Command Palette"));
    setMinimumWidth(520);
    setMaximumWidth(560);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    d->input = new QLineEdit(this);
    d->input->setPlaceholderText(QStringLiteral("Type a command or search…"));
    d->input->setClearButtonEnabled(true);
    d->input->setStyleSheet(QStringLiteral(
        "QLineEdit { font-size: 14px; padding: 8px 10px; }"));
    layout->addWidget(d->input);

    d->list = new QListWidget(this);
    d->list->setStyleSheet(QStringLiteral(
        "QListWidget { border: 1px solid #353535; background-color: #1A1A1A; }"
        "QListWidget::item { padding: 8px 10px; }"
        "QListWidget::item:selected { background-color: #3A3A3A; color: #D0D0D0; }"));
    layout->addWidget(d->list, 1);

    auto* hint = new QLabel(QStringLiteral("↑↓ Navigate    Enter Execute    Esc Close"), this);
    hint->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);

    connect(d->input, &QLineEdit::textChanged, this, [this](const QString& text) {
        applyFilter(text);
    });

    connect(d->list, &QListWidget::itemActivated, this, [this]() {
        executeSelected();
    });

    // Keyboard handling for arrow navigation
    connect(d->list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < d->filtered.size()) {
            d->executedId = d->filtered[row].id;
        }
    });
}

void CommandPalette::setCommands(const QVector<Command>& commands)
{
    d->commands = commands;
    applyFilter(d->input->text());
}

void CommandPalette::addCommand(const Command& command)
{
    d->commands.append(command);
    applyFilter(d->input->text());
}

void CommandPalette::clearCommands()
{
    d->commands.clear();
    d->list->clear();
}

void CommandPalette::setSearchProvider(SearchProvider provider)
{
    d->provider = std::move(provider);
}

QString CommandPalette::execPalette()
{
    d->executedId.clear();
    d->input->clear();
    applyFilter(QString());
    d->input->setFocus();
    exec();
    return d->executedId;
}

QString CommandPalette::run(QWidget* parent, const QVector<Command>& commands, const QString& title)
{
    CommandPalette palette(parent);
    if (!title.isEmpty()) palette.setWindowTitle(title);
    palette.setCommands(commands);
    return palette.execPalette();
}

void CommandPalette::applyFilter(const QString& query)
{
    d->filtered.clear();
    const QString norm = arete::search::normalize(query);

    if (norm.isEmpty()) {
        d->filtered = d->commands;
    } else {
        for (const auto& cmd : d->commands) {
            if (arete::search::contains(cmd.title, query) ||
                arete::search::contains(cmd.detail, query) ||
                arete::search::contains(cmd.group, query)) {
                d->filtered.append(cmd);
            }
        }
        // Provider-based results
        if (d->provider) {
            const auto provided = d->provider(query);
            for (const auto& cmd : provided) {
                if (!arete::search::contains(cmd.title, query)) continue;
                d->filtered.append(cmd);
            }
        }
        std::sort(d->filtered.begin(), d->filtered.end(),
                  [&query](const Command& a, const Command& b) {
                      const int sa = arete::search::scoreItem(a.title, query, {}).value_or({}).score;
                      const int sb = arete::search::scoreItem(b.title, query, {}).value_or({}).score;
                      return sa > sb;
                  });
    }

    d->list->clear();
    for (const auto& cmd : d->filtered) {
        auto* item = new QListWidgetItem(cmd.title, d->list);
        if (!cmd.detail.isEmpty()) {
            item->setToolTip(cmd.detail);
        }
        item->setData(Qt::UserRole, cmd.id);
    }
    if (d->list->count() > 0) {
        d->list->setCurrentRow(0);
    }
}

void CommandPalette::executeSelected()
{
    const int row = d->list->currentRow();
    if (row < 0 || row >= d->filtered.size()) return;

    const QString id = d->filtered[row].id;
    d->executedId = id;
    const auto action = d->filtered[row].action;
    accept();
    if (action) {
        action();
    }
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Up) {
        const int row = d->list->currentRow();
        if (row > 0) {
            d->list->setCurrentRow(row - 1);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        const int row = d->list->currentRow();
        if (row < d->list->count() - 1) {
            d->list->setCurrentRow(row + 1);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        executeSelected();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        d->executedId.clear();
        reject();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void CommandPalette::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    d->input->setFocus();
}

} // namespace arete::widgets
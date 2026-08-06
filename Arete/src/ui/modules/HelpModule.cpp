#include "ui/modules/HelpModule.h"
#include "app/AppContext.h"
#include "ui/MainWindow.h"

#include <QDesktopServices>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace arete::ui {

namespace {
const QColor kKeyColor(0x7A, 0x9B, 0xC7);
const QColor kValueColor(0x9A, 0x9A, 0x9A);
const QColor kHeaderColor(0x6A, 0x6A, 0x6A);
} // namespace

HelpModule::HelpModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("Help"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    outer->addWidget(title);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 12, 0);
    layout->setSpacing(6);

    addSection(layout, QStringLiteral("SHORTCUTS"));
    addRow(layout, QStringLiteral("Ctrl+P"), QStringLiteral("Command palette"));
    addRow(layout, QStringLiteral("Ctrl+K"), QStringLiteral("Universal search"));
    addRow(layout, QStringLiteral("Ctrl+T"), QStringLiteral("Quick capture — task"));
    addRow(layout, QStringLiteral("Ctrl+J"), QStringLiteral("Jump to Journal"));
    addRow(layout, QStringLiteral("Ctrl+Tab / Ctrl+Shift+Tab"), QStringLiteral("Next / previous tab"));
    addRow(layout, QStringLiteral("Alt+1 … Alt+9"), QStringLiteral("Jump to tab"));
    addRow(layout, QStringLiteral("F1"), QStringLiteral("Shortcut summary"));

    addSection(layout, QStringLiteral("TABS"));
    addRow(layout, QStringLiteral("Home / Today"), QStringLiteral("Day overview, quick actions"));
    addRow(layout, QStringLiteral("Projects / Kanban"), QStringLiteral("Work planning and drag-and-drop boards"));
    addRow(layout, QStringLiteral("Calendar / Chronos"), QStringLiteral("Events, focus sessions, statistics"));
    addRow(layout, QStringLiteral("Codex / Logos / Phonio"), QStringLiteral("Notes, scripture, music"));
    addRow(layout, QStringLiteral("Journal / Habits / Wellbeing"), QStringLiteral("Reflection and tracking"));
    addRow(layout, QStringLiteral("Alerts / Logs / Ops"), QStringLiteral("Notifications, log output, activity feed"));

    addSection(layout, QStringLiteral("FEEDBACK"));
    addRow(layout, QStringLiteral("Bugs"), QStringLiteral("Report issues on the GitHub tracker"));
    addActionRow(layout, QStringLiteral("Report a bug"), [this] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/RJohn-Ezekiel/Utilities/issues")));
    });
    addActionRow(layout, QStringLiteral("Open Alerts"), [this, context] {
        if (auto* window = context->mainWindow()) window->activateModule(QStringLiteral("alerts"));
    });
    addActionRow(layout, QStringLiteral("Open Logs"), [this, context] {
        if (auto* window = context->mainWindow()) window->activateModule(QStringLiteral("logs"));
    });
    addSection(layout, QStringLiteral("DATA"));
    addRow(layout, QStringLiteral("Database"), context->databasePath());
    addRow(layout, QStringLiteral("Log file"), context->dataDirectory() + QStringLiteral("/arete.log"));
    addRow(layout, QStringLiteral("Settings"), QStringLiteral("~/.config/Arete/Arete.ini"));

    layout->addStretch(1);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
}

QLabel* HelpModule::addSection(QVBoxLayout* layout, const QString& title)
{
    auto* label = new QLabel(title, this);
    label->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; letter-spacing: 2px; margin-top: 12px;")
                             .arg(kHeaderColor.name()));
    layout->addWidget(label);
    return label;
}

void HelpModule::addRow(QVBoxLayout* layout, const QString& key, const QString& value)
{
    auto* label = new QLabel(QStringLiteral("<span style=\"color:%1;\">%2</span>"
                                            "  <span style=\"color:%3;\">— %4</span>")
                                 .arg(kKeyColor.name(), key.toHtmlEscaped(),
                                      kValueColor.name(), value.toHtmlEscaped()),
                             this);
    label->setStyleSheet(QStringLiteral("font-size: 13px;"));
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    layout->addWidget(label);
}

void HelpModule::addActionRow(QVBoxLayout* layout, const QString& labelText, std::function<void()> action)
{
    auto* button = new QPushButton(labelText, this);
    button->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #242424; color: #C4C4C4;"
                       " border: 1px solid #353535; border-radius: 8px; padding: 6px 16px; font-size: 13px; }"
                       "QPushButton:hover { border-color: #4A4A4A; }"));
    connect(button, &QPushButton::clicked, this, [action] { action(); });
    layout->addWidget(button);
}

} // namespace arete::ui

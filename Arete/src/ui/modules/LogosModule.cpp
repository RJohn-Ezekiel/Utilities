#include "ui/modules/LogosModule.h"
#include "app/AppContext.h"

#include "arete/settings/SettingsManager.h"
#include "arete/logging/Logger.h"
#include "services/ReferenceParser.h"
#include "services/VerseService.h"
#include "vendor/logos/ui/MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>

using arete::logging::Logger;

namespace arete::ui {

LogosModule::LogosModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // Daily verse banner shown above the Bible reader.
    m_verseLabel = new QLabel(this);
    m_verseLabel->setWordWrap(true);
    m_verseLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_verseLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #A0A0A0; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
        " border-radius: 12px; padding: 12px 14px; font-size: 14px; }"));
    m_verseLabel->hide();
    layout->addWidget(m_verseLabel);

    m_placeholder = new QLabel(QStringLiteral("Loading Logos\u2026"), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 14px;"));
    layout->addWidget(m_placeholder, 1);
}

void LogosModule::lazyLoad()
{
    // Show the daily scripture here too, so opening Logos always greets the
    // reader with the verse of the day.
    const auto verse = context()->verse()->dailyVerse();
    if (verse.valid) {
        m_verseLabel->setText(QStringLiteral("\u201C%1\u201D  \u2014  %2")
                                  .arg(verse.text, verse.reference));
        m_verseLabel->show();
    }

    QString biblesDir = context()->settings()->get(QStringLiteral("logos/biblesDir"), QString());
    if (biblesDir.isEmpty()) {
        biblesDir = QStringLiteral(ARETE_LOGOS_BIBLES_DIR);
    }
    if (!QFileInfo::exists(biblesDir)) {
        Logger::instance().warning(QStringLiteral("Bibles directory not found: %1").arg(biblesDir),
                                   QStringLiteral("logos"));
    }

    auto* window = new ::MainWindow(this);
    window->setWindowFlags(Qt::Widget);
    window->loadBibles(biblesDir.toStdString());
    window->resize(size());
    window->show();
    layout()->replaceWidget(m_placeholder, window);
    m_placeholder->deleteLater();
}

void LogosModule::handleCommand(const QString& command)
{
    if (command == QLatin1String("prayer")) {
        ensureLoaded();
        if (auto* window = findChild<::MainWindow*>()) window->openPrayerMode();
        return;
    }
    if (command == QLatin1String("hymn")) {
        ensureLoaded();
        if (auto* window = findChild<::MainWindow*>()) window->openHymnMode();
        return;
    }

    // Plain reference ("John 3:16-18") or prefixed ("logos:John 3:16").
    QString refText = command.trimmed();
    if (refText.startsWith(QLatin1String("logos:"))) {
        refText = refText.mid(6).trimmed();
    }
    if (refText.isEmpty()) return;

    const auto ref = ReferenceParser::parse(refText.toStdString());
    if (!ref.has_value()) return;

    ensureLoaded();
    if (auto* window = findChild<::MainWindow*>()) {
        window->navigateTo(*ref);
    }
}

} // namespace arete::ui

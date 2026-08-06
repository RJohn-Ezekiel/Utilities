#include "ui/modules/CodexModule.h"
#include "app/AppContext.h"

#include "arete/settings/SettingsManager.h"
#include "arete/logging/Logger.h"
#include "core/vault.h"
#include "vendor/codex/ui/mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QDir>

using arete::logging::Logger;

namespace arete::ui {

CodexModule::CodexModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_placeholder = new QLabel(QStringLiteral("Loading Codex\u2026"), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 14px;"));
    layout->addWidget(m_placeholder);
}

void CodexModule::lazyLoad()
{
    QString vaultRoot = context()->settings()->get(QStringLiteral("codex/vaultPath"), QString());
    if (vaultRoot.isEmpty()) {
        vaultRoot = context()->dataDirectory() + QStringLiteral("/codex-vault");
    }

    codex::VaultManager vaultManager;
    vaultManager.ensureVault(vaultRoot.toStdString());
    context()->settings()->set(QStringLiteral("codex/vaultPath"), vaultRoot);
    context()->settings()->sync();

    auto* window = new codex::MainWindow(&vaultManager, this);
    window->setWindowFlags(Qt::Widget);
    window->resize(size());
    window->show();
    layout()->replaceWidget(m_placeholder, window);
    m_placeholder->deleteLater();

    Logger::instance().info(QStringLiteral("Codex vault ready: %1").arg(vaultRoot),
                            QStringLiteral("codex"));
}

} // namespace arete::ui

#include "ui/modules/ModulesPage.h"
#include "app/AppContext.h"
#include "core/ModuleRegistry.h"
#include "ui/common/TaskToggle.h"
#include "ui/common/EmptyStateWidget.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <algorithm>

namespace arete::ui {

ModulesModule::ModulesModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(48, 32, 48, 32);
    m_layout->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Modules"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);
    m_layout->addLayout(header);

    m_summary = new QLabel(this);
    m_summary->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
    m_summary->setWordWrap(true);
    m_layout->addWidget(m_summary);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 8px 0; border-bottom: 1px solid #222222; }"));
    m_layout->addWidget(m_list, 1);

    // Rebuild the list when a module is toggled elsewhere.
    connect(context->modules(), &core::ModuleRegistry::modulesChanged,
            this, &ModulesModule::rebuild);

    rebuild();
}

void ModulesModule::onActivated()
{
    rebuild();
}

void ModulesModule::refreshView()
{
    rebuild();
}

void ModulesModule::rebuild()
{
    m_list->clear();

    const auto registry = context()->modules();
    const auto infos = registry->infos();

    int enabledCount = 0;
    for (const auto& info : infos) {
        if (info.optional && registry->isEnabled(info.id)) ++enabledCount;
    }
    m_summary->setText(QStringLiteral("%1 of %2 modules enabled \u00B7 core tabs (Home, Modules, Chronos, Settings) are always on")
                           .arg(enabledCount)
                           .arg(int(std::count_if(infos.begin(), infos.end(),
                                                  [](const core::ModuleInfo& i) { return i.optional; }))));

    if (infos.empty()) {
        m_list->addItem(QString());
        m_list->setItemWidget(
            m_list->item(0),
            new common::EmptyStateWidget(QStringLiteral(""),
                                         QStringLiteral("No modules installed."), this));
        return;
    }

    for (const auto& info : infos) {
        if (!info.optional) continue;

        auto* item = new QListWidgetItem;
        item->setData(Qt::UserRole, info.id);
        item->setSizeHint(QSize(0, 54));
        m_list->addItem(item);

        auto* widget = new QWidget(this);
        auto* row = new QHBoxLayout(widget);
        row->setContentsMargins(4, 0, 4, 0);
        row->setSpacing(14);

        auto* toggle = new common::TaskToggle(widget);
        const bool enabled = registry->isEnabled(info.id);
        toggle->setChecked(enabled, false);
        toggle->setToolTip(enabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled"));
        connect(toggle, &common::TaskToggle::toggled, this,
                [this, registry, id = info.id](bool checked) {
                    registry->setEnabled(id, checked);
                });
        row->addWidget(toggle);

        auto* textColumn = new QVBoxLayout;
        textColumn->setSpacing(2);
        auto* nameRow = new QHBoxLayout;
        nameRow->setSpacing(8);
        auto* nameLabel = new QLabel(info.title, widget);
        nameLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #D6D6D6;"));
        nameRow->addWidget(nameLabel);

        auto* versionLabel = new QLabel(info.version.isEmpty() ? QString() : info.version, widget);
        versionLabel->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 11px;"));
        nameRow->addWidget(versionLabel);
        nameRow->addStretch(1);
        textColumn->addLayout(nameRow);

        auto* descriptionLabel = new QLabel(info.description, widget);
        descriptionLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
        descriptionLabel->setWordWrap(true);
        textColumn->addWidget(descriptionLabel);
        row->addLayout(textColumn, 1);

        m_list->setItemWidget(item, widget);
    }
}

} // namespace arete::ui
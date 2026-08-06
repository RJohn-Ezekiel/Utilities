#include "ui/modules/ChronosModule.h"
#include "app/AppContext.h"
#include "services/TimerService.h"

#include "vendor/chronos/ui/MainWindow.h"

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace arete::ui {

namespace {
struct FlowPreset {
    const char* name;
    const char* label;
    int minutes;
};
const FlowPreset kFlows[] = {
    { "DEEP", "Deep work", 50 },
    { "POMODORO", "Pomodoro", 25 },
    { "BLITZ", "Blitz", 90 },
};
}

ChronosModule::ChronosModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* flows = new QHBoxLayout;
    flows->setContentsMargins(32, 14, 32, 10);
    flows->setSpacing(8);

    auto* caption = new QLabel(QStringLiteral("WORKDAY FLOWS"), this);
    caption->setStyleSheet(QStringLiteral("color: #6A6A6A; font-size: 11px; letter-spacing: 2px;"));
    flows->addWidget(caption);
    flows->addSpacing(6);

    const auto makeButton = [this, layout](const QString& text) {
        auto* b = new QPushButton(text, this);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton { color: #C4C4C4; background-color: #242424; border: 1px solid #353535;"
            " border-radius: 8px; padding: 5px 12px; font-size: 12px; letter-spacing: 1px; }"
            "QPushButton:hover { background-color: #2A2A2A; }"));
        return b;
    };

    for (const FlowPreset& flow : kFlows) {
        auto* button = makeButton(QString::fromLatin1(flow.name));
        button->setToolTip(QStringLiteral("%1 — %2 minutes of focus, then the usual break")
                               .arg(QString::fromLatin1(flow.label)).arg(flow.minutes));
        connect(button, &QPushButton::clicked, this,
                [this, context, flow] { context->timer()->startCustomTimer(flow.minutes, QString::fromLatin1(flow.label)); });
        flows->addWidget(button);
    }

    auto* custom = makeButton(QStringLiteral("CUSTOM\u2026"));
    custom->setToolTip(QStringLiteral("Set your own focus length and label"));
    connect(custom, &QPushButton::clicked, this, [this, context] {
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("Custom timer"));
        auto* form = new QFormLayout(&dialog);
        auto* minutes = new QSpinBox(&dialog);
        minutes->setRange(1, 480);
        minutes->setValue(30);
        auto* label = new QLineEdit(&dialog);
        label->setPlaceholderText(QStringLiteral("e.g. Deep reading"));
        label->setStyleSheet(QStringLiteral(
            "QLineEdit, QSpinBox { background: #1C1C1C; border: 1px solid #2A2A2A;"
            " color: #D6D6D6; border-radius: 6px; padding: 6px 8px; }"));
        form->addRow(QStringLiteral("Minutes"), minutes);
        form->addRow(QStringLiteral("Label"), label);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Start"));
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        form->addRow(buttons);
        if (dialog.exec() != QDialog::Accepted) return;
        const QString labelText = label->text().trimmed().isEmpty()
            ? QStringLiteral("Custom") : label->text().trimmed();
        context->timer()->startCustomTimer(minutes->value(), labelText);
    });
    flows->addWidget(custom);
    flows->addStretch(1);
    layout->addLayout(flows);

    m_placeholder = new QLabel(QStringLiteral("Loading Chronos\u2026"), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 14px;"));
    layout->addWidget(m_placeholder, 1);
}

void ChronosModule::lazyLoad()
{
    auto* window = new chronos::MainWindow(context()->chronosTimer(),
                                           context()->chronosTasks(),
                                           context()->chronosStats(),
                                           context()->chronosReminders(),
                                           context()->chronosNotifications(),
                                           this);
    window->setWindowFlags(Qt::Widget);
    window->setParent(this);
    window->resize(size());
    window->show();
    layout()->replaceWidget(m_placeholder, window);
    m_placeholder->deleteLater();
}

} // namespace arete::ui

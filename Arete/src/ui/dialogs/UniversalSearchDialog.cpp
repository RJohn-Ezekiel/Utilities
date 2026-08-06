#include "ui/dialogs/UniversalSearchDialog.h"

#include <QLineEdit>
#include <QShowEvent>

namespace arete::ui {

UniversalSearchDialog::UniversalSearchDialog(QWidget* parent)
    : arete::widgets::CommandPalette(parent)
{
    setWindowTitle(QStringLiteral("Universal Search"));
}

void UniversalSearchDialog::setInitialQuery(const QString& query)
{
    m_initialQuery = query;
}

void UniversalSearchDialog::showEvent(QShowEvent* event)
{
    arete::widgets::CommandPalette::showEvent(event);
    if (!m_initialQuery.isEmpty()) {
        if (auto* input = findChild<QLineEdit*>()) {
            input->setText(m_initialQuery);
            input->selectAll();
        }
        m_initialQuery.clear();
    }
}

} // namespace arete::ui

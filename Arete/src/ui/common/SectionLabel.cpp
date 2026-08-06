#include "ui/common/SectionLabel.h"

namespace arete::ui::common {

SectionLabel::SectionLabel(const QString& text, QWidget* parent)
    : QLabel(text, parent)
{
    setProperty("heading", true);
    setStyleSheet(QStringLiteral("font-size: 11px; letter-spacing: 2px; color: #7A7A7A;"));
}

} // namespace arete::ui::common

#pragma once

#include <QLabel>

namespace arete::ui::common {

// Muted section heading with a small separator rule.
class SectionLabel : public QLabel
{
    Q_OBJECT

public:
    explicit SectionLabel(const QString& text, QWidget* parent = nullptr);
};

} // namespace arete::ui::common

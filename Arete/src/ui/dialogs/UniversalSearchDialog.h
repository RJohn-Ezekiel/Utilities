#pragma once

#include <arete/widgets/CommandPalette.h>
#include <QString>

namespace arete::ui {

// Universal search (Ctrl+K): the shared palette with the app-wide search
// provider attached and grouped results.
class UniversalSearchDialog : public arete::widgets::CommandPalette
{
    Q_OBJECT

public:
    explicit UniversalSearchDialog(QWidget* parent = nullptr);

    void setInitialQuery(const QString& query);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QString m_initialQuery;
};

} // namespace arete::ui

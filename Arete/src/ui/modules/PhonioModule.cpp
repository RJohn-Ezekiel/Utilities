#include "ui/modules/PhonioModule.h"
#include "app/AppContext.h"

#include "vendor/phonio/app/App.h"
#include "vendor/phonio/ui/MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>

namespace arete::ui {

PhonioModule::PhonioModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_placeholder = new QLabel(QStringLiteral("Loading Phonio\u2026"), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 14px;"));
    layout->addWidget(m_placeholder);
}

void PhonioModule::lazyLoad()
{
    auto* window = context()->phonioApp()->mainWindow();
    window->setParent(this);
    window->setWindowFlags(Qt::Widget);
    window->resize(size());
    window->show();
    layout()->replaceWidget(m_placeholder, window);
    m_placeholder->deleteLater();
}

} // namespace arete::ui

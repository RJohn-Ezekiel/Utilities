#include "core/Module.h"

namespace arete::core {

Module::Module(app::AppContext* context, QWidget* parent)
    : QWidget(parent), m_context(context)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
}

} // namespace arete::core

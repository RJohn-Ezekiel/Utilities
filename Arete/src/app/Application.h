#pragma once

#include <QObject>
#include <memory>

namespace arete {
namespace app { class AppContext; }
namespace ui { class MainWindow; }

// Composition root of the Arete shell: owns the context and main window,
// applies the global theme, and restores the session.
class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    void shutdown();

private:
    void applyTheme();
    void restoreSession();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete

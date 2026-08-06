#pragma once

#include <QDialog>
#include <QString>
#include <memory>

class QLabel;

namespace arete::widgets {

// Consistent About dialog for all Arete applications.
struct AboutInfo
{
    QString applicationName;
    QString version;
    QString description;
    QString organization = QStringLiteral("Arete");
    QString copyright;
    QString license;
    QString website;
};

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(const AboutInfo& info, QWidget* parent = nullptr);
    ~AboutDialog() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::widgets
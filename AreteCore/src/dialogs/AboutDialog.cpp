#include "arete/dialogs/AboutDialog.h"
#include "arete/theme/Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

namespace arete::widgets {

class AboutDialog::Private
{
public:
    AboutInfo info;
};

AboutDialog::~AboutDialog() = default;

AboutDialog::AboutDialog(const AboutInfo& info, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->info = info;
    setWindowTitle(QStringLiteral("About %1").arg(info.applicationName));
    setModal(true);
    setFixedSize(380, 340);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 16);
    layout->setSpacing(8);

    // Icon / name
    auto* nameLabel = new QLabel(info.applicationName, this);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet(QStringLiteral(
        "font-size: 24px; font-weight: 700; color: #D0D0D0;"));
    layout->addWidget(nameLabel);

    auto* versionLabel = new QLabel(QStringLiteral("Version %1").arg(info.version), this);
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet(QStringLiteral("color: #A0A0A0; font-size: 12px;"));
    layout->addWidget(versionLabel);

    auto* orgLabel = new QLabel(info.organization, this);
    orgLabel->setAlignment(Qt::AlignCenter);
    orgLabel->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
    layout->addWidget(orgLabel);

    // Separator
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QStringLiteral("color: #353535;"));
    layout->addWidget(separator);
    layout->addSpacing(8);

    // Description
    auto* descLabel = new QLabel(info.description, this);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QStringLiteral("color: #A0A0A0;"));
    layout->addWidget(descLabel);

    layout->addStretch();

    // Details
    if (!info.copyright.isEmpty()) {
        auto* copyrightLabel = new QLabel(info.copyright, this);
        copyrightLabel->setAlignment(Qt::AlignCenter);
        copyrightLabel->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
        layout->addWidget(copyrightLabel);
    }

    if (!info.license.isEmpty()) {
        auto* licenseLabel = new QLabel(QStringLiteral("License: %1").arg(info.license), this);
        licenseLabel->setAlignment(Qt::AlignCenter);
        licenseLabel->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
        layout->addWidget(licenseLabel);
    }

    if (!info.website.isEmpty()) {
        auto* websiteLabel = new QLabel(info.website, this);
        websiteLabel->setAlignment(Qt::AlignCenter);
        websiteLabel->setStyleSheet(QStringLiteral(
            "color: #8A8A8A; font-size: 11px; text-decoration: underline;"));
        websiteLabel->setCursor(Qt::PointingHandCursor);
        layout->addWidget(websiteLabel);
    }

    layout->addSpacing(8);

    auto* okButton = new QPushButton(QStringLiteral("OK"), this);
    okButton->setProperty("primary", true);
    okButton->setDefault(true);
    okButton->setFixedWidth(120);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(okButton, 0, Qt::AlignCenter);

    setStyleSheet(theme::appStyleSheet());
}

} // namespace arete::widgets
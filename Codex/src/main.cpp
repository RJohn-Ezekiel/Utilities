#include <QApplication>
#include <QCommandLineParser>
#include <QFont>

#include <filesystem>

#include "core/vault.h"
#include "core/config.h"
#include "core/util.h"
#include "cli/cli.h"
#include "ui/mainwindow.h"

__attribute__((constructor))
static void earlySuppress()
{
    qputenv("QT_LOGGING_RULES", "kf.*.warning=false");
    qputenv("QT_QPA_PLATFORMTHEME", "");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Codex"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("Codex"));

    QFont monoFont(QStringLiteral("JetBrains Mono"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(13);
    app.setFont(monoFont);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Offline Markdown Knowledge Base"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("vault"), QStringLiteral("Path to vault directory"));
    parser.addOption({{"c", "create"}, QStringLiteral("Create a new vault")});
    parser.addOption({{"l", "list"}, QStringLiteral("List all notes")});
    parser.addOption({{"s", "show"}, QStringLiteral("Show note content"), QStringLiteral("title")});
    parser.addOption({{"n", "new"}, QStringLiteral("Create a new note"), QStringLiteral("title")});
    parser.addOption({{"q", "search"}, QStringLiteral("Search notes"), QStringLiteral("query")});
    parser.addOption({{"e", "export"}, QStringLiteral("Export a note or --all"), QStringLiteral("target")});
    parser.addOption(QCommandLineOption(QStringLiteral("all"), QStringLiteral("Export all notes")));
    parser.addOption({{"o", "output"}, QStringLiteral("Export output directory"), QStringLiteral("dir")});
    parser.process(app);

    // Determine vault path
    std::filesystem::path vaultPath;
    const auto args = parser.positionalArguments();
    if (!args.isEmpty()) {
        vaultPath = args.first().toStdString();
    } else if (auto *env = getenv("CODEX_VAULT")) {
        vaultPath = env;
    } else {
        // Try current working directory
        auto cwd = std::filesystem::current_path();
        if (std::filesystem::exists(cwd / codex::vault_layout::DIR_NOTES)) {
            vaultPath = cwd;
        } else {
            // Fall back to config
            codex::Config cfg;
            if (cfg.load(codex::util::defaultConfigPath()) && !cfg.vaultPath().empty())
                vaultPath = cfg.vaultPath();
            if (vaultPath.empty())
                vaultPath = cwd; // use CWD, will auto-create
        }
    }

    // Create vault if requested (explicit --create)
    if (parser.isSet(QStringLiteral("create"))) {
        if (vaultPath.empty()) {
            qCritical().noquote() << "Provide a path for the new vault.";
            return 1;
        }
        codex::VaultManager creator;
        if (!creator.createVault(vaultPath)) {
            qCritical().noquote() << "Failed to create vault at:" << QString::fromStdString(vaultPath.string());
            return 1;
        }
        qInfo().noquote() << "Vault created at:" << QString::fromStdString(vaultPath.string());
        return 0;
    }

    // CLI mode
    if (parser.isSet(QStringLiteral("list")) ||
        parser.isSet(QStringLiteral("show")) ||
        parser.isSet(QStringLiteral("new")) ||
        parser.isSet(QStringLiteral("search")) ||
        parser.isSet(QStringLiteral("export")) ||
        parser.isSet(QStringLiteral("all")))
    {
        if (vaultPath.empty()) {
            qCritical().noquote() << "No vault specified. Pass a path.";
            return 1;
        }
        codex::VaultManager cliVault;
        if (!cliVault.ensureVault(vaultPath)) {
            qCritical().noquote() << "Cannot access vault at:" << QString::fromStdString(vaultPath.string());
            return 1;
        }
        codex::CLI cli(vaultPath);
        return cli.run(parser);
    }

    // GUI mode — auto-create vault if needed
    if (vaultPath.empty())
        vaultPath = std::filesystem::current_path();

    codex::VaultManager vault;
    if (!vault.ensureVault(vaultPath)) {
        qCritical().noquote() << "Failed to open or create vault at:" << QString::fromStdString(vaultPath.string());
        return 1;
    }

    // Save to config for future launches
    codex::Config cfg;
    cfg.setVaultPath(vaultPath);
    std::filesystem::create_directories(codex::util::configDir());
    cfg.save(codex::util::defaultConfigPath());

    codex::MainWindow window(&vault);
    window.show();
    return app.exec();
}

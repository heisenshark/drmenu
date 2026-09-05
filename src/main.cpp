#include "cli_parser.h"
#include "config_loader.h"
#include "daemon.h"
#include "client.h"
#include "standalone_app.h"
#include "hypr_shader.h"
#include "config_editor_app.h"

#include <QTextStream>
#include <QProcess>

int main(int argc, char *argv[]) {
    ParsedArgs args = CliParser::parse(argc, argv);

    if (args.editorMode)
        return ConfigEditorApp::run(argc, argv, args.configPath);

    if (args.pingMode) {
        QTextStream out(stdout);
        out << "========================================\n";
        out << "  drmenu <-> hypr-liquid-glass IPC Ping  \n";
        out << "========================================\n";
        QString response = HyprlandGlassShader::ping();
        out << "Response from Hyprland Socket:\n";
        out << "  " << response << "\n";
        out << "========================================\n";
        out.flush();
        return response.startsWith("ERROR") ? 1 : 0;
    }

    if (args.daemonMode)
        return DaemonServer::run(argc, argv);

    if (!args.menuName.isEmpty()) {
        QVariantMap allMenus = ConfigLoader::loadAllMenus(args.configPath);
        if (allMenus.isEmpty()) return 1;

        if (!allMenus.contains(args.menuName)) {
            QTextStream(stderr) << "drmenu: menu '" << args.menuName << "' not found.\n"
                                << "Available: " << allMenus.keys().join(", ") << "\n";
            return 1;
        }

        ConfigLoader::MenuOptions opts = ConfigLoader::loadMenuOptions(args.menuName, args.configPath);
        QVariantMap initialStyle        = ConfigLoader::loadStyle(args.menuName, args.configPath);

        if (!args.standaloneMode && args.configPath.isEmpty()) {
            if (ClientRunner::tryRunMenus(allMenus, args.menuName, opts.spawnAtMouse, opts.escapeClosesAll, initialStyle))
                return 0;
        }

        return StandaloneApp::run(argc, argv, allMenus, args.menuName, opts.spawnAtMouse, opts.escapeClosesAll, initialStyle);
    }

    // Inline / media / stdin mode
    if (args.items.isEmpty()) {
        QTextStream(stderr) << "drmenu: no items provided.\n"
                            << "Usage:\n"
                            << "  drmenu --daemon                      (start background daemon)\n"
                            << "  drmenu --editor                      (launch GUI configuration editor)\n"
                            << "  drmenu --menu <name>                 (launch a named menu from config)\n"
                            << "  drmenu --media                       (launch dynamic MPRIS media sources menu)\n"
                            << "  drmenu --menu <name> --config <path> (use a custom config file)\n"
                            << "  echo -e 'Item1\\nItem2' | drmenu     (stdin, dmenu style)\n";
        return 1;
    }

    if (!args.standaloneMode && ClientRunner::tryRun(args.items, QVariantMap()))
        return 0;

    return StandaloneApp::runItems(argc, argv, args.items, QVariantMap());
}

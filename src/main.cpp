#include "cli_parser.h"
#include "config_loader.h"
#include "daemon.h"
#include "client.h"
#include "standalone_app.h"
#include <QTextStream>

int main(int argc, char *argv[]) {
    ParsedArgs args = CliParser::parse(argc, argv);

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

        if (ClientRunner::tryRunMenus(allMenus, args.menuName, opts.spawnAtMouse, opts.escapeClosesAll, initialStyle))
            return 0;

        return StandaloneApp::run(argc, argv, allMenus, args.menuName, opts.spawnAtMouse, opts.escapeClosesAll, initialStyle);
    }

    // Inline / stdin mode
    if (args.items.isEmpty()) {
        QTextStream(stderr) << "drmenu: no items provided.\n"
                            << "Usage:\n"
                            << "  drmenu --daemon                      (start background daemon)\n"
                            << "  drmenu --menu <name>                 (launch a named menu from config)\n"
                            << "  drmenu --menu <name> --config <path> (use a custom config file)\n"
                            << "  echo -e 'Item1\\nItem2' | drmenu     (stdin, dmenu style)\n";
        return 1;
    }

    QVariantMap defaultStyle = ConfigLoader::loadStyle({}, args.configPath);

    if (ClientRunner::tryRun(args.items, defaultStyle))
        return 0;

    return StandaloneApp::runItems(argc, argv, args.items, defaultStyle);
}

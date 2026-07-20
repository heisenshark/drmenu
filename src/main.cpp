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

    // Load items from config if --menu was specified
    if (!args.menuName.isEmpty())
        args.items = ConfigLoader::loadMenu(args.menuName, args.configPath);

    if (args.items.isEmpty()) {
        QTextStream(stderr) << "drmenu: no items provided.\n"
                            << "Usage:\n"
                            << "  drmenu --daemon                      (start background daemon)\n"
                            << "  drmenu --menu <name>                 (launch a named menu from config)\n"
                            << "  drmenu --menu <name> --config <path> (use a custom config file)\n"
                            << "  drmenu 'Label:icon\\tcommand' ...    (inline items)\n"
                            << "  echo -e 'Item1\\nItem2' | drmenu     (stdin, dmenu style)\n";
        return 1;
    }

    if (ClientRunner::tryRun(args.items))
        return 0;

    return StandaloneApp::run(argc, argv, args.items);
}

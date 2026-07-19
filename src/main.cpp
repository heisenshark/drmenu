#include "cli_parser.h"
#include "daemon.h"
#include "client.h"
#include "standalone_app.h"
#include <QTextStream>

int main(int argc, char *argv[]) {
    if (CliParser::isDaemonMode(argc, argv)) {
        return DaemonServer::run(argc, argv);
    }

    QList<MenuItem> items = CliParser::parseItems(argc, argv);
    if (items.isEmpty()) {
        QTextStream(stderr) << "drmenu: no items provided.\n"
                            << "Usage:\n"
                            << "  drmenu --daemon              (start background daemon)\n"
                            << "  echo -e 'Item1\\nItem2' | drmenu\n"
                            << "  drmenu 'Item1' 'Item2'\n";
        return 1;
    }

    if (ClientRunner::tryRun(items)) {
        return 0;
    }

    return StandaloneApp::run(argc, argv, items);
}

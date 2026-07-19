#pragma once

#include <QString>

class DaemonServer {
public:
    static const QString SOCKET_NAME;
    static int run(int argc, char *argv[]);
};

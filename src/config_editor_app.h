#pragma once

#include <QString>

class ConfigEditorApp {
public:
    static int run(int argc, char *argv[], const QString &initialConfigPath = {});
};

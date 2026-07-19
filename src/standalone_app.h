#pragma once

#include "item.h"
#include <QList>

class StandaloneApp {
public:
    static int run(int argc, char *argv[], const QList<MenuItem> &items);
};

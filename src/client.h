#pragma once

#include "item.h"
#include <QList>

class ClientRunner {
public:
    static bool tryRun(const QList<MenuItem> &items);
};

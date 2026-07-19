#pragma once

#include <QString>

struct TargetScreenInfo {
    QString monitorName;
    int localX = -1;
    int localY = -1;
};

class ScreenDetector {
public:
    static TargetScreenInfo getTargetScreenInfo();
};

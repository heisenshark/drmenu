#pragma once

#include <QString>

struct TargetScreenInfo {
    QString monitorName;
    int localX = -1;
    int localY = -1;
    int monitorX = 0;
    int monitorY = 0;
    int monitorWidth = 0;
    int monitorHeight = 0;
};

class ScreenDetector {
public:
    static TargetScreenInfo getTargetScreenInfo();
};

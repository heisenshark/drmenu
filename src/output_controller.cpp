#include "output_controller.h"
#include "screen_detector.h"

OutputController::OutputController(QObject *parent) : QObject(parent) {}

void OutputController::setItems(const QVariantList &items) {
    m_items = items;
    emit itemsChanged();
}

void OutputController::setMenuData(const QVariantMap &menus, const QString &initialMenu,
                                    bool spawnAtMouse, bool escapeClosesAll) {
    m_menus           = menus;
    m_initialMenu     = initialMenu;
    m_spawnAtMouse    = spawnAtMouse;
    m_escapeClosesAll = escapeClosesAll;
    emit menusChanged();
}

void OutputController::select(const QString &label) {
    if (onSelectCallback) onSelectCallback(label);
}

void OutputController::cancel() {
    if (onCancelCallback) onCancelCallback();
}

QVariantMap OutputController::getMousePosition() {
    TargetScreenInfo info = ScreenDetector::getTargetScreenInfo();
    QVariantMap map;
    map["x"] = info.localX;
    map["y"] = info.localY;
    return map;
}

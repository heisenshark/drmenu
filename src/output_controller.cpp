#include "output_controller.h"
#include "screen_detector.h"
#include "theme_manager.h"

OutputController::OutputController(QObject *parent)
    : QObject(parent), m_style(ThemeManager::resolveStyle("blender")) {}

void OutputController::setItems(const QVariantList &items, const QVariantMap &style) {
    m_items = items;
    if (!style.isEmpty()) {
        m_style = style;
        emit styleChanged();
    }
    emit itemsChanged();
}

void OutputController::setMenuData(const QVariantMap &menus, const QString &initialMenu,
                                    bool spawnAtMouse, bool escapeClosesAll,
                                    const QVariantMap &style) {
    m_menus           = menus;
    m_initialMenu     = initialMenu;
    m_spawnAtMouse    = spawnAtMouse;
    m_escapeClosesAll = escapeClosesAll;
    if (!style.isEmpty()) {
        m_style = style;
        emit styleChanged();
    }
    emit menusChanged();
}

void OutputController::setStyle(const QVariantMap &style) {
    m_style = style;
    emit styleChanged();
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

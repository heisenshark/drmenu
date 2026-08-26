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

#include "hypr_shader.h"

void OutputController::activateGlassShader(int screenW, int screenH, float centerX, float centerY,
                                           const QVariantList &pillsList,
                                           float chromaticAberration,
                                           float blurRadius,
                                           float vibrancy,
                                           float refraction) {
    QList<PillGeometry> pills;
    for (const QVariant &v : pillsList) {
        QVariantMap m = v.toMap();
        PillGeometry p;
        p.x = m["x"].toFloat();
        p.y = m["y"].toFloat();
        p.halfWidth = m["halfWidth"].toFloat();
        p.halfHeight = m["halfHeight"].toFloat();
        p.radius = m["radius"].toFloat();
        pills.append(p);
    }
    HyprlandGlassShader::activate(screenW, screenH, centerX, centerY, pills,
                                  chromaticAberration, blurRadius, vibrancy, refraction);
}

void OutputController::deactivateGlassShader() {
    HyprlandGlassShader::deactivate();
}

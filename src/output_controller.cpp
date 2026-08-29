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
#include <QColor>

void OutputController::activateGlassShader(int screenW, int screenH, float centerX, float centerY,
                                           const QVariantList &pillsList,
                                           float chromaticAberration,
                                           float blurRadius,
                                           float vibrancy,
                                           float refraction,
                                           float specular) {
    QList<PillGeometry> pills;
    for (const QVariant &v : pillsList) {
        QVariantMap m = v.toMap();
        PillGeometry p;
        p.x = m["x"].toFloat();
        p.y = m["y"].toFloat();
        p.halfWidth = m["halfWidth"].toFloat();
        p.halfHeight = m["halfHeight"].toFloat();
        p.radius = m["radius"].toFloat();

        if (m.contains("pillColor")) {
            QColor c(m["pillColor"].toString());
            if (c.isValid()) {
                p.milkyR = c.redF();
                p.milkyG = c.greenF();
                p.milkyB = c.blueF();
                p.milkyA = c.alphaF();
            }
        }
        if (m.contains("borderColor")) {
            QColor bc(m["borderColor"].toString());
            if (bc.isValid()) {
                p.borderR = bc.redF();
                p.borderG = bc.greenF();
                p.borderB = bc.blueF();
                p.borderA = bc.alphaF();
            }
        }
        if (m.contains("borderWidth")) {
            p.borderWidth = m["borderWidth"].toFloat();
        }
        if (m.contains("blur")) {
            p.blur = m["blur"].toFloat();
        }
        if (m.contains("refraction")) {
            p.refraction = m["refraction"].toFloat();
        }
        if (m.contains("chromatic")) {
            p.chromatic = m["chromatic"].toFloat();
        }
        if (m.contains("specular")) {
            p.specular = m["specular"].toFloat();
        }

        pills.append(p);
    }
    HyprlandGlassShader::activate(screenW, screenH, centerX, centerY, pills,
                                  chromaticAberration, blurRadius, vibrancy, refraction, specular);
}

void OutputController::deactivateGlassShader() {
    HyprlandGlassShader::deactivate();
}

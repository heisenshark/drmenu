#pragma once

#include <QString>
#include <QPoint>
#include <QSize>
#include <QVariantList>
#include <QVariantMap>

struct PillGeometry {
    float x = 0;
    float y = 0;
    float halfWidth = 0;
    float halfHeight = 0;
    float radius = 0;
    float milkyR = 1.0f, milkyG = 1.0f, milkyB = 1.0f, milkyA = 0.12f;
    float borderR = 1.0f, borderG = 1.0f, borderB = 1.0f, borderA = 0.40f;
    float borderWidth = 1.5f;
};

class HyprlandGlassShader {
public:
    static bool isSupported();

    static void activate(int screenWidth, int screenHeight,
                         float centerX, float centerY,
                         const QList<PillGeometry> &pills,
                         float chromaticAberration = 14.0f,
                         float blurRadius = 8.0f,
                         float vibrancy = 1.45f,
                         float refraction = 0.008f,
                         float specular = 0.70f);

    static void deactivate();
    static QString ping();

private:
    static QString generateShader(int screenWidth, int screenHeight,
                                  float centerX, float centerY,
                                  const QList<PillGeometry> &pills,
                                  float chromaticAberration,
                                  float blurRadius,
                                  float vibrancy,
                                  float refraction);
};

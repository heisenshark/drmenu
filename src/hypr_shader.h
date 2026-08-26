#pragma once

#include <QString>
#include <QPoint>
#include <QSize>
#include <QVariantList>
#include <QVariantMap>

struct PillGeometry {
    float x;
    float y;
    float halfWidth;
    float halfHeight;
    float radius;
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
                         float refraction = 0.008f);

    static void deactivate();

private:
    static QString generateShader(int screenWidth, int screenHeight,
                                  float centerX, float centerY,
                                  const QList<PillGeometry> &pills,
                                  float chromaticAberration,
                                  float blurRadius,
                                  float vibrancy,
                                  float refraction);
};

#include "hypr_shader.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <cmath>

bool HyprlandGlassShader::isSupported() {
    return !qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty();
}

QString HyprlandGlassShader::generateShader(int screenWidth, int screenHeight,
                                            float centerX, float centerY,
                                            const QList<PillGeometry> &pills,
                                            float chromaticAberration,
                                            float blurRadius,
                                            float vibrancy,
                                            float refraction) {
    Q_UNUSED(centerX);
    Q_UNUSED(centerY);

    QString glsl;
    QTextStream out(&glsl);

    out << "precision mediump float;\n";
    out << "varying vec2 v_texcoord;\n";
    out << "uniform sampler2D tex;\n\n";

    // Signed Distance Field for rounded rectangle (squircle)
    out << "float sdRoundedBox(vec2 p, vec2 b, float r) {\n";
    out << "    vec2 q = abs(p) - b + r;\n";
    out << "    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n";
    out << "}\n\n";

    out << "void main() {\n";
    out << "    vec2 uv = v_texcoord;\n";
    out << "    vec2 res = vec2(" << screenWidth << ".0, " << screenHeight << ".0);\n";
    out << "    vec2 p = uv * res;\n";
    out << "    float minDist = 9999.0;\n";
    out << "    vec2 hitNormal = vec2(0.0);\n";
    out << "    vec2 hitCenter = vec2(0.0);\n\n";

    // Evaluate distance to each pill
    for (int i = 0; i < pills.size(); ++i) {
        const PillGeometry &pill = pills[i];
        out << "    {\n";
        out << "        vec2 pillCenter = vec2(" << pill.x << ", " << pill.y << ");\n";
        out << "        vec2 d = p - pillCenter;\n";
        out << "        float dist = sdRoundedBox(d, vec2(" << pill.halfWidth << ", " << pill.halfHeight << "), " << pill.radius << ");\n";
        out << "        if (dist < minDist) {\n";
        out << "            minDist = dist;\n";
        out << "            hitNormal = (length(d) > 0.001) ? normalize(d) : vec2(0.0, -1.0);\n";
        out << "            hitCenter = pillCenter;\n";
        out << "        }\n";
        out << "    }\n";
    }

    out << "\n    if (minDist <= 0.0) {\n";
    out << "        // ── Inside Liquid Glass Pill ──\n";
    out << "        float edgeFactor = clamp(-minDist / 10.0, 0.0, 1.0);\n";
    out << "        float refr = (1.0 - edgeFactor) * " << QString::number(refraction, 'f', 4) << ";\n";
    out << "        vec2 uvRefract = uv - hitNormal * refr;\n\n";

    // Optical Multi-Wavelength Chromatic Dispersion
    float shiftNorm = chromaticAberration / (float)screenWidth;
    out << "        float shift = " << QString::number(shiftNorm, 'f', 6) << " * (1.0 - edgeFactor * 0.4);\n";
    out << "        vec2 offsetR = vec2(shift, -shift * 0.5);\n";
    out << "        vec2 offsetB = vec2(-shift, shift * 0.5);\n\n";

    if (blurRadius > 0.5f) {
        // Multi-tap GPU Poisson bokeh blur
        float blurStepX = (blurRadius * 0.75f) / (float)screenWidth;
        float blurStepY = (blurRadius * 0.75f) / (float)screenHeight;

        out << "        vec2 bStep = vec2(" << QString::number(blurStepX, 'f', 6) << ", " << QString::number(blurStepY, 'f', 6) << ");\n";
        out << "        vec3 col = vec3(0.0);\n";
        out << "        col.r += texture2D(tex, uvRefract + offsetR).r * 0.30;\n";
        out << "        col.r += texture2D(tex, uvRefract + offsetR + vec2( bStep.x,  bStep.y)).r * 0.20;\n";
        out << "        col.r += texture2D(tex, uvRefract + offsetR + vec2(-bStep.x,  bStep.y)).r * 0.20;\n";
        out << "        col.r += texture2D(tex, uvRefract + offsetR + vec2( bStep.x, -bStep.y)).r * 0.15;\n";
        out << "        col.r += texture2D(tex, uvRefract + offsetR + vec2(-bStep.x, -bStep.y)).r * 0.15;\n\n";

        out << "        col.g += texture2D(tex, uvRefract).g * 0.30;\n";
        out << "        col.g += texture2D(tex, uvRefract + vec2( bStep.x,  0.0)).g * 0.20;\n";
        out << "        col.g += texture2D(tex, uvRefract + vec2(-bStep.x,  0.0)).g * 0.20;\n";
        out << "        col.g += texture2D(tex, uvRefract + vec2( 0.0,  bStep.y)).g * 0.15;\n";
        out << "        col.g += texture2D(tex, uvRefract + vec2( 0.0, -bStep.y)).g * 0.15;\n\n";

        out << "        col.b += texture2D(tex, uvRefract + offsetB).b * 0.30;\n";
        out << "        col.b += texture2D(tex, uvRefract + offsetB + vec2( bStep.x,  bStep.y)).b * 0.20;\n";
        out << "        col.b += texture2D(tex, uvRefract + offsetB + vec2(-bStep.x,  bStep.y)).b * 0.20;\n";
        out << "        col.b += texture2D(tex, uvRefract + offsetB + vec2( bStep.x, -bStep.y)).b * 0.15;\n";
        out << "        col.b += texture2D(tex, uvRefract + offsetB + vec2(-bStep.x, -bStep.y)).b * 0.15;\n";
    } else {
        out << "        float r = texture2D(tex, uvRefract + offsetR).r;\n";
        out << "        float g = texture2D(tex, uvRefract).g;\n";
        out << "        float b = texture2D(tex, uvRefract + offsetB).b;\n";
        out << "        vec3 col = vec3(r, g, b);\n";
    }

    if (vibrancy > 1.0f) {
        out << "        float gray = dot(col, vec3(0.299, 0.587, 0.114));\n";
        out << "        col = mix(vec3(gray), col, " << QString::number(vibrancy, 'f', 2) << ");\n";
    }

    // Specular Fresnel gloss reflection on upper edge
    out << "        float specular = clamp((0.5 - (p.y - hitCenter.y) / 40.0), 0.0, 1.0) * (1.0 - edgeFactor) * 0.20;\n";
    out << "        col += vec3(specular);\n";

    out << "        gl_FragColor = vec4(col, 1.0);\n";
    out << "    } else {\n";
    out << "        // ── Outside Glass: Clean Raw Live Screen ──\n";
    out << "        gl_FragColor = texture2D(tex, uv);\n";
    out << "    }\n";
    out << "}\n";

    return glsl;
}

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "screen_detector.h"

static void sendHyprlandSocketCommand(const std::string& cmd) {
    const char* his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!his || !his[0]) return;

    std::string sockPath = "/run/user/1000/hypr/" + std::string(his) + "/.socket.sock";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        (void)write(fd, cmd.c_str(), cmd.size());
    }
    close(fd);
}

void HyprlandGlassShader::activate(int screenWidth, int screenHeight,
                                   float centerX, float centerY,
                                   const QList<PillGeometry> &pills,
                                   float chromaticAberration,
                                   float blurRadius,
                                   float vibrancy,
                                   float refraction) {
    if (!isSupported()) return;

    Q_UNUSED(screenWidth);
    Q_UNUSED(screenHeight);
    Q_UNUSED(centerX);
    Q_UNUSED(centerY);
    Q_UNUSED(vibrancy);

    TargetScreenInfo screenInfo = ScreenDetector::getTargetScreenInfo();
    int monOffsetX = screenInfo.monitorX;
    int monOffsetY = screenInfo.monitorY;

    QString pillData;
    QTextStream ss(&pillData);

    for (int i = 0; i < pills.size(); ++i) {
        const PillGeometry &p = pills[i];
        float px = (float)monOffsetX + p.x - p.halfWidth;
        float py = (float)monOffsetY + p.y - p.halfHeight;
        float pw = p.halfWidth * 2.0f;
        float ph = p.halfHeight * 2.0f;
        float rad = p.radius > 0.0f ? p.radius : 18.0f;
        float blur = blurRadius > 0.0f ? blurRadius : 24.0f;
        float refr = refraction > 0.0f ? refraction : 0.85f;
        float chrom = chromaticAberration > 0.0f ? chromaticAberration : 1.4f;
        float spec = 0.70f;

        // format: x y w h radius blur refr chrom spec milkyR milkyG milkyB milkyA borderR borderG borderB borderA borderW;
        ss << QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 1.0 1.0 1.0 0.12 1.0 1.0 1.0 0.40 1.5;")
                .arg(px).arg(py).arg(pw).arg(ph).arg(rad).arg(blur).arg(refr).arg(chrom).arg(spec);
    }

    std::string luaCall = "/repl return hl.plugin.liquid_glass.set_pills([[" + pillData.toStdString() + "]])";
    sendHyprlandSocketCommand(luaCall);
}

void HyprlandGlassShader::deactivate() {
    if (!isSupported()) return;
    sendHyprlandSocketCommand("/repl return hl.plugin.liquid_glass.clear()");
}

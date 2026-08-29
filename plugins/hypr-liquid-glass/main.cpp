#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/pass/PassElement.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprutils/math/Misc.hpp>
#include <hyprutils/math/Mat3x3.hpp>
#include <lua.hpp>
#include <GLES3/gl3.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <fstream>
#include "Shaders.hpp"

static void logToFile(const std::string& msg) {
    std::ofstream ofs("/tmp/hypr_liquid_glass.log", std::ios::app);
    ofs << msg << std::endl;
}

inline HANDLE PHANDLE = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

struct GlassPill {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float radius = 18.0f;
    float blur = 24.0f;
    float refraction = 0.85f;
    float chromatic = 1.4f;
    float specular = 0.70f;
    float milkyTintR = 1.0f;
    float milkyTintG = 1.0f;
    float milkyTintB = 1.0f;
    float milkyTintA = 0.12f;
    float borderColorR = 1.0f;
    float borderColorG = 1.0f;
    float borderColorB = 1.0f;
    float borderColorA = 0.40f;
    float borderWidth = 1.5f;
};

static std::vector<GlassPill> g_pills;
static std::mutex g_pillMutex;

static std::atomic<uint64_t> g_renderCount{0};
static std::atomic<uint64_t> g_preRenderCount{0};
static std::atomic<uint64_t> g_drawCallCount{0};
static std::atomic<int> g_lastMonW{0};
static std::atomic<int> g_lastMonH{0};
static std::atomic<GLenum> g_lastGLError{GL_NO_ERROR};

static void renderLiquidGlassPills(PHLMONITOR pMonArg);
static void renderTriangles(PHLMONITOR pMonArg);

static SP<Render::IFramebuffer> g_pSurfaceTempFB;
static SP<Render::IFramebuffer> g_pSavedCurrentFB;

static GLuint g_program = 0;
static GLuint g_underlayTex = 0;
static int g_texW = 0;
static int g_texH = 0;
static GLuint g_vbo = 0;
static GLuint g_vao = 0;

static GLuint g_blitProg = 0;
static GLuint g_blitVao = 0;
static GLuint g_blitVbo = 0;

static const char* BLIT_VERT = R"#(
#version 300 es
precision highp float;
layout (location = 0) in vec2 a_pos;
out vec2 v_uv;
void main() {
    v_uv = a_pos * 0.5 + 0.5;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)#";

static const char* BLIT_FRAG = R"#(
#version 300 es
precision highp float;
in vec2 v_uv;
out vec4 fragColor;
uniform sampler2D u_tex;
void main() {
    vec4 c = texture(u_tex, v_uv);
    if (c.a <= 0.001) discard;
    fragColor = c;
}
)#";

static GLuint compileShader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* c_src = src.c_str();
    glShaderSource(shader, 1, &c_src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
        logToFile("Shader compilation failed: " + std::string(buf));
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint createProgram(const std::string& vsSrc, const std::string& fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        logToFile("Program link failed: " + std::string(buf));
        glDeleteProgram(prog);
        return 0;
    }
    logToFile("Program link SUCCESS: ID=" + std::to_string(prog));
    return prog;
}

static SP<Render::IFramebuffer> g_pUnderlayFB;

static void initGLResources() {
    if (!g_program) {
        g_program = createProgram(Shaders::LIQUID_GLASS_VERT, Shaders::LIQUID_GLASS_FRAG);
    }
    if (!g_vao) {
        glGenVertexArrays(1, &g_vao);
        glGenBuffers(1, &g_vbo);
        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        static const float unitQuad[8] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuad), unitQuad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    }
}

static void sampleCleanBackground(PHLMONITOR pMonArg) {
    if (!pMonArg || !g_pHyprRenderer) return;
    auto source = g_pHyprRenderer->m_renderData.currentFB;
    if (!source) return;

    int monW = (int)pMonArg->m_transformedSize.x;
    int monH = (int)pMonArg->m_transformedSize.y;
    if (monW <= 0 || monH <= 0) return;

    initGLResources();

    if (!g_pUnderlayFB) {
        g_pUnderlayFB = g_pHyprRenderer->createFB("hypr-liquid-glass-underlay");
    }
    if (g_pUnderlayFB->m_size.x != monW || g_pUnderlayFB->m_size.y != monH || g_pUnderlayFB->m_drmFormat != source->m_drmFormat) {
        g_pUnderlayFB->alloc(monW, monH, source->m_drmFormat);
    }

    Render::GL::g_pHyprOpenGL->setCapStatus(GL_SCISSOR_TEST, false);

    GLuint sampleFboId = dynamic_cast<Render::GL::CGLFramebuffer*>(g_pUnderlayFB.get())->getFBID();
    GLuint sourceFboId = dynamic_cast<Render::GL::CGLFramebuffer*>(source.get())->getFBID();

    glBindFramebuffer(GL_FRAMEBUFFER, sampleFboId);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFboId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sampleFboId);
    glBlitFramebuffer(0, 0, monW, monH, 0, 0, monW, monH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void blitTempFB(PHLMONITOR pMon, SP<Render::IFramebuffer> tempFB) {
    if (!tempFB || !pMon) return;
    auto tex = tempFB->getTexture();
    if (!tex) return;

    if (!g_blitProg) {
        g_blitProg = createProgram(BLIT_VERT, BLIT_FRAG);
        glGenVertexArrays(1, &g_blitVao);
        glGenBuffers(1, &g_blitVbo);
        glBindVertexArray(g_blitVao);
        glBindBuffer(GL_ARRAY_BUFFER, g_blitVbo);
        static const float quadVerts[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    }

    int monW = (int)pMon->m_transformedSize.x;
    int monH = (int)pMon->m_transformedSize.y;
    glViewport(0, 0, monW, monH);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_blitProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->m_texID);
    glUniform1i(glGetUniformLocation(g_blitProg, "u_tex"), 0);

    glBindVertexArray(g_blitVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

class CLiquidGlassPrePassElement : public IPassElement {
  public:
    explicit CLiquidGlassPrePassElement(PHLMONITOR monitor) : m_pMonitor(monitor) {}
    ~CLiquidGlassPrePassElement() override = default;

    std::vector<UP<IPassElement>> draw() override {
        if (!m_pMonitor || !g_pHyprRenderer) return {};
        auto source = g_pHyprRenderer->m_renderData.currentFB;
        if (!source) return {};

        int monW = (int)m_pMonitor->m_transformedSize.x;
        int monH = (int)m_pMonitor->m_transformedSize.y;

        // 1. Snapshot clean background BEFORE drmenu draws
        sampleCleanBackground(m_pMonitor);

        // 2. Allocate or resize temp FBO for drmenu UI
        DRMFormat tempFormat = (m_pMonitor->useFP16()) ? source->m_drmFormat : DRM_FORMAT_ARGB8888;
        if (!g_pSurfaceTempFB)
            g_pSurfaceTempFB = g_pHyprRenderer->createFB("hypr-liquid-glass-temp");

        if (g_pSurfaceTempFB->m_size.x != monW || g_pSurfaceTempFB->m_size.y != monH ||
            g_pSurfaceTempFB->m_drmFormat != tempFormat)
            g_pSurfaceTempFB->alloc(monW, monH, tempFormat);

        g_pSavedCurrentFB = source;

        // 3. Redirect currentFB to temp FBO cleared to transparent
        g_pHyprRenderer->m_renderData.currentFB = g_pSurfaceTempFB;
        glBindFramebuffer(GL_FRAMEBUFFER, dynamic_cast<Render::GL::CGLFramebuffer*>(g_pSurfaceTempFB.get())->getFBID());

        glViewport(0, 0, monW, monH);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        return {};
    }

    [[nodiscard]] bool needsLiveBlur() override { return true; }
    [[nodiscard]] bool needsPrecomputeBlur() override { return false; }
    [[nodiscard]] bool disableSimplification() override { return true; }
    [[nodiscard]] const char* passName() override { return "CLiquidGlassPrePassElement"; }
    [[nodiscard]] ePassElementType type() override { return EK_CUSTOM; }

    [[nodiscard]] std::optional<CBox> boundingBox() override {
        if (m_pMonitor) {
            return CBox{0, 0, m_pMonitor->m_size.x, m_pMonitor->m_size.y};
        }
        return std::nullopt;
    }

  private:
    PHLMONITOR m_pMonitor;
};

class CLiquidGlassCompositeElement : public IPassElement {
  public:
    explicit CLiquidGlassCompositeElement(PHLMONITOR monitor) : m_pMonitor(monitor) {}
    ~CLiquidGlassCompositeElement() override = default;

    std::vector<UP<IPassElement>> draw() override {
        if (!m_pMonitor || !g_pHyprRenderer) return {};

        // 1. Restore real monitor framebuffer
        if (g_pSavedCurrentFB) {
            g_pHyprRenderer->m_renderData.currentFB = g_pSavedCurrentFB;
            glBindFramebuffer(GL_FRAMEBUFFER, dynamic_cast<Render::GL::CGLFramebuffer*>(g_pSavedCurrentFB.get())->getFBID());
        }

        // 2. Draw liquid glass pills using clean background
        renderLiquidGlassPills(m_pMonitor);
        renderTriangles(m_pMonitor);

        // 3. Composite drmenu UI from temp FBO on top of glass
        if (g_pSurfaceTempFB) {
            blitTempFB(m_pMonitor, g_pSurfaceTempFB);
        }

        g_pSavedCurrentFB.reset();
        return {};
    }

    [[nodiscard]] bool needsLiveBlur() override { return false; }
    [[nodiscard]] bool needsPrecomputeBlur() override { return false; }
    [[nodiscard]] bool disableSimplification() override { return true; }
    [[nodiscard]] const char* passName() override { return "CLiquidGlassCompositeElement"; }
    [[nodiscard]] ePassElementType type() override { return EK_CUSTOM; }

    [[nodiscard]] std::optional<CBox> boundingBox() override {
        if (m_pMonitor) {
            return CBox{0, 0, m_pMonitor->m_size.x, m_pMonitor->m_size.y};
        }
        return std::nullopt;
    }

  private:
    PHLMONITOR m_pMonitor;
};

static void renderLiquidGlassPills(PHLMONITOR pMonArg) {
    std::vector<GlassPill> pillsCopy;
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        if (g_pills.empty()) return;
        pillsCopy = g_pills;
    }

    if (!pMonArg || !g_pHyprRenderer || !g_pHyprRenderer->m_renderData.currentFB) return;
    if (!g_pUnderlayFB || !g_pUnderlayFB->getTexture()) return;
    auto curFB = g_pHyprRenderer->m_renderData.currentFB;

    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    int monW = pMonArg->m_transformedSize.x > 0 ? (int)pMonArg->m_transformedSize.x : (curFB->m_size.x > 0 ? (int)curFB->m_size.x : (prevVp[2] > 0 ? prevVp[2] : 1920));
    int monH = pMonArg->m_transformedSize.y > 0 ? (int)pMonArg->m_transformedSize.y : (curFB->m_size.y > 0 ? (int)curFB->m_size.y : (prevVp[3] > 0 ? prevVp[3] : 1080));

    initGLResources();
    if (!g_program) return;

    g_renderCount++;
    g_lastMonW = monW;
    g_lastMonH = monH;

    // Save existing OpenGL state completely
    GLint prevProg = 0, prevVao = 0, prevVbo = 0, prevTex0 = 0, prevActiveTex = 0;
    GLint prevDrawFb = 0, prevReadFb = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFb);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFb);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex0);
    GLint prevScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
    GLboolean prevScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);
    GLboolean prevColorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

    curFB->bind();

    // Disable scissor and setup viewport for full unclipped drawing
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, monW, monH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_pUnderlayFB->getTexture()->m_texID);
    glUniform1i(glGetUniformLocation(g_program, "u_tex"), 0);
    glUniform2f(glGetUniformLocation(g_program, "u_resolution"), (float)monW, (float)monH);

    GLint uProj = glGetUniformLocation(g_program, "u_proj");
    GLint uRect = glGetUniformLocation(g_program, "u_pill_rect");
    GLint uRadius = glGetUniformLocation(g_program, "u_corner_radius");
    GLint uBlur = glGetUniformLocation(g_program, "u_blur_strength");
    GLint uRefr = glGetUniformLocation(g_program, "u_refraction_strength");
    GLint uChrom = glGetUniformLocation(g_program, "u_chromatic_aberration");
    GLint uSpec = glGetUniformLocation(g_program, "u_specular_strength");
    GLint uMilky = glGetUniformLocation(g_program, "u_milky_tint");
    GLint uBorderCol = glGetUniformLocation(g_program, "u_border_color");
    GLint uBorderW = glGetUniformLocation(g_program, "u_border_width");

    glBindVertexArray(g_vao);

    const auto transform = Math::wlTransformToHyprutils(
        Math::invertTransform(pMonArg->m_transform));

    for (const auto& pill : pillsCopy) {
        if (pill.w <= 0.0f || pill.h <= 0.0f) continue;

        CBox pillBox{pill.x, pill.y, pill.w, pill.h};
        Mat3x3 glMatrix = g_pHyprRenderer->projectBoxToTarget(pillBox, transform);
        glMatrix.transpose();

        glUniformMatrix3fv(uProj, 1, GL_FALSE, glMatrix.getMatrix().data());
        glUniform4f(uRect, pill.x, pill.y, pill.w, pill.h);
        glUniform1f(uRadius, pill.radius);
        glUniform1f(uBlur, pill.blur);
        glUniform1f(uRefr, pill.refraction);
        glUniform1f(uChrom, pill.chromatic);
        glUniform1f(uSpec, pill.specular);
        glUniform4f(uMilky, pill.milkyTintR, pill.milkyTintG, pill.milkyTintB, pill.milkyTintA);
        glUniform4f(uBorderCol, pill.borderColorR, pill.borderColorG, pill.borderColorB, pill.borderColorA);
        glUniform1f(uBorderW, pill.borderWidth);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        g_drawCallCount++;
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        g_lastGLError = err;
    }

    // Full state restoration
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFb);
    glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
    glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
    if (prevScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (!prevBlend) glDisable(GL_BLEND); else { glEnable(GL_BLEND); glBlendFunc(prevBlendSrc, prevBlendDst); }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prevTex0);
    glActiveTexture(prevActiveTex);
    glBindVertexArray(prevVao);
    glBindBuffer(GL_ARRAY_BUFFER, prevVbo);
    glUseProgram(prevProg);
}

struct OverlayTriangle {
    float x1 = 0.0f, y1 = 0.0f;
    float x2 = 0.0f, y2 = 0.0f;
    float x3 = 0.0f, y3 = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

static std::vector<OverlayTriangle> g_triangles;
static std::mutex g_triMutex;

static GLuint g_triProgram = 0;
static GLuint g_triVao = 0;
static GLuint g_triVbo = 0;

static const char* TRI_STATE_FILE = "/tmp/hypr_liquid_glass_tris.bin";
static std::atomic<int> g_activeTriangleCount{0};

static void updateActiveCountFromFile() {
    FILE* f = fopen(TRI_STATE_FILE, "rb");
    if (!f) { g_activeTriangleCount.store(0); return; }
    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, f) == 1) {
        g_activeTriangleCount.store((int)count);
    } else {
        g_activeTriangleCount.store(0);
    }
    fclose(f);
}

static void writeTrianglesToFile(const std::vector<OverlayTriangle>& tris) {
    FILE* f = fopen(TRI_STATE_FILE, "wb");
    if (!f) return;
    uint32_t count = tris.size();
    g_activeTriangleCount.store((int)count);
    fwrite(&count, sizeof(count), 1, f);
    if (count > 0)
        fwrite(tris.data(), sizeof(OverlayTriangle), count, f);
    fclose(f);
}

static std::vector<OverlayTriangle> readTrianglesFromFile() {
    FILE* f = fopen(TRI_STATE_FILE, "rb");
    if (!f) return {};
    uint32_t count = 0;
    if (fread(&count, sizeof(count), 1, f) != 1 || count == 0 || count > 10000) {
        fclose(f);
        return {};
    }
    std::vector<OverlayTriangle> tris(count);
    if (fread(tris.data(), sizeof(OverlayTriangle), count, f) != count) {
        fclose(f);
        return {};
    }
    fclose(f);
    return tris;
}

static bool g_triInitLogged = false;

static void renderTriangles(PHLMONITOR pMonArg) {
    auto trisCopy = readTrianglesFromFile();
    if (trisCopy.empty()) return;

    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    int monW = prevVp[2] > 0 ? prevVp[2] : 1920;
    int monH = prevVp[3] > 0 ? prevVp[3] : 1080;

    if (!g_triProgram) {
        const std::string vs = "#version 300 es\n"
                               "layout(location = 0) in vec2 a_pos;\n"
                               "layout(location = 1) in vec4 a_col;\n"
                               "out vec4 v_col;\n"
                               "void main() {\n"
                               "    v_col = a_col;\n"
                               "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
                               "}\n";
        const std::string fs = "#version 300 es\n"
                               "precision highp float;\n"
                               "in vec4 v_col;\n"
                               "out vec4 fragColor;\n"
                               "void main() {\n"
                               "    fragColor = v_col;\n"
                               "}\n";
        g_triProgram = createProgram(vs, fs);
        logToFile("renderTriangles: compiled shader, g_triProgram=" + std::to_string(g_triProgram));
    }
    if (!g_triVao) {
        glGenVertexArrays(1, &g_triVao);
        glGenBuffers(1, &g_triVbo);
        logToFile("renderTriangles: created VAO=" + std::to_string(g_triVao) + " VBO=" + std::to_string(g_triVbo));
    }
    if (!g_triProgram || !g_triVao) return;

    GLint curFb = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &curFb);
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);

    if (!g_triInitLogged) {
        logToFile("renderTriangles: first render active with " + std::to_string(trisCopy.size()) + 
                  " tris, vp=" + std::to_string(monW) + "x" + std::to_string(monH) + 
                  ", drawFB=" + std::to_string(curFb) + 
                  ", colorMask=[" + std::to_string((int)colorMask[0]) + "," + std::to_string((int)colorMask[1]) + "," + std::to_string((int)colorMask[2]) + "," + std::to_string((int)colorMask[3]) + "]" +
                  ", preErr=" + std::to_string(glGetError()));
        g_triInitLogged = true;
    }

    // Full OpenGL state preservation
    GLint prevProg = 0, prevVao = 0, prevVbo = 0, prevTex0 = 0, prevActiveTex = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex0);
    GLint prevScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
    GLboolean prevScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);
    GLboolean prevColorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, monW, monH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_triProgram);
    std::vector<float> vertexData;
    vertexData.reserve(trisCopy.size() * 3 * 6);

    for (const auto& tri : trisCopy) {
        float ndcX1 = (tri.x1 / (float)monW) * 2.0f - 1.0f;
        float ndcY1 = 1.0f - (tri.y1 / (float)monH) * 2.0f;

        float ndcX2 = (tri.x2 / (float)monW) * 2.0f - 1.0f;
        float ndcY2 = 1.0f - (tri.y2 / (float)monH) * 2.0f;

        float ndcX3 = (tri.x3 / (float)monW) * 2.0f - 1.0f;
        float ndcY3 = 1.0f - (tri.y3 / (float)monH) * 2.0f;

        vertexData.push_back(ndcX1); vertexData.push_back(ndcY1);
        vertexData.push_back(tri.r); vertexData.push_back(tri.g); vertexData.push_back(tri.b); vertexData.push_back(tri.a);

        vertexData.push_back(ndcX2); vertexData.push_back(ndcY2);
        vertexData.push_back(tri.r); vertexData.push_back(tri.g); vertexData.push_back(tri.b); vertexData.push_back(tri.a);

        vertexData.push_back(ndcX3); vertexData.push_back(ndcY3);
        vertexData.push_back(tri.r); vertexData.push_back(tri.g); vertexData.push_back(tri.b); vertexData.push_back(tri.a);
    }

    glBindVertexArray(g_triVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_triVbo);

    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));

    while (glGetError() != GL_NO_ERROR) {} // drain prior errors

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertexData.size() / 6));
    GLenum drawErr = glGetError();
    if (drawErr) {
        logToFile("renderTriangles: glDrawArrays error=" + std::to_string(drawErr));
    }

    // Full state restoration
    glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
    glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
    if (prevScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (!prevBlend) glDisable(GL_BLEND); else { glEnable(GL_BLEND); glBlendFunc(prevBlendSrc, prevBlendDst); }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, prevTex0);
    glActiveTexture(prevActiveTex);
    glBindVertexArray(prevVao);
    glBindBuffer(GL_ARRAY_BUFFER, prevVbo);
    glUseProgram(prevProg);
}

// Parses triangle string format: "x1 y1 x2 y2 x3 y3 [r g b a]; ..." or "x y w h [r g b a]; ..."
static void parseTrianglesString(const std::string& input) {
    std::vector<OverlayTriangle> newTris;
    std::stringstream ss(input);
    std::string line;

    while (std::getline(ss, line, ';')) {
        if (line.empty()) continue;
        std::stringstream ls(line);
        std::vector<float> nums;
        float val;
        while (ls >> val) {
            nums.push_back(val);
        }
        if (nums.size() >= 6) {
            OverlayTriangle tri;
            tri.x1 = nums[0]; tri.y1 = nums[1];
            tri.x2 = nums[2]; tri.y2 = nums[3];
            tri.x3 = nums[4]; tri.y3 = nums[5];
            if (nums.size() >= 10) {
                tri.r = nums[6]; tri.g = nums[7]; tri.b = nums[8]; tri.a = nums[9];
            } else if (nums.size() >= 9) {
                tri.r = nums[6]; tri.g = nums[7]; tri.b = nums[8];
            }
            newTris.push_back(tri);
        } else if (nums.size() >= 4) {
            float x = nums[0], y = nums[1], w = nums[2], h = nums[3];
            OverlayTriangle tri;
            tri.x1 = x + w * 0.5f; tri.y1 = y;
            tri.x2 = x;            tri.y2 = y + h;
            tri.x3 = x + w;        tri.y3 = y + h;
            if (nums.size() >= 8) {
                tri.r = nums[4]; tri.g = nums[5]; tri.b = nums[6]; tri.a = nums[7];
            }
            newTris.push_back(tri);
        }
    }

    logToFile("parseTrianglesString: received " + std::to_string(newTris.size()) + " triangles");

    writeTrianglesToFile(newTris);
}

// Parses string format: "x y w h [radius] [blur] [refr] [chrom] [spec] [milkyR milkyG milkyB milkyA] [borderR borderG borderB borderA borderW]"
static void parsePillsString(const std::string& input) {
    std::vector<GlassPill> newPills;
    std::stringstream ss(input);
    std::string line;

    while (std::getline(ss, line, ';')) {
        if (line.empty()) continue;
        std::stringstream ls(line);
        GlassPill pill;
        if (!(ls >> pill.x >> pill.y >> pill.w >> pill.h)) continue;
        ls >> pill.radius >> pill.blur >> pill.refraction >> pill.chromatic >> pill.specular;
        ls >> pill.milkyTintR >> pill.milkyTintG >> pill.milkyTintB >> pill.milkyTintA;
        ls >> pill.borderColorR >> pill.borderColorG >> pill.borderColorB >> pill.borderColorA >> pill.borderWidth;
        newPills.push_back(pill);
    }

    if (!newPills.empty()) {
        logToFile("parsePillsString: received " + std::to_string(newPills.size()) + " pills, p0=(" + std::to_string(newPills[0].x) + "," + std::to_string(newPills[0].y) + "," + std::to_string(newPills[0].w) + "," + std::to_string(newPills[0].h) + ") blur=" + std::to_string(newPills[0].blur) + " refr=" + std::to_string(newPills[0].refraction));
    }

    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        g_pills = std::move(newPills);
    }
}

static void scheduleRepaint() {
    try {
        if (g_pHyprRenderer) {
            g_pHyprRenderer->damageBox(0, 0, 99999, 99999);
        }
    } catch (...) {}
}

static SDispatchResult handleTriangles(std::string args) {
    parseTrianglesString(args);
    scheduleRepaint();
    return {false, true, ""};
}

static SDispatchResult handlePills(std::string args) {
    parsePillsString(args);
    scheduleRepaint();
    return {false, true, ""};
}

static SDispatchResult handleClear(std::string args) {
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        g_pills.clear();
    }
    writeTrianglesToFile({});
    scheduleRepaint();
    return {false, true, ""};
}

static int luaTriangles(lua_State* L) {
    const char* str = luaL_optstring(L, 1, "");
    parseTrianglesString(str);
    scheduleRepaint();
    lua_pushboolean(L, 1);
    return 1;
}

static int luaPills(lua_State* L) {
    const char* str = luaL_optstring(L, 1, "");
    parsePillsString(str);
    scheduleRepaint();
    lua_pushboolean(L, 1);
    return 1;
}

static int luaClear(lua_State* L) {
    logToFile("luaClear called");
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        g_pills.clear();
    }
    writeTrianglesToFile({});
    scheduleRepaint();
    lua_pushboolean(L, 1);
    return 1;
}

static void forceFrames() {
    updateActiveCountFromFile();
    scheduleRepaint();
}

static SDispatchResult handleRefresh(std::string args) {
    forceFrames();
    return {false, true, ""};
}

static int luaRefresh(lua_State* L) {
    forceFrames();
    lua_pushboolean(L, 1);
    return 1;
}

static int luaPing(lua_State* L) {
    size_t pillCount = 0;
    std::string pillSample = "none";
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        pillCount = g_pills.size();
        if (!g_pills.empty()) {
            const auto& p = g_pills[0];
            std::stringstream ps;
            ps << "[x=" << (int)p.x << " y=" << (int)p.y
               << " w=" << (int)p.w << " h=" << (int)p.h
               << " rad=" << (int)p.radius << " blur=" << (int)p.blur << "]";
            pillSample = ps.str();
        }
    }
    std::stringstream ss;
    ss << "PONG: hypr-liquid-glass v0.1.0\n"
       << "  * Active Pills: " << pillCount << " (sample: " << pillSample << ")\n"
       << "  * Shader Program: " << g_program << "\n"
       << "  * Underlay Texture: " << g_underlayTex << " (" << g_texW << "x" << g_texH << ")\n"
       << "  * Framebuffer Size: " << g_lastMonW.load() << "x" << g_lastMonH.load() << "\n"
       << "  * Frames Rendered: " << g_renderCount.load() << "\n"
       << "  * Draw Calls: " << g_drawCallCount.load() << "\n"
       << "  * Pre-Render Events: " << g_preRenderCount.load() << "\n"
       << "  * OpenGL State: " << (g_lastGLError.load() == GL_NO_ERROR ? "GL_NO_ERROR (OK)" : ("GL_ERROR " + std::to_string(g_lastGLError.load())));
    
    std::string response = ss.str();
    logToFile("luaPing called -> " + response);
    lua_pushstring(L, response.c_str());
    return 1;
}

static bool hasActiveShapes() {
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        if (!g_pills.empty()) return true;
    }
    return g_activeTriangleCount.load() > 0;
}

static CHyprSignalListener g_renderPreListener;

typedef void (*origRenderLayer)(void* thisptr, PHLLS pLS, PHLMONITOR pMonitor, const Time::steady_tp& now, bool popups, bool lockscreen);
static CFunctionHook* g_pRenderLayerHook = nullptr;

static void hkRenderLayer(void* thisptr, PHLLS layerSurface, PHLMONITOR monitor, const Time::steady_tp& now, bool popups, bool lockscreen) {
    if (!popups && layerSurface && layerSurface->m_namespace == "drmenu") {
        if (g_pHyprRenderer) {
            auto currentPosition = layerSurface->position(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
            auto currentSize     = layerSurface->size(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
            const float scale = monitor ? monitor->m_scale : 1.0f;
            auto box = CBox{currentPosition, currentSize};
            box.expand(32.0f / scale).noNegativeSize();
            if (box.w > 0.0 && box.h > 0.0)
                g_pHyprRenderer->damageBox(box);

            // 1. Pre-surface pass: snapshot clean background, redirect currentFB to temp FBO
            g_pHyprRenderer->m_renderPass.add(makeUnique<CLiquidGlassPrePassElement>(monitor));

            // 2. Render drmenu surface into temp FBO
            if (g_pRenderLayerHook && g_pRenderLayerHook->m_original) {
                ((origRenderLayer)g_pRenderLayerHook->m_original)(thisptr, layerSurface, monitor, now, popups, lockscreen);
            }

            // 3. Post-surface pass: restore currentFB, draw liquid glass, composite drmenu UI on top
            g_pHyprRenderer->m_renderPass.add(makeUnique<CLiquidGlassCompositeElement>(monitor));
            return;
        }
    }
    if (g_pRenderLayerHook && g_pRenderLayerHook->m_original) {
        ((origRenderLayer)g_pRenderLayerHook->m_original)(thisptr, layerSurface, monitor, now, popups, lockscreen);
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    logToFile("PLUGIN_INIT called");

    auto renderLayerMatches = HyprlandAPI::findFunctionsByName(PHANDLE, "renderLayer");
    for (const auto& match : renderLayerMatches) {
        if (match.demangled.contains("renderLayer") && match.demangled.contains("LayerSurface")) {
            g_pRenderLayerHook = HyprlandAPI::createFunctionHook(PHANDLE, match.address, (void*)hkRenderLayer);
            if (g_pRenderLayerHook) {
                g_pRenderLayerHook->hook();
                logToFile("renderLayer hooked successfully!");
            }
            break;
        }
    }

    g_renderPreListener = Event::bus()->m_events.render.pre.listen([](PHLMONITOR pMon) {
        g_preRenderCount++;
        try {
            if (pMon && pMon->m_enabled && hasActiveShapes()) {
                pMon->m_forceFullFrames = 2;
                if (g_pHyprRenderer)
                    g_pHyprRenderer->damageMonitor(pMon);
            }
        } catch (...) {}
    });

    SHyprCtlCommand cmdTriangles;
    cmdTriangles.name = "liquid_glass_triangles";
    cmdTriangles.exact = true;
    cmdTriangles.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        parseTrianglesString(args);
        scheduleRepaint();
        return "ok\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdTriangles);

    SHyprCtlCommand cmdPills;
    cmdPills.name = "liquid_glass_pills";
    cmdPills.exact = true;
    cmdPills.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        parsePillsString(args);
        scheduleRepaint();
        return "ok\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdPills);

    SHyprCtlCommand cmdClear;
    cmdClear.name = "liquid_glass_clear";
    cmdClear.exact = true;
    cmdClear.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        handleClear(args);
        return "ok\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdClear);

    SHyprCtlCommand cmdRefresh;
    cmdRefresh.name = "liquid_glass_refresh";
    cmdRefresh.exact = true;
    cmdRefresh.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        forceFrames();
        return "ok\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdRefresh);

    SHyprCtlCommand cmdPing;
    cmdPing.name = "liquid_glass_ping";
    cmdPing.exact = true;
    cmdPing.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        size_t pillCount = 0;
        {
            std::lock_guard<std::mutex> lock(g_pillMutex);
            pillCount = g_pills.size();
        }
        return "PONG: hypr-liquid-glass v0.1.0 is active (active pills: " + std::to_string(pillCount) + 
               ", shader program: " + std::to_string(g_program) + ")\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdPing);

    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_triangles", handleTriangles);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_pills", handlePills);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_clear", handleClear);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_refresh", handleRefresh);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_ping", [](std::string args) -> SDispatchResult {
        return {false, true, ""};
    });

    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "set_triangles", luaTriangles);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "set_pills", luaPills);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "clear", luaClear);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "refresh", luaRefresh);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "ping", luaPing);

    return {"hypr-liquid-glass", "Apple Liquid Glass & Vector Overlay Compositor", "drmenu", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    if (g_pRenderLayerHook) {
        g_pRenderLayerHook->unhook();
    }
    if (g_pHyprRenderer) {
        g_pHyprRenderer->m_renderPass.removeAllOfType("CLiquidGlassPrePassElement");
        g_pHyprRenderer->m_renderPass.removeAllOfType("CLiquidGlassCompositeElement");
    }
    g_pSurfaceTempFB.reset();
    g_pSavedCurrentFB.reset();
    g_renderPreListener.reset();
    PHANDLE = nullptr;
    logToFile("PLUGIN_EXIT cleanly finished");
}

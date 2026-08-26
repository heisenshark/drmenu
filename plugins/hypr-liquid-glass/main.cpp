#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/desktop/state/LayerState.hpp>
#include <hyprland/src/state/MonitorState.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <lua.hpp>
#include <GLES3/gl3.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <mutex>
#include "Shaders.hpp"

#include <fstream>

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

typedef void (*origRenderLayer)(void* thisptr, PHLLS pLS, PHLMONITOR pMonitor, const Time::steady_tp& now, bool popups, bool lockscreen);
static CFunctionHook* g_pRenderLayerHook = nullptr;

static GLuint g_program = 0;
static GLuint g_underlayTex = 0;
static int g_texW = 0;
static int g_texH = 0;
static GLuint g_vbo = 0;
static GLuint g_vao = 0;

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

static GLuint createProgram(const std::string& vSrc, const std::string& fSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fSrc);
    if (!vs || !fs) {
        logToFile("createProgram: failed to compile vs or fs");
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

static void initGLResources() {
    if (!g_program) {
        g_program = createProgram(Shaders::LIQUID_GLASS_VERT, Shaders::LIQUID_GLASS_FRAG);
    }
    if (!g_underlayTex) {
        glGenTextures(1, &g_underlayTex);
        glBindTexture(GL_TEXTURE_2D, g_underlayTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (!g_vao) {
        glGenVertexArrays(1, &g_vao);
        glGenBuffers(1, &g_vbo);
    }
}

static void renderLiquidGlassPills(PHLMONITOR pMonArg) {
    std::vector<GlassPill> pillsCopy;
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        if (g_pills.empty()) return;
        pillsCopy = g_pills;
    }

    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    int monW = prevVp[2] > 0 ? prevVp[2] : 1920;
    int monH = prevVp[3] > 0 ? prevVp[3] : 1080;

    initGLResources();
    if (!g_program) return;

    // Save existing OpenGL state completely
    GLint prevProg = 0, prevVao = 0, prevVbo = 0, prevTex = 0, prevActiveTex = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    GLint prevScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
    GLboolean prevScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);

    // Snapshot underlying screen texture for Poisson refractive sampling
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_underlayTex);
    if (g_texW != monW || g_texH != monH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, monW, monH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        g_texW = monW;
        g_texH = monH;
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, monW, monH);

    // Disable scissor and setup viewport for full unclipped drawing
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, monW, monH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_program);
    glUniform1i(glGetUniformLocation(g_program, "u_tex"), 0);
    glUniform2f(glGetUniformLocation(g_program, "u_resolution"), (float)monW, (float)monH);

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
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

    for (const auto& pill : pillsCopy) {
        if (pill.w <= 0.0f || pill.h <= 0.0f) continue;

        // Convert pixel coordinates to OpenGL NDC [-1, 1]
        float left = (pill.x / (float)monW) * 2.0f - 1.0f;
        float right = ((pill.x + pill.w) / (float)monW) * 2.0f - 1.0f;
        float top = 1.0f - (pill.y / (float)monH) * 2.0f;
        float bottom = 1.0f - ((pill.y + pill.h) / (float)monH) * 2.0f;

        float quadVertices[] = {
            left,  top,    0.0f, 0.0f,
            left,  bottom, 0.0f, 1.0f,
            right, top,    1.0f, 0.0f,
            right, bottom, 1.0f, 1.0f,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

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
    }

    // Full state restoration
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
    glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
    if (prevScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (!prevBlend) glDisable(GL_BLEND); else { glEnable(GL_BLEND); glBlendFunc(prevBlendSrc, prevBlendDst); }
    glActiveTexture(prevActiveTex);
    glBindTexture(GL_TEXTURE_2D, prevTex);
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

static void writeTrianglesToFile(const std::vector<OverlayTriangle>& tris) {
    FILE* f = fopen(TRI_STATE_FILE, "wb");
    if (!f) return;
    uint32_t count = tris.size();
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
    GLint prevProg = 0, prevVao = 0, prevVbo = 0, prevTex = 0, prevActiveTex = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    GLint prevScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
    GLboolean prevScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, monW, monH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_triProgram);
    std::vector<float> vertexData;
    vertexData.reserve((trisCopy.size() + 1) * 3 * 6);

    // Add a guaranteed giant center white triangle in NDC: [-0.6, -0.6], [0.6, -0.6], [0.0, 0.6]
    vertexData.push_back(-0.6f); vertexData.push_back(-0.6f);
    vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f);

    vertexData.push_back(0.6f); vertexData.push_back(-0.6f);
    vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f);

    vertexData.push_back(0.0f); vertexData.push_back(0.6f);
    vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f); vertexData.push_back(1.0f);

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
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
    glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
    if (prevScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (!prevBlend) glDisable(GL_BLEND); else { glEnable(GL_BLEND); glBlendFunc(prevBlendSrc, prevBlendDst); }
    glActiveTexture(prevActiveTex);
    glBindTexture(GL_TEXTURE_2D, prevTex);
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

    if (State::monitorState()) {
        for (auto& mon : State::monitorState()->monitors()) {
            if (mon && mon->m_enabled) {
                mon->m_forceFullFrames = 4;
                mon->scheduleFrame();
            }
        }
    }
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

    logToFile("parsePillsString: received " + std::to_string(newPills.size()) + " pills");

    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        g_pills = std::move(newPills);
    }

    if (State::monitorState()) {
        for (auto& mon : State::monitorState()->monitors()) {
            if (mon && mon->m_enabled) {
                mon->m_forceFullFrames = 4;
                mon->scheduleFrame();
            }
        }
    }
}

static SDispatchResult handleTriangles(std::string args) {
    parseTrianglesString(args);
    return {false, true, ""};
}

static SDispatchResult handlePills(std::string args) {
    parsePillsString(args);
    return {false, true, ""};
}

static SDispatchResult handleClear(std::string args) {
    {
        std::lock_guard<std::mutex> lock(g_pillMutex);
        g_pills.clear();
    }
    writeTrianglesToFile({});
    if (State::monitorState()) {
        for (auto& mon : State::monitorState()->monitors()) {
            if (mon && mon->m_enabled) {
                mon->m_forceFullFrames = 4;
                mon->scheduleFrame();
            }
        }
    }
    return {false, true, ""};
}

static int luaTriangles(lua_State* L) {
    const char* str = luaL_optstring(L, 1, "");
    parseTrianglesString(str);
    lua_pushboolean(L, 1);
    return 1;
}

static int luaPills(lua_State* L) {
    const char* str = luaL_optstring(L, 1, "");
    parsePillsString(str);
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
    if (State::monitorState()) {
        for (auto& mon : State::monitorState()->monitors()) {
            if (mon && mon->m_enabled) {
                mon->m_forceFullFrames = 4;
                mon->scheduleFrame();
            }
        }
    }
    lua_pushboolean(L, 1);
    return 1;
}

static void forceFrames() {
    if (State::monitorState()) {
        for (auto& mon : State::monitorState()->monitors()) {
            if (mon && mon->m_enabled) {
                if (g_pHyprRenderer)
                    g_pHyprRenderer->damageMonitor(mon);
                mon->m_forceFullFrames = 4;
                mon->scheduleFrame();
            }
        }
    }
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

static CHyprSignalListener g_renderStageListener;

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    logToFile("PLUGIN_INIT called");

    // Clean up any lingering trampoline hooks from previous in-memory compilations
    if (g_pFunctionHookSystem) {
        struct HookSystemHack {
            std::vector<UP<CFunctionHook>> m_hooks;
        };
        auto* hack = reinterpret_cast<HookSystemHack*>(g_pFunctionHookSystem.get());
        for (auto& hook : hack->m_hooks) {
            if (hook) {
                hook->unhook();
            }
        }
        hack->m_hooks.clear();
        logToFile("All lingering hooks unhooked and cleared successfully!");
    }

    g_renderStageListener = Event::bus()->m_events.render.stage.registerListener([](std::any d) {
        try {
            eRenderStage stage = std::any_cast<eRenderStage>(d);
            if (stage == RENDER_LAST_MOMENT || stage == RENDER_POST_WINDOWS) {
                renderLiquidGlassPills(nullptr);
                renderTriangles(nullptr);
            }
        } catch (...) {}
    });
    logToFile("render.stage listener registered successfully!");

    SHyprCtlCommand cmdTriangles;
    cmdTriangles.name = "liquid_glass_triangles";
    cmdTriangles.exact = true;
    cmdTriangles.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        parseTrianglesString(args);
        return "ok\n";
    };
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, cmdTriangles);

    SHyprCtlCommand cmdPills;
    cmdPills.name = "liquid_glass_pills";
    cmdPills.exact = true;
    cmdPills.fn = [](eHyprCtlOutputFormat format, std::string args) -> std::string {
        parsePillsString(args);
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

    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_triangles", handleTriangles);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_pills", handlePills);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_clear", handleClear);
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid_glass_refresh", handleRefresh);

    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "set_triangles", luaTriangles);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "set_pills", luaPills);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "clear", luaClear);
    HyprlandAPI::addLuaFunction(PHANDLE, "liquid_glass", "refresh", luaRefresh);

    return {"hypr-liquid-glass", "Apple Liquid Glass & Vector Overlay Compositor", "drmenu", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    g_renderStageListener = nullptr;
    if (g_triProgram) {
        glDeleteProgram(g_triProgram);
        g_triProgram = 0;
    }
    if (g_triVbo) {
        glDeleteBuffers(1, &g_triVbo);
        g_triVbo = 0;
    }
    if (g_triVao) {
        glDeleteVertexArrays(1, &g_triVao);
        g_triVao = 0;
    }
    if (g_program) {
        glDeleteProgram(g_program);
        g_program = 0;
    }
    if (g_underlayTex) {
        glDeleteTextures(1, &g_underlayTex);
        g_underlayTex = 0;
    }
    if (g_vbo) {
        glDeleteBuffers(1, &g_vbo);
        g_vbo = 0;
    }
    if (g_vao) {
        glDeleteVertexArrays(1, &g_vao);
        g_vao = 0;
    }
    PHANDLE = nullptr;
}

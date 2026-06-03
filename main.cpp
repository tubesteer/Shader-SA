#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

MYMODCFG(com.username.sa.gles3loader, GTASA_ShaderLoader30, 1.0, Username)

extern GLuint LoadPNGFromStorage(const char* path);
uintptr_t pGameLibrary = 0;

EGLContext (*orig_eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLContext hook_eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    EGLint gles3_attribs[] = {
        0x3098, 3, 
        EGL_NONE
    };
    EGLContext context = orig_eglCreateContext(dpy, config, share_context, gles3_attribs);
    if (context != EGL_NO_CONTEXT) {
        logger->Info("ShaderLoader: OpenGL ES 3.0 Context Created.");
    } else {
        context = orig_eglCreateContext(dpy, config, share_context, attrib_list);
    }
    return context;
}

std::string ReadShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string UpgradeShaderToGLSL3(const std::string& originalSource, int shaderType) {
    std::string upgraded = "#version 300 es\nprecision highp float;\nprecision highp sampler2D;\n";
    std::string line;
    std::stringstream ss(originalSource);
    
    while (std::getline(ss, line)) {
        if (line.find("precision ") != std::string::npos) continue;
        if (shaderType == 0x8B31) {
            if (line.find("attribute ") != std::string::npos) line.replace(line.find("attribute "), 10, "in ");
            if (line.find("varying ") != std::string::npos) line.replace(line.find("varying "), 8, "out ");
        } 
        else if (shaderType == 0x8B30) {
            if (line.find("varying ") != std::string::npos) line.replace(line.find("varying "), 8, "in ");
            if (line.find("gl_FragColor") != std::string::npos) {
                size_t pos;
                while ((pos = line.find("gl_FragColor")) != std::string::npos) line.replace(pos, 12, "fragOutColor");
            }
        }
        size_t texPos;
        while ((texPos = line.find("texture2D")) != std::string::npos) line.replace(texPos, 9, "texture");
        upgraded += line + "\n";
    }

    if (shaderType == 0x8B30 && upgraded.find("fragOutColor") != std::string::npos) {
        size_t insertPos = upgraded.find("\n", upgraded.find("#version 300 es"));
        upgraded.insert(insertPos + 1, "out vec4 fragOutColor;\n");
    }
    return upgraded;
}

uintptr_t (*orig_rwOpenGLShaderCompile)(int shaderType, const char* source);
uintptr_t hook_rwOpenGLShaderCompile(int shaderType, const char* source)
{
    std::string finalShaderSource;
    std::string customPath = "/sdcard/Android/data/com.rockstargames.gtasa/";
    customPath += (shaderType == 0x8B31) ? "custom_vshader.glsl" : "custom_fshader.glsl";

    std::string externalShader = ReadShaderFile(customPath.c_str());

    if (!externalShader.empty()) {
        finalShaderSource = externalShader;
    } else {
        finalShaderSource = UpgradeShaderToGLSL3(source, shaderType);
    }

    return orig_rwOpenGLShaderCompile(shaderType, finalShaderSource.c_str());
}

void* ScanPattern(uintptr_t base, size_t size, const unsigned char* pattern, size_t patternSize) {
    for (size_t i = 0; i < size - patternSize; ++i) {
        if (std::memcmp(reinterpret_cast<void*>(base + i), pattern, patternSize) == 0) {
            return reinterpret_cast<void*>(base + i);
        }
    }
    return nullptr;
}

extern "C" void OnModLoad()
{
    logger->SetTag("ShaderLoader30");

    uintptr_t libEGL = aml->GetLib("libEGL.so");
    if (libEGL) {
        aml->Hook((void*)aml->GetSym(libEGL, "eglCreateContext"), 
                  (void*)hook_eglCreateContext, 
                  (void**)&orig_eglCreateContext);
    }

    pGameLibrary = aml->GetLib("libGTASA.so");
    if (pGameLibrary)
    {
        void* shaderCompileSym = (void*)aml->GetSym(pGameLibrary, "_Z21rwOpenGLShaderCompileP14RwShaderSourcePKc");
        if (!shaderCompileSym) {
            shaderCompileSym = (void*)aml->GetSym(pGameLibrary, "_Z21rwOpenGLShaderCompilejPKc");
        }

        if (!shaderCompileSym) {
            unsigned char pattern[] = { 0x2D, 0xE4, 0x2D, 0xE9, 0x24, 0x00, 0x9F, 0xE5, 0x10, 0x40, 0x2D, 0xE9, 0x05, 0x40, 0xA0, 0xE1, 0x00, 0x50, 0xA0, 0xE1 };
            shaderCompileSym = ScanPattern(pGameLibrary, 0x400000, pattern, sizeof(pattern));
        }

        if (shaderCompileSym) {
            aml->Hook(shaderCompileSym, 
                      (void*)hook_rwOpenGLShaderCompile, 
                      (void**)&orig_rwOpenGLShaderCompile);
            logger->Info("ShaderLoader: Hooked successfully via dynamic detection.");
        } else {
            logger->Error("ShaderLoader: Failed to find target function.");
        }
    }

    cfg->Save();
}

#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include <fstream>
#include <sstream>

// Menggunakan makro konfigurasi bawaan template RusJJ
MYMODCFG(com.username.sa.gles3loader, GTASA_ShaderLoader30, 1.0, Username)

// Mengambil fungsi dari texture_loader.cpp jika nanti dibutuhkan
extern GLuint LoadPNGFromStorage(const char* path);

uintptr_t pGameLibrary = 0;

// --- HOOK EGL CONTEXT (FOR GLES 3.0) ---
EGLContext (*orig_eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLContext hook_eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    logger->Info("ShaderLoader: Mengintersepsi eglCreateContext untuk GLES 3.0...");
    EGLint gles3_attribs[] = {
        0x3098, 3, // EGL_CONTEXT_CLIENT_VERSION = 3
        EGL_NONE
    };
    EGLContext context = orig_eglCreateContext(dpy, config, share_context, gles3_attribs);
    if (context != EGL_NO_CONTEXT) {
        logger->Info("ShaderLoader: Sukses beralih ke OpenGL ES 3.0!");
    } else {
        logger->Error("ShaderLoader: Gagal membuat konteks GLES 3.0, fallback ke default.");
        context = orig_eglCreateContext(dpy, config, share_context, attrib_list);
    }
    return context;
}

// --- HELPER: MEMBACA FILE SHADER EKSTERNAL ---
std::string ReadShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --- HELPER: KONVERSI OTOMATIS GLSL 2 KE GLSL 3 ---
std::string UpgradeShaderToGLSL3(const std::string& originalSource, int shaderType) {
    std::string upgraded = "#version 300 es\n";
    upgraded += "precision highp float;\n";
    upgraded += "precision highp sampler2D;\n";

    std::string line;
    std::stringstream ss(originalSource);
    
    while (std::getline(ss, line)) {
        if (line.find("precision ") != std::string::npos) continue;
        
        if (shaderType == 0x8B31) { // Vertex Shader
            if (line.find("attribute ") != std::string::npos) {
                size_t pos = line.find("attribute ");
                line.replace(pos, 10, "in ");
            }
            if (line.find("varying ") != std::string::npos) {
                size_t pos = line.find("varying ");
                line.replace(pos, 8, "out ");
            }
        } 
        else if (shaderType == 0x8B30) { // Fragment Shader
            if (line.find("varying ") != std::string::npos) {
                size_t pos = line.find("varying ");
                line.replace(pos, 8, "in ");
            }
            if (line.find("gl_FragColor") != std::string::npos) {
                size_t pos;
                while ((pos = line.find("gl_FragColor")) != std::string::npos) {
                    line.replace(pos, 12, "fragOutColor");
                }
            }
        }

        size_t texPos;
        while ((texPos = line.find("texture2D")) != std::string::npos) {
            line.replace(texPos, 9, "texture");
        }

        upgraded += line + "\n";
    }

    if (shaderType == 0x8B30 && upgraded.find("fragOutColor") != std::string::npos) {
        size_t insertPos = upgraded.find("\n", upgraded.find("#version 300 es"));
        upgraded.insert(insertPos + 1, "out vec4 fragOutColor;\n");
    }

    return upgraded;
}

// --- HOOK COMPILER SHADER INTERNAL (OFFSET 0x1BD150) ---
void* (*orig_rwOpenGLShaderCompile)(int shaderType, const char* source);
void* hook_rwOpenGLShaderCompile(int shaderType, const char* source)
{
    std::string finalShaderSource;
    std::string customPath = "/sdcard/Android/data/com.rockstargames.gtasa/";
    customPath += (shaderType == 0x8B31) ? "custom_vshader.glsl" : "custom_fshader.glsl";

    std::string externalShader = ReadShaderFile(customPath.c_str());

    if (!externalShader.empty()) {
        logger->Info("ShaderLoader: Menggunakan shader luar dari %s", customPath.c_str());
        finalShaderSource = externalShader;
    } else {
        finalShaderSource = UpgradeShaderToGLSL3(source, shaderType);
    }

    return orig_rwOpenGLShaderCompile(shaderType, finalShaderSource.c_str());
}

// --- ENTRY POINT PLUGIN ---
extern "C" void OnModLoad()
{
    // Mengatur nama tag log khusus sesuai template
    logger->SetTag("ShaderLoader30");
    logger->Info("ShaderLoader: Memulai injeksi sistem...");

    // 1. Jalankan EGL Hook ke libEGL.so bawaan Android
    uintptr_t libEGL = aml->GetLib("libEGL.so");
    if (libEGL) {
        aml->Hook((void*)aml->GetSym(libEGL, "eglCreateContext"), 
                  (void*)hook_eglCreateContext, 
                  (void**)&orig_eglCreateContext);
        logger->Info("ShaderLoader: Jembatan GLES 3.0 siap.");
    }

    // 2. Jalankan Shader Compiler Hook ke libGTASA.so v2.00
    pGameLibrary = aml->GetLib("libGTASA.so");
    if (pGameLibrary)
    {
        logger->Info("ShaderLoader: Target libGTASA.so v2.00 ditemukan!");
        
        // Hook fungsi internal kompilasi shader (Offset: 0x1BD150)
        aml->Hook((void*)(pGameLibrary + 0x1BD150), 
                  (void*)hook_rwOpenGLShaderCompile, 
                  (void**)&orig_rwOpenGLShaderCompile);
                  
        logger->Info("ShaderLoader: Semua fungsi grafis berhasil di-hook!");
    }
    else
    {
        logger->Error("ShaderLoader: libGTASA.so tidak terdeteksi. Pastikan versi game 2.00!");
        return;
    }

    // Simpan konfigurasi otomatis bawaan template jika ada perubahan
    cfg->Save();
}

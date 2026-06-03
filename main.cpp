#include "mod/amlmod.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include <fstream>
#include <sstream>

// Identitas Plugin AML
MYMOD(com.username.sa.gles3loader, GTASA_ShaderLoader30, 1.0, Username)

// Deklarasi fungsi dari texture_loader.cpp agar bisa dipakai di sini jika dibutuhkan
extern GLuint LoadPNGFromStorage(const char* path);

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
        // Abaikan presisi bawaan lama agar tidak bentrok
        if (line.find("precision ") != std::string::npos) continue;
        
        // Konversi sintaks Vertex Shader (Tipe 0x8B31 = GL_VERTEX_SHADER)
        if (shaderType == 0x8B31) {
            if (line.find("attribute ") != std::string::npos) {
                size_t pos = line.find("attribute ");
                line.replace(pos, 10, "in ");
            }
            if (line.find("varying ") != std::string::npos) {
                size_t pos = line.find("varying ");
                line.replace(pos, 8, "out ");
            }
        } 
        // Konversi sintaks Fragment Shader (Tipe 0x8B30 = GL_FRAGMENT_SHADER)
        else if (shaderType == 0x8B30) {
            if (line.find("varying ") != std::string::npos) {
                size_t pos = line.find("varying ");
                line.replace(pos, 8, "in ");
            }
            // Di GLSL 3, gl_FragColor didepak. Kita ganti dengan out variabel kustom
            if (line.find("gl_FragColor") != std::string::npos) {
                size_t pos;
                while ((pos = line.find("gl_FragColor")) != std::string::npos) {
                    line.replace(pos, 12, "fragOutColor");
                }
            }
        }

        // Konversi fungsi pembacaan tekstur lama ke fungsi modern 'texture'
        size_t texPos;
        while ((texPos = line.find("texture2D")) != std::string::npos) {
            line.replace(texPos, 9, "texture");
        }

        upgraded += line + "\n";
    }

    // Tambahkan deklarasi output warna di Fragment Shader jika mendeteksi fragOutColor
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
    
    // Tentukan path jika kamu ingin membuat file override kustom di SDCard
    // Misal: /sdcard/Android/data/com.rockstargames.gtasa/vshader.glsl
    std::string customPath = "/sdcard/Android/data/com.rockstargames.gtasa/";
    customPath += (shaderType == 0x8B31) ? "custom_vshader.glsl" : "custom_fshader.glsl";

    std::string externalShader = ReadShaderFile(customPath.c_str());

    if (!externalShader.empty()) {
        // Jika ada file shader kustom di folder game, pakai file itu!
        logger->Info("ShaderLoader: Menggunakan shader luar dari %s", customPath.c_str());
        finalShaderSource = externalShader;
    } else {
        // Jika tidak ada, pakai shader asli game tapi paksa upgrade ke GLSL versi 3
        finalShaderSource = UpgradeShaderToGLSL3(source, shaderType);
    }

    // Eksekusi fungsi kompilasi asli menggunakan source code baru yang sudah berbasis GLSL 3
    return orig_rwOpenGLShaderCompile(shaderType, finalShaderSource.c_str());
}

// --- ENTRY POINT PLUGIN ---
extern "C" void OnModLoad()
{
    logger->Info("ShaderLoader: Memulai injeksi sistem...");

    // 1. Jalankan EGL Hook ke libEGL.so bawaan Android
    void* libEGL = aml->GetLib("libEGL.so");
    if (libEGL) {
        aml->HookFunc(aml->GetSym(libEGL, "eglCreateContext"), 
                      (void*)hook_eglCreateContext, 
                      (void**)&orig_eglCreateContext);
        logger->Info("ShaderLoader: Jembatan GLES 3.0 siap.");
    }

    // 2. Jalankan Shader Compiler Hook ke libGTASA.so v2.00
    uintptr_t libBase = aml->GetLib("libGTASA.so");
    if (libBase) {
        logger->Info("ShaderLoader: Target libGTASA.so v2.00 ditemukan.");
        
        // Hook fungsi internal kompilasi shader (Offset: 0x1BD150)
        aml->Hook((void*)(libBase + 0x1BD150), 
                  (void*)hook_rwOpenGLShaderCompile, 
                  (void**)&orig_rwOpenGLShaderCompile);
                  
        logger->Info("ShaderLoader: Semua fungsi grafis berhasil di-hook!");
    } else {
        logger->Error("ShaderLoader: libGTASA.so tidak terdeteksi. Pastikan versi game 2.00!");
    }
}

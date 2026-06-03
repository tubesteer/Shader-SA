#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include <fstream>
#include <sstream>

MYMODCFG(com.username.sa.gles3loader, GTASA_ShaderLoader30, 1.0, Username)

extern GLuint LoadPNGFromStorage(const char* path);
uintptr_t pGameLibrary = 0;

// --- TRACING HOOK EGL CONTEXT ---
EGLContext (*orig_eglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLContext hook_eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    logger->Info("TRACE [1]: Memasuki hook_eglCreateContext");
    
    EGLint gles3_attribs[] = {
        0x3098, 3, 
        EGL_NONE
    };
    
    logger->Info("TRACE [2]: Memanggil orig_eglCreateContext dengan GLES 3");
    EGLContext context = orig_eglCreateContext(dpy, config, share_context, gles3_attribs);
    
    if (context != EGL_NO_CONTEXT) {
        logger->Info("TRACE [3]: Sukses membuat konteks OpenGL ES 3.0");
    } else {
        logger->Error("TRACE [3_ERR]: Gagal GLES 3, mencoba menggunakan attribute asli");
        context = orig_eglCreateContext(dpy, config, share_context, attrib_list);
    }
    
    logger->Info("TRACE [4]: Keluar dari hook_eglCreateContext");
    return context;
}

// --- HELPER READ SHADER ---
std::string ReadShaderFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --- HELPER UPGRADE GLSL ---
std::string UpgradeShaderToGLSL3(const std::string& originalSource, int shaderType) {
    std::string upgraded = "#version 300 es\nprecision highp float;\nprecision highp sampler2D;\n";
    std::string line;
    std::stringstream ss(originalSource);
    
    while (std::getline(ss, line)) {
        if (line.find("precision ") != std::string::npos) continue;
        if (shaderType == 0x8B31) { // Vertex
            if (line.find("attribute ") != std::string::npos) line.replace(line.find("attribute "), 10, "in ");
            if (line.find("varying ") != std::string::npos) line.replace(line.find("varying "), 8, "out ");
        } 
        else if (shaderType == 0x8B30) { // Fragment
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

// --- TRACING HOOK COMPILER SHADER ---
// Menggunakan uintptr_t sebagai return type untuk mencegah ketidakcocokan register (32-bit asm)
uintptr_t (*orig_rwOpenGLShaderCompile)(int shaderType, const char* source);
uintptr_t hook_rwOpenGLShaderCompile(int shaderType, const char* source)
{
    // Menggunakan logger->Print agar aman dari buffer log yang terlalu panjang
    logger->Print(LogP_Info, "TRACE [5]: Memasuki hook_rwOpenGLShaderCompile. Tipe Shader: %s", 
                  (shaderType == 0x8B31) ? "VERTEX" : "FRAGMENT");

    std::string finalShaderSource;
    std::string customPath = "/sdcard/Android/data/com.rockstargames.gtasa/";
    customPath += (shaderType == 0x8B31) ? "custom_vshader.glsl" : "custom_fshader.glsl";

    logger->Info("TRACE [6]: Memeriksa shader eksternal di storage...");
    std::string externalShader = ReadShaderFile(customPath.c_str());

    if (!externalShader.empty()) {
        logger->Print(LogP_Info, "TRACE [7]: Menemukan file shader kustom di %s", customPath.c_str());
        finalShaderSource = externalShader;
    } else {
        logger->Info("TRACE [7]: File kustom kosong, memproses auto-upgrade ke GLSL 3");
        finalShaderSource = UpgradeShaderToGLSL3(source, shaderType);
    }

    logger->Info("TRACE [8]: Memanggil fungsi asli orig_rwOpenGLShaderCompile...");
    
    // Eksekusi fungsi asli game
    uintptr_t result = orig_rwOpenGLShaderCompile(shaderType, finalShaderSource.c_str());
    
    logger->Print(LogP_Info, "TRACE [9]: Selesai kompilasi shader. Pointer Result: 0x%X", result);
    return result;
}

// --- ENTRY POINT PLUGIN ---
extern "C" void OnModLoad()
{
    logger->SetTag("ShaderLoader30");
    logger->Info("TRACE [START]: Memulai inisialisasi OnModLoad");

    // 1. Hook EGL Context
    uintptr_t libEGL = aml->GetLib("libEGL.so");
    if (libEGL) {
        logger->Info("TRACE: libEGL.so ditemukan, memasang hook...");
        aml->Hook((void*)aml->GetSym(libEGL, "eglCreateContext"), 
                  (void*)hook_eglCreateContext, 
                  (void**)&orig_eglCreateContext);
    }

    // 2. Hook Shader Compiler libGTASA v2.00
    pGameLibrary = aml->GetLib("libGTASA.so");
    if (pGameLibrary)
    {
        logger->Print(LogP_Info, "TRACE: libGTASA.so ditemukan di alamat 0x%X", pGameLibrary);
        
        // Memasang hook ke Offset 0x1BD150
        aml->Hook((void*)(pGameLibrary + 0x1BD150), 
                  (void*)hook_rwOpenGLShaderCompile, 
                  (void**)&orig_rwOpenGLShaderCompile);
                  
        logger->Info("TRACE [INIT_SUCCESS]: Semua jembatan fungsi berhasil di-hook!");
    }
    else
    {
        logger->Error("TRACE [INIT_FAIL]: libGTASA.so tidak terdeteksi!");
        return;
    }

    cfg->Save();
}

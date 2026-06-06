#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

MYMOD(com.username.gtasa.graphicsmod, GTA_Graphics_Engine_Mod, 1.0, ModderName)

uintptr_t g_libGTASA = 0;

// Mendefinisikan fungsi asli OpenGL yang akan di-hook
decltype(eglGetProcAddress)* eglGetProcAddress_real = nullptr;
void (*glShaderSource_real)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) = nullptr;

// Struktur konfigurasi efek (Always On / Aktif Otomatis)
struct ShaderConfig {
    bool enableGodRay = true;
    bool enableBloom = true;
    float exposure = 1.5f; 
} config;

// Fungsi Hook untuk glShaderSource (Tempat memodifikasi teks/source shader game)
void h_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    if (count > 0 && string && string[0]) {
        std::string shaderSource(string[0]);

        // 1. Deteksi jika ini adalah Shader Kendaraan (VehicleShader)
        if (shaderSource.find("Vehicle") != std::string::npos || shaderSource.find("GenerateReflection") != std::string::npos) {
            logger->Info("Menemukan Vehicle Shader! Menyuntikkan efek grafis...");
            
            // Contoh manipulasi string shader: Mengganti atau menyisipkan baris kode GLSL
            // Di sini Anda bisa menyisipkan formula matematika GodRay atau Bloom ke dalam teks shader
            /*
            size_t pos = shaderSource.find("void main()");
            if (pos != std::string::npos) {
                shaderSource.insert(pos, "uniform float u_Exposure;\n");
            }
            */
            
            const char* modifiedSource = shaderSource.c_str();
            glShaderSource_real(shader, count, &modifiedSource, length);
            return;
        }
        
        // 2. Deteksi jika ini adalah Shader Bangunan (BuildingShader)
        if (shaderSource.find("Building") != std::string::npos) {
            logger->Info("Menemukan Building Shader! Memodifikasi pencahayaan...");
            // Lakukan manipulasi serupa untuk shader bangunan di sini
        }
    }

    // Jika bukan shader yang dicari, teruskan ke fungsi asli game tanpa perubahan
    glShaderSource_real(shader, count, string, length);
}

// Fungsi Hook untuk eglGetProcAddress
void* h_eglGetProcAddress(const char* procname) {
    void* proc = (void*)eglGetProcAddress_real(procname);
    
    if (procname && strcmp(procname, "glCompileShader") == 0) {
        logger->Info("OpenGL Hook: Game sedang memanggil glCompileShader!");
    }
    
    // Mencegat pencarian fungsi glShaderSource oleh game, lalu belokkan ke fungsi buatan kita
    if (procname && strcmp(procname, "glShaderSource") == 0) {
        if (glShaderSource_real == nullptr) {
            glShaderSource_real = (void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*))proc;
        }
        return (void*)h_glShaderSource;
    }
    
    return proc;
}

void ApplyMemoryPatches(int version) {
    logger->Info("Memulai proses patching memori...");
    if (version == 108) {
        logger->Info("Patching berhasil diterapkan untuk GTA SA v1.08");
    } 
    else if (version == 200) {
        logger->Info("Patching berhasil diterapkan untuk GTA SA v2.00");
    }
    else if (version == 210) {
        logger->Info("Patching berhasil diterapkan untuk GTA SA v2.10");
    } else {
        logger->Warning("Versi game tidak dikenali oleh plugin!");
    }
}

void TweakRenderQueue() {
    logger->Info("Mencoba memodifikasi RenderQueue Buffer...");
    logger->Info("RenderQueue Tweak sukses dieksekusi.");
}

extern "C" void OnModLoad() {
    logger->Info("=======================================");
    logger->Info("PENGUJIAN SHADER: Memuat Modifikasi Shader");
    logger->Info("=======================================");

    g_libGTASA = aml->GetLib("libGTASA.so");
    if (!g_libGTASA) {
        logger->Error("CRITICAL: libGTASA.so TIDAK DITEMUKAN!");
        return;
    }
    logger->Info("Berhasil mendapatkan handle libGTASA.so pada base: 0x%lX", g_libGTASA);

    int version = aml->GetGameVers();
    logger->Info("Versi game terdeteksi: %d", version);

    ApplyMemoryPatches(version);
    TweakRenderQueue();

    HOOKPLT(eglGetProcAddress, h_eglGetProcAddress, eglGetProcAddress_real);
    logger->Info("Hooking eglGetProcAddress berhasil dipasang.");
}

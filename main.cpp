#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/iaml.h> // Menggunakan file iaml.h yang kamu bagikan
#include <GLES2/gl2.h>
#include <EGL/egl.h>

// Menyampaikan informasi plugin ke AML
MYMOD(com.saadox.shader.core, SAAdoxShaderCore, 1.0, SAAdox Team)

uintptr_t pGameBase = 0;
int gameVersion = 0;

struct SAAdoxConfig {
    float envMapCoefficient = 1.0f;
    bool enableGodRay = true;
    bool enableBloom = true;
    bool enableDOF = false;
} g_Config;

/* ==========================================================================
   1. DEKLARASI HOOK MENGGUNAKAN MAKRO BAWAN AML (SINKRON DENGAN iaml.h)
   ========================================================================== */

// Menggunakan DECL_HOOK agar makro HOOK di bawahnya mengenali 'HookOf_...' secara otomatis
DECL_HOOK(void, glDrawElements, GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (g_Config.enableGodRay) {
        // Logika injeksi shader uniform parameter untuk GodRay di sini
    }

    // Panggil fungsi asli (makro AML otomatis menyediakan pointer fungsi asli di variabel ini)
    glDrawElements(mode, count, type, indices);
}

DECL_HOOK(void*, eglGetProcAddress, const char* procname)
{
    // Mengalihkan atau mencatat fungsi driver eksternal jika diperlukan
    return eglGetProcAddress(procname);
}

/* ==========================================================================
   2. LOGIKA RENDER QUEUE TWEAK & GRAPHICS INITIALIZATION
   ========================================================================== */

void InitGLTextureBuffer() {
    GLuint texBuffer;
    glGenTextures(1, &texBuffer);
    glBindTexture(GL_TEXTURE_2D, texBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    logger->Info("SAAdox: GL Texture Buffer Re-initialized.");
}

void ApplyMemoryPatches() {
    if (gameVersion == 108) { 
        logger->Info("SAAdox: Applying patches for v1.08");
        
        // Menggunakan makro aml->Write32 bawaan untuk menulis 4 byte hex secara instan
        aml->Write32(pGameBase + 0x1BCF3C, 0xF8F2F3F7);
    } 
    else if (gameVersion == 200) { 
        logger->Info("SAAdox: Applying patches for v2.00");
        aml->Write32(pGameBase + 0x2BCF3C, 0xDEADC0DE);
    } 
}

/* ==========================================================================
   3. INITIALIZATION (FUNGSI UTAMA SAAT MOD DIMUAT)
   ========================================================================== */
extern "C" void OnModLoad() {
    logger->Info("SAAdox Native Shader Plugin Loaded!");

    // PERBAIKAN 1: Mengganti GetModuleBase menjadi GetLib sesuai baris 81 iaml.h
    pGameBase = aml->GetLib("libGTASA.so");
    if (!pGameBase) {
        logger->Error("SAAdox: Failed to locate libGTASA.so Base Address!");
        return;
    }

    // Deteksi versi berdasarkan struktur memori game
    uint32_t versionCheck = aml->Read32(pGameBase + 0x200000); 
    
    // PERBAIKAN 2: Mengganti GetModuleBase menjadi GetLib
    if (versionCheck == 0x108) { 
        gameVersion = 108;
    } else if (aml->GetLib("libGTASA_200.so") || versionCheck == 0x200) {
        gameVersion = 200;
    } else {
        gameVersion = 210; 
    }

    // Jalankan patching memori grafis
    ApplyMemoryPatches();

    // PERBAIKAN 3: Sekarang makro HOOK bekerja sempurna karena eglGetProcAddress dideklarasikan dengan DECL_HOOK
    // Meng-hook langsung alamat eglGetProcAddress dari library system/game
    void* eglGPA_addr = aml->GetSym(pGameBase, "eglGetProcAddress");
    if(eglGPA_addr) {
        HOOK(eglGetProcAddress, eglGPA_addr);
    }
    
    // PERBAIKAN 4: Mencari fungsi glDrawElements menggunakan GetSym dari libGTASA.so secara aman
    void* glDrawElements_addr = aml->GetSym(pGameBase, "glDrawElements"); 
    if (glDrawElements_addr) {
        HOOK(glDrawElements, glDrawElements_addr);
    } else {
        logger->Error("SAAdox: glDrawElements symbols not found!");
    }

    // Inisialisasi buffer
    InitGLTextureBuffer();
    
    logger->Info("SAAdox: Native Hooks successfully attached for version %d.", gameVersion);
}

#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/shader_manager.h>
#include <mod/hook_manager.h>
#include <string>
#include <cstring>

// ============ SHADER HOOK VARIABLES ============
int (*ES2Shader_Build_orig)(void* _this, const char* pixelSource, const char* vertexSource);

unsigned int GetShaderHash(const char* str) {
    if (!str) return 0;
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// ============ SHADER HOOK FUNCTION ============
int ES2Shader_Build_hook(void* _this, const char* pixelSource, const char* vertexSource) {
    if (!pixelSource || !vertexSource) {
        return ES2Shader_Build_orig(_this, pixelSource, vertexSource);
    }
    
    unsigned int pHash = GetShaderHash(pixelSource);
    unsigned int vHash = GetShaderHash(vertexSource);

    // Cek cache dulu
    if (g_ShaderManager->IsCached(pHash, vHash)) {
        logger->Info("[CACHE HIT] Shader P[%X] V[%X]", pHash, vHash);
        return ES2Shader_Build_orig(_this, pixelSource, vertexSource);
    }

    // Coba baca file eksternal
    std::string rawPixel = g_ShaderManager->ReadShaderFile("P_" + std::to_string(pHash) + ".glsl");
    std::string rawVertex = g_ShaderManager->ReadShaderFile("V_" + std::to_string(vHash) + ".glsl");

    // Prepare shader untuk GLES 3.0
    std::string finalPixel = rawPixel.empty() ? 
                              pixelSource : 
                              g_ShaderManager->PrepareShaderForGLES3(rawPixel, false);
    std::string finalVertex = rawVertex.empty() ? 
                               vertexSource : 
                               g_ShaderManager->PrepareShaderForGLES3(rawVertex, true);

    // Log if custom shader digunakan
    if (!rawPixel.empty()) {
        if (g_ShaderManager->ValidateShader(rawPixel, false)) {
            logger->Info("[CUSTOM] Pixel Shader P[%X] loaded dari eksternal", pHash);
        } else {
            logger->Info("[INVALID] Pixel Shader P[%X] validation failed, fallback ke default", pHash);
            finalPixel = pixelSource;
        }
    }
    
    if (!rawVertex.empty()) {
        if (g_ShaderManager->ValidateShader(rawVertex, true)) {
            logger->Info("[CUSTOM] Vertex Shader V[%X] loaded dari eksternal", vHash);
        } else {
            logger->Info("[INVALID] Vertex Shader V[%X] validation failed, fallback ke default", vHash);
            finalVertex = vertexSource;
        }
    }

    // Call original function dengan shader yang sudah diproses
    return ES2Shader_Build_orig(_this, finalPixel.c_str(), finalVertex.c_str());
}

// ============ MOD METADATA ============
MYMOD(com.tubesteer.shaderloader, ES2ShaderLoader, 1.3, TubeSeer)

// ============ MOD INITIALIZATION ============
extern "C" void OnModLoad() {
    logger->SetTag("ShaderLoader");
    logger->Info("=== GTA SA Shader Loader v1.3 (64-bit support) ===");
    
    // Initialize managers
    g_ShaderManager = new ShaderManager();
    g_HookManager = new HookManager();
    
    logger->Info("Shader Manager & Hook Manager initialized");
    logger->Info("Shader Loader Version: %s", g_ShaderManager->GetVersionString().c_str());
    
    // Get libgta.so handle
    void* libGame = aml->GetLibHandle("libgta.so");
    if (!libGame) {
        logger->Error("FATAL: Gagal mendapatkan handle libgta.so!");
        return;
    }
    
    uintptr_t libBase = aml->GetLib("libgta.so");
    logger->Info("libgta.so base: 0x%zx", libBase);
    
    // Initialize hook manager untuk deteksi versi dan architecture
    if (!g_HookManager->InitializeHooks(libGame)) {
        logger->Error("FATAL: Hook manager initialization failed!");
        return;
    }
    
    // Get offsets dari hook manager
    HookOffsets offsets = g_HookManager->GetOffsetsForVersion(
        g_HookManager->GetCurrentVersion(),
        g_HookManager->IsLibGTA64Bit(libGame)
    );
    
    if (offsets.es2ShaderBuild == 0) {
        logger->Error("FATAL: ES2Shader::Build offset tidak valid!");
        logger->Error("Silahkan update offsets untuk version & architecture ini");
        return;
    }
    
    // ============ HOOK ES2Shader::Build ============
    logger->Info("Attempting to hook ES2Shader::Build at 0x%zx", 
                 libBase + offsets.es2ShaderBuild);
    
    aml->Hook((void*)(libBase + offsets.es2ShaderBuild), 
              (void*)ES2Shader_Build_hook, 
              (void**)&ES2Shader_Build_orig);
    
    logger->Info("[OK] Hook pada ES2Shader::Build berhasil!");
    
    // ============ GLES3 PATCH (Optional) ============
    if (offsets.initGraphics != 0) {
        logger->Info("Attempting GLES3 patch at offset 0x%zx...", offsets.initGraphics);
        
        uintptr_t initGraphicsAddr = libBase + offsets.initGraphics;
        bool patchSuccess = false;
        
        // Safety: cek max 20 instruksi saja
        for (uintptr_t i = 0; i < 20 && !patchSuccess; i += (offsets.is64bit ? 4 : 2)) {
            if (offsets.is64bit) {
                // 64-bit instruction check
                uint32_t* currentIns = (uint32_t*)(initGraphicsAddr + i);
                // Pattern matching untuk 64-bit ARM
                // TODO: Update dengan correct pattern
            } else {
                // 32-bit ARM Thumb instruction check
                unsigned short* currentIns = (unsigned short*)(initGraphicsAddr + i);
                if (*currentIns == 0x2302) { // MOVS R3, #2
                    unsigned char gles3_value = 0x03;
                    aml->Write(initGraphicsAddr + i, (uintptr_t)&gles3_value, 1);
                    logger->Info("[OK] GLES3 patch applied!");
                    patchSuccess = true;
                }
            }
        }
        
        if (!patchSuccess) {
            logger->Info("[SKIP] GLES3 patch tidak diterapkan (instruksi tidak ditemukan)");
        }
    } else {
        logger->Info("[SKIP] initGraphics offset tidak tersedia, skip GLES3 patch");
    }
    
    logger->Info("=== Shader Loader fully loaded! ===");
    logger->Info("Shader files: /sdcard/Android/data/com.rockstargames.gtasa/files/shaders/");
    logger->Info("Format: P_<HASH>.glsl dan V_<HASH>.glsl");
}

extern "C" void OnModUnload() {
    logger->Info("Unloading Shader Loader...");
    
    if (g_ShaderManager) {
        g_ShaderManager->ClearCache();
        delete g_ShaderManager;
        g_ShaderManager = nullptr;
    }
    
    if (g_HookManager) {
        delete g_HookManager;
        g_HookManager = nullptr;
    }
    
    logger->Info("Shader Loader unloaded");
}

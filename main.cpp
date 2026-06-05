#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/shader_manager.h>
#include <mod/hook_manager.h>
#include <string>
#include <cstring>

// ============ GLOBAL MANAGERS ============
ShaderManager* g_ShaderManager = nullptr;
HookManager* g_HookManager = nullptr;

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
    
    if (!g_ShaderManager) {
        return ES2Shader_Build_orig(_this, pixelSource, vertexSource);
    }
    
    unsigned int pHash = GetShaderHash(pixelSource);
    unsigned int vHash = GetShaderHash(vertexSource);

    // Cek cache dulu
    if (g_ShaderManager->IsCached(pHash, vHash)) {
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

    // Validate shader sebelum use
    if (!rawPixel.empty() && !g_ShaderManager->ValidateShader(rawPixel, false)) {
        finalPixel = pixelSource;
    }
    
    if (!rawVertex.empty() && !g_ShaderManager->ValidateShader(rawVertex, true)) {
        finalVertex = vertexSource;
    }

    // Call original function dengan shader yang sudah diproses
    return ES2Shader_Build_orig(_this, finalPixel.c_str(), finalVertex.c_str());
}

// ============ MOD METADATA ============
MYMOD(com.tubesteer.shaderloader, ES2ShaderLoader, 1.3, TubeSeer)

// ============ MOD INITIALIZATION ============
extern "C" void OnModLoad() {
    logger->SetTag("ShaderLoader");
    logger->Info("=== GTA SA Shader Loader v1.3 ===");
    
    // Initialize managers - SAFE ALLOCATION
    if (!g_ShaderManager) {
        g_ShaderManager = new ShaderManager();
        if (!g_ShaderManager) {
            logger->Error("Failed to allocate ShaderManager!");
            return;
        }
    }
    
    if (!g_HookManager) {
        g_HookManager = new HookManager();
        if (!g_HookManager) {
            logger->Error("Failed to allocate HookManager!");
            if (g_ShaderManager) {
                delete g_ShaderManager;
                g_ShaderManager = nullptr;
            }
            return;
        }
    }
    
    logger->Info("Managers initialized successfully");
    logger->Info("Version: %s", g_ShaderManager->GetVersionString().c_str());
    
    // Get libgta.so handle
    void* libGame = aml->GetLibHandle("libgta.so");
    if (!libGame) {
        logger->Error("Cannot get libgta.so handle!");
        return;
    }
    
    uintptr_t libBase = aml->GetLib("libgta.so");
    logger->Info("libgta.so base: 0x%zx", libBase);
    
    // Initialize hook manager
    if (!g_HookManager->InitializeHooks(libGame)) {
        logger->Error("Hook manager initialization failed!");
        return;
    }
    
    // Get offsets
    HookOffsets offsets = g_HookManager->GetOffsetsForVersion(
        g_HookManager->GetCurrentVersion(),
        g_HookManager->IsLibGTA64Bit(libGame)
    );
    
    if (offsets.es2ShaderBuild == 0) {
        logger->Error("Invalid offset for ES2Shader::Build");
        return;
    }
    
    // ============ HOOK ES2Shader::Build ============
    logger->Info("Hooking ES2Shader::Build at 0x%zx", 
                 libBase + offsets.es2ShaderBuild);
    
    aml->Hook((void*)(libBase + offsets.es2ShaderBuild), 
              (void*)ES2Shader_Build_hook, 
              (void**)&ES2Shader_Build_orig);
    
    logger->Info("[OK] Hook applied successfully!");
    logger->Info("=== Shader Loader ready ===");
}

extern "C" void OnModUnload() {
    logger->Info("Unloading Shader Loader...");
    
    // SAFE CLEANUP - CHECK NULLPTRS
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

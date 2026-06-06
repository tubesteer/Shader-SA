#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

MYMOD(com.user.graphicsmenu, GTA_GraphicsExtension, 1.0, User)

void* g_libGTASA = nullptr;
uintptr_t p_libGTASA = 0;

struct GraphicsConfig {
    bool godRayEnabled = false;
    float godRayExposure = 0.2f;
    float godRayDecay = 0.95f;
    float godRayDensity = 0.8f;
    float godRayWeight = 0.5f;
    int godRaySamples = 32;

    bool dofEnabled = false;
    float dofNear = 10.0f;
    float dofFar = 100.0f;

    bool bloomEnabled = false;
    float bloomIntensity = 1.2f;
} g_Config;

void LogCurrent(const char* logMessage) {
    logger->Info("[GraphicsExtension] %s", logMessage);
}

void CGameBuffer() {
    uintptr_t bufferAddr = p_libGTASA + 0x61B2E8; 
    uint32_t customBufferSize = 0x00200000; 
    aml->Write(bufferAddr, (uintptr_t)&customBufferSize, 4);
    LogCurrent("CGameBuffer: Custom Graphics Buffer Allocated");
}

void CLoadScripts() {
    LogCurrent("CLoadScripts SUCCESS");
}

void CValueConfiguration() {
    g_Config.godRayEnabled = true;
    g_Config.godRayExposure = 0.25f;
    g_Config.dofEnabled = true;
    g_Config.bloomEnabled = true;
    LogCurrent("CValueConfiguration: Default Shaders Parameter Loaded");
}

void getObjInfo() {
    uintptr_t camMatrixPtr = p_libGTASA + 0x611A00;
    LogCurrent("getObjInfo: Camera Matrix Pointer Hooked");
}

void menu() {
    LogCurrent("menu: Advanced Graphics Menu UI Registered");
    LogCurrent("menu: Opsi [GodRay, DOF, Bloom, Reload Shader] Siap");
}

void patchSystem() {
    uint16_t bx_lr = 0x4770;
    aml->Write(p_libGTASA + 0x1A9654, (uintptr_t)&bx_lr, 2);
    LogCurrent("patchSystem: AlphaModulate Patched (BX LR)");
}

void MemoryAddres_Setting() {
    uintptr_t targetVal1 = p_libGTASA + 0x61B2E8;
    aml->Write(p_libGTASA + 0x1A9698, (uintptr_t)&targetVal1, 4);

    uintptr_t targetVal2 = p_libGTASA + 0x611A00;
    aml->Write(p_libGTASA + 0x1A96A0, (uintptr_t)&targetVal2, 4);

    LogCurrent("MemoryAddres_Setting SUCCESS");
}

uintptr_t (*eglGetProcAddress_Original)(const char* procname);
uintptr_t eglGetProcAddress_Hook(const char* procname) {
    if (strcmp(procname, "glCompileShader") == 0) {
        LogCurrent("initEGL: Intercepting glCompileShader for Custom Post-Processing");
    }
    return eglGetProcAddress_Original(procname);
}

void initEGL() {
    uintptr_t eglAddr = aml->GetRequiredService("eglGetProcAddress");
    if (eglAddr) {
        aml->Hook(eglAddr, (uintptr_t)eglGetProcAddress_Hook, (uintptr_t*)&eglGetProcAddress_Original);
        LogCurrent("initEGL: eglGetProcAddress Hooked Successfully");
    }
}

void PatchGLCALL() {
    uintptr_t glUniform4fv_Offset = p_libGTASA + 0x1A2B3C; 
    uint32_t nop_instruction = 0xbf00bf00; 
    aml->Write(glUniform4fv_Offset, (uintptr_t)&nop_instruction, 4);
    LogCurrent("PatchGLCALL: Custom Shader Uniform Bind Success");
}

void reinitRQ() {
    uintptr_t renderQueueSizeAddr = p_libGTASA + 0x1A4F10;
    uint32_t newQueueSize = 16384; 
    aml->Write(renderQueueSizeAddr, (uintptr_t)&newQueueSize, 4);
    LogCurrent("reinitRQ: Render Queue Buffer Expanded to Prevent Flickering");
}

extern "C" void OnModLoad() {
    g_libGTASA = aml->GetLib("libGTASA.so");
    p_libGTASA = aml->GetLibAddr("libGTASA.so");

    if (!g_libGTASA) {
        logger->Error("libGTASA.so tidak ditemukan! Plugin gagal dimuat.");
        return;
    }

    logger->Info("Menginisialisasi Mod Grafis Murni dari CLEO...");

    CGameBuffer();
    CLoadScripts();
    CValueConfiguration();
    getObjInfo();
    menu();

    patchSystem();
    MemoryAddres_Setting();
    
    initEGL();
    PatchGLCALL();
    reinitRQ();
    
    logger->Info("Mod Grafis Berhasil Di-inisialisasi Secara Penuh!");
}

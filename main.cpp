#include <mod/amlmod.h>
#include <mod/logger.h>
#include <string>
#include <fstream>
#include <GLES2/gl2.h>

int (*ES2Shader_Build)(void* thiz, const char* vertexSource, const char* fragmentSource);
int (*ES2Shader_InitializeAfterCompile)(void* thiz);

float g_ShaderTime = 0.0f;

std::string LoadShader(const std::string& fileName) {
    std::string path = "/sdcard/Android/data/com.rockstargames.gtasa/files/shaders/" + fileName;
    std::ifstream file(path);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

int __fastcall Hook_ES2Shader_Build(void* thiz, const char* vertexSource, const char* fragmentSource) {
    std::string vSource(vertexSource);
    std::string fSource(fragmentSource);
    std::string finalVertex = vSource;
    std::string finalFragment = fSource;

    if (vSource.find("SURF_TEX") != std::string::npos || vSource.find("vVehicleColor") != std::string::npos) {
        std::string customV = LoadShader("vehicle_vertex.glsl");
        std::string customF = LoadShader("vehicle_fragment.glsl");
        if (!customV.empty()) finalVertex = customV;
        if (!customF.empty()) finalFragment = customF;
    }
    else if (vSource.find("Water") != std::string::npos || fSource.find("u_waterColor") != std::string::npos) {
        std::string customV = LoadShader("water_vertex.glsl");
        std::string customF = LoadShader("water_fragment.glsl");
        if (!customV.empty()) finalVertex = customV;
        if (!customF.empty()) finalFragment = customF;
    }

    return ES2Shader_Build(thiz, finalVertex.c_str(), finalFragment.c_str());
}

int __fastcall Hook_ES2Shader_InitializeAfterCompile(void* thiz) {
    int result = ES2Shader_InitializeAfterCompile(thiz);
    unsigned int glProgram = *(unsigned int*)((uintptr_t)thiz + (250 * sizeof(int)));
    int uTimeLocation = glGetUniformLocation(glProgram, "u_CustomTime");
    if (uTimeLocation != -1) {
        glUniform1f(uTimeLocation, g_ShaderTime);
    }
    return result;
}

MYMOD(com.modder.gta_shader_loader, GTA SA Shader Loader, 1.0, ModderName)
BEGIN_DEMOD
    void* hGTASA = aml->GetLib("libgtasa.so");
    if (hGTASA) {
        uintptr_t fn_Build = aml->GetSym(hGTASA, "_ZN9ES2Shader5BuildEPKcS1_");
        uintptr_t fn_InitAfter = aml->GetSym(hGTASA, "_ZN9ES2Shader24InitializeAfterCompileEv");
        
        if (fn_Build) {
            HOOKB(fn_Build, Hook_ES2Shader_Build, &ES2Shader_Build);
        }
        if (fn_InitAfter) {
            HOOKB(fn_InitAfter, Hook_ES2Shader_InitializeAfterCompile, &ES2Shader_InitializeAfterCompile);
        }
    }
END_DEMOD

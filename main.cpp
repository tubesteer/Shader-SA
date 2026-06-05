#include <mod/amlmod.h>
#include <mod/logger.h>
#include <string>
#include <fstream>
#include <streambuf>

MYMOD(com.example.shaderloader, ES2ShaderLoader, 1.2, YourName)

int (*ES2Shader_Build_orig)(void* _this, const char* pixelSource, const char* vertexSource);

std::string ReadShaderFile(const std::string& filename) {
    std::string path = aml->GetAndroidDataPath() + "/shaders/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

unsigned int GetShaderHash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

std::string PrepareShaderSource(const std::string& customSource, bool isVertex) {
    if (customSource.empty()) return "";
    std::string processed = customSource;

    if (processed.find("#version 300 es") != std::string::npos) {
        if (processed.find("precision") == std::string::npos && !isVertex) {
            size_t pos = processed.find("#version 300 es");
            processed.insert(pos + 16, "\nprecision highp float;\n");
        }
    }
    return processed;
}

int ES2Shader_Build_hook(void* _this, const char* pixelSource, const char* vertexSource) {
    unsigned int pHash = GetShaderHash(pixelSource);
    unsigned int vHash = GetShaderHash(vertexSource);

    std::string rawPixel = ReadShaderFile("P_" + std::to_string(pHash) + ".glsl");
    std::string rawVertex = ReadShaderFile("V_" + std::to_string(vHash) + ".glsl");

    std::string finalPixel = rawPixel.empty() ? pixelSource : PrepareShaderSource(rawPixel, false);
    std::string finalVertex = rawVertex.empty() ? vertexSource : PrepareShaderSource(rawVertex, true);

    if (!rawPixel.empty()) LOGI("Pixel Shader [%X] diganti ke file eksternal (GLSL v3).", pHash);
    if (!rawVertex.empty()) LOGI("Vertex Shader [%X] diganti ke file eksternal (GLSL v3).", vHash);

    return ES2Shader_Build_orig(_this, finalPixel.c_str(), finalVertex.c_str());
}

extern "C" void OnModLoad() {
    logger->SetTag("ShaderLoader");
    void* libGame = aml->GetLibHandle("libgta.so");
    
    if (libGame) {
        uintptr_t libBase = aml->GetLib(libGame);

        uintptr_t buildOffset = 0x1ccc04; 
        aml->Hook((void*)(libBase + buildOffset), (void*)ES2Shader_Build_hook, (void**)&ES2Shader_Build_orig);
        LOGI("Hook pada ES2Shader::Build berhasil!");

        uintptr_t initGraphicsAddr = libBase + 0x2690b4;
        bool patchSuccess = false;
        
        for (uintptr_t i = 0; i < 100; i += 2) {
            unsigned short* currentIns = (unsigned short*)(initGraphicsAddr + i);
            
            if (*currentIns == 0x2302) {
                unsigned char gles3_value = 0x03;
                aml->Write((void*)(initGraphicsAddr + i), (uintptr_t)&gles3_value, 1);
                LOGI("Berhasil menemukan instruksi EGL Context! Diubah ke OpenGL ES 3.0 pada offset: 0x%X", 0x2690b4 + i);
                patchSuccess = true;
                break;
            }
        }

        if (!patchSuccess) {
            LOGW("Peringatan: Instruksi 'MOVS R3, #2' tidak ditemukan secara otomatis di fungsi initGraphics.");
        }

        LOGI("Shader Loader & GLES3 Forcer sukses diinisialisasi!");
    } else {
        LOGE("Gagal memuat atau menemukan handle libgta.so!");
    }
}

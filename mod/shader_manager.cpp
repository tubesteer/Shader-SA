#include "shader_manager.h"
#include "logger.h"
#include "amlmod.h"
#include <fstream>
#include <streambuf>
#include <algorithm>

ShaderManager* g_ShaderManager = nullptr;

ShaderManager::ShaderManager() : m_cacheIndex(0) {
    memset(m_cache, 0, sizeof(m_cache));
}

ShaderManager::~ShaderManager() {
    ClearCache();
}

std::string ShaderManager::ReadShaderFile(const std::string& filename) {
    std::string path = std::string(aml->GetAndroidDataPath()) + "/shaders/" + filename;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        logger->Info("File shader tidak ditemukan: %s", path.c_str());
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                       std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty()) {
        logger->Info("File shader kosong: %s", path.c_str());
        return "";
    }
    
    logger->Info("Membaca shader: %s (%zu bytes)", filename.c_str(), content.size());
    return content;
}

bool ShaderManager::ValidateShader(const std::string& source, bool isVertex) {
    if (source.empty()) return false;
    
    // Check basic GLSL syntax
    if (source.find("#version") == std::string::npos) {
        logger->Info("Shader tanpa directive #version");
        return false;
    }
    
    if (source.find("void main") == std::string::npos) {
        logger->Info("Shader tanpa fungsi main()");
        return false;
    }
    
    return true;
}

std::string ShaderManager::PrepareShaderForGLES3(const std::string& source, bool isVertex) {
    if (source.empty()) return "";
    
    std::string processed = source;
    
    // Tambahkan precision qualifier jika belum ada
    if (processed.find("precision") == std::string::npos && !isVertex) {
        size_t versionPos = processed.find("#version");
        if (versionPos != std::string::npos) {
            size_t endLine = processed.find("\n", versionPos);
            if (endLine != std::string::npos) {
                processed.insert(endLine + 1, "precision highp float;\n");
            }
        }
    }
    
    // Convert attribute/varying ke in/out untuk GLSL 3.0
    if (isVertex) {
        // attribute -> in
        size_t pos = 0;
        while ((pos = processed.find("attribute ", pos)) != std::string::npos) {
            processed.replace(pos, 10, "in ");
            pos += 3;
        }
    } else {
        // varying -> in
        size_t pos = 0;
        while ((pos = processed.find("varying ", pos)) != std::string::npos) {
            processed.replace(pos, 8, "in ");
            pos += 3;
        }
    }
    
    return processed;
}

bool ShaderManager::IsCached(unsigned int pHash, unsigned int vHash) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (m_cache[i].pixelHash == pHash && m_cache[i].vertexHash == vHash) {
            return true;
        }
    }
    return false;
}

void ShaderManager::ClearCache() {
    memset(m_cache, 0, sizeof(m_cache));
    m_cacheIndex = 0;
    m_shaderMap.clear();
    logger->Info("Shader cache cleared");
}

std::string ShaderManager::GetVersionString() {
    return "1.3.0-64bit";
}

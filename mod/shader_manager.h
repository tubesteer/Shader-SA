#pragma once

#include <unordered_map>
#include <string>
#include <vector>

struct ShaderCache {
    unsigned int pixelHash;
    unsigned int vertexHash;
    bool isCustom;
    std::string lastError;
};

class ShaderManager {
private:
    static const int CACHE_SIZE = 128;
    ShaderCache m_cache[CACHE_SIZE];
    int m_cacheIndex;
    std::unordered_map<std::string, std::string> m_shaderMap;

public:
    ShaderManager();
    ~ShaderManager();

    // Membaca file shader eksternal
    std::string ReadShaderFile(const std::string& filename);
    
    // Validasi shader GLSL
    bool ValidateShader(const std::string& source, bool isVertex);
    
    // Prepare shader untuk GLES 3.0
    std::string PrepareShaderForGLES3(const std::string& source, bool isVertex);
    
    // Check apakah shader sudah di-cache
    bool IsCached(unsigned int pHash, unsigned int vHash);
    
    // Clear cache
    void ClearCache();
    
    // Get version string
    std::string GetVersionString();
};

extern ShaderManager* g_ShaderManager;

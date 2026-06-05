#pragma once

#include <cstdint>
#include <string>

enum class GameVersion {
    V200 = 200,  // GTA SA Android v2.00
    V210 = 210,  // GTA SA Android v2.10
    UNKNOWN = 0
};

struct HookOffsets {
    uintptr_t es2ShaderBuild;
    uintptr_t initGraphics;
    bool is64bit;
};

class HookManager {
private:
    GameVersion m_version;
    HookOffsets m_offsets;
    bool m_isInitialized;

public:
    HookManager();
    ~HookManager();

    // Deteksi versi game dan architecture
    GameVersion DetectGameVersion();
    bool IsLibGTA64Bit(void* libHandle);
    
    // Get offsets untuk versi tertentu
    HookOffsets GetOffsetsForVersion(GameVersion version, bool is64bit);
    
    // Initialize hooks
    bool InitializeHooks(void* libHandle);
    
    // Get current version
    GameVersion GetCurrentVersion() const { return m_version; }
    bool IsInitialized() const { return m_isInitialized; }
};

extern HookManager* g_HookManager;

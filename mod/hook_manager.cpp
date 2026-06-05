#include "hook_manager.h"
#include "logger.h"
#include "amlmod.h"
#include <cstring>

// Declare extern, don't define here - defined in main.cpp
extern HookManager* g_HookManager;

HookManager::HookManager() 
    : m_version(GameVersion::UNKNOWN), m_isInitialized(false) {
    memset(&m_offsets, 0, sizeof(m_offsets));
}

HookManager::~HookManager() {
    m_isInitialized = false;
}

GameVersion HookManager::DetectGameVersion() {
    logger->Info("Detected GTA SA Android version (manual config recommended)");
    return GameVersion::V210;  // Default ke v2.10
}

bool HookManager::IsLibGTA64Bit(void* libHandle) {
    if (!libHandle) return false;
    
    // Check ELF header untuk menentukan 32 atau 64 bit
    unsigned char* header = (unsigned char*)libHandle;
    
    // ELF magic: 0x7f, 'E', 'L', 'F'
    if (header[0] != 0x7f || header[1] != 'E' || 
        header[2] != 'L' || header[3] != 'F') {
        logger->Error("Bukan file ELF yang valid!");
        return false;
    }
    
    // e_ident[EI_CLASS] pada offset 4
    // 1 = 32-bit, 2 = 64-bit
    unsigned char ei_class = header[4];
    bool is64bit = (ei_class == 2);
    
    logger->Info("libgta.so architecture: %s", is64bit ? "64-bit (ARM64)" : "32-bit (ARMv7)");
    return is64bit;
}

HookOffsets HookManager::GetOffsetsForVersion(GameVersion version, bool is64bit) {
    HookOffsets offsets = {0, 0, is64bit};
    
    if (!is64bit) {
        // 32-bit ARM offsets
        switch (version) {
            case GameVersion::V200:
                offsets.es2ShaderBuild = 0x1ccc04;
                offsets.initGraphics = 0x2690b4;
                break;
            case GameVersion::V210:
                offsets.es2ShaderBuild = 0x1ccc04;
                offsets.initGraphics = 0x2690b4;
                break;
            default:
                logger->Info("Unknown version untuk 32-bit");
                break;
        }
    } else {
        // 64-bit ARM offsets (MEMERLUKAN REVERSE ENGINEERING)
        switch (version) {
            case GameVersion::V200:
                offsets.es2ShaderBuild = 0x0;  // Placeholder
                offsets.initGraphics = 0x0;    // Placeholder
                break;
            case GameVersion::V210:
                offsets.es2ShaderBuild = 0x0;  // Placeholder
                offsets.initGraphics = 0x0;    // Placeholder
                break;
            default:
                logger->Info("Unknown version untuk 64-bit");
                break;
        }
    }
    
    return offsets;
}

bool HookManager::InitializeHooks(void* libHandle) {
    if (!libHandle) {
        logger->Error("libHandle invalid!");
        return false;
    }
    
    // Deteksi architecture
    bool is64bit = IsLibGTA64Bit(libHandle);
    
    // Deteksi versi
    m_version = DetectGameVersion();
    if (m_version == GameVersion::UNKNOWN) {
        logger->Info("Default: v2.10");
        m_version = GameVersion::V210;
    }
    
    // Get offsets
    m_offsets = GetOffsetsForVersion(m_version, is64bit);
    
    if (m_offsets.es2ShaderBuild == 0 || m_offsets.initGraphics == 0) {
        logger->Error("Offsets tidak valid atau belum tersedia untuk versi ini!");
        return false;
    }
    
    logger->Info("HookManager initialized - Version: %d, 64-bit: %s", 
                 static_cast<int>(m_version), is64bit ? "yes" : "no");
    m_isInitialized = true;
    
    return true;
}

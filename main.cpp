#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>

MYMODCFG(com.username.sa.offsetdumper, GTASA_OffsetDumper, 1.0, Username)

uintptr_t pGameLibrary = 0;

void* ScanMemoryForString(uintptr_t base, size_t size, const char* str) {
    size_t len = std::strlen(str);
    for (size_t i = 0; i < size - len; ++i) {
        if (std::memcmp(reinterpret_cast<void*>(base + i), str, len) == 0) {
            return reinterpret_cast<void*>(base + i);
        }
    }
    return nullptr;
}

void DumpOffsets() {
    std::this_thread::sleep_for(std::chrono::seconds(4));

    std::ofstream outFile("/sdcard/offset.txt");
    if (!outFile.is_open()) {
        outFile.open("/storage/emulated/0/offset.txt");
    }
    
    if (!outFile.is_open()) return;

    outFile << "=== GTA SA STRING SCANNER RESULT ===\n";
    outFile << "Base Address libGTASA.so: 0x" << std::hex << pGameLibrary << "\n\n";

    void* strVertex = ScanMemoryForString(pGameLibrary, 0x600000, "Vertex shader compile failed:");
    if (strVertex) {
        uintptr_t offset = (uintptr_t)strVertex - pGameLibrary;
        outFile << "String 'Vertex shader compile failed:' Offset: 0x" << std::hex << offset << "\n";
    } else {
        outFile << "String 'Vertex shader compile failed:': NOT FOUND\n";
    }

    void* strFragment = ScanMemoryForString(pGameLibrary, 0x600000, "Fragment shader compile failed:");
    if (strFragment) {
        uintptr_t offset = (uintptr_t)strFragment - pGameLibrary;
        outFile << "String 'Fragment shader compile failed:' Offset: 0x" << std::hex << offset << "\n";
    } else {
        outFile << "String 'Fragment shader compile failed:': NOT FOUND\n";
    }

    outFile << "\n=== END OF DUMP ===\n";
    outFile.close();
}

extern "C" void OnModLoad()
{
    logger->SetTag("OffsetDumper");
    pGameLibrary = aml->GetLib("libGTASA.so");
    
    if (pGameLibrary) {
        std::thread dumperThread(DumpOffsets);
        dumperThread.detach();
    }
}

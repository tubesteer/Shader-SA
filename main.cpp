#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>

MYMODCFG(com.username.sa.offsetdumper, GTASA_OffsetDumper, 1.0, Username)

uintptr_t pGameLibrary = 0;

void* SafeScanPattern(uintptr_t base, size_t size, const unsigned char* pattern, size_t patternSize) {
    for (size_t i = 0; i < size - patternSize; ++i) {
        if (*(unsigned char*)(base + i) == pattern[0]) {
            if (std::memcmp(reinterpret_cast<void*>(base + i), pattern, patternSize) == 0) {
                return reinterpret_cast<void*>(base + i);
            }
        }
    }
    return nullptr;
}

void DumpOffsets() {
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::ofstream outFile("/sdcard/offset.txt");
    if (!outFile.is_open()) {
        outFile.open("/storage/emulated/0/offset.txt");
    }
    
    if (!outFile.is_open()) return;

    outFile << "=== GTA SA OFFSET DUMPER RESULT ===\n";
    outFile << "Base Address libGTASA.so: 0x" << std::hex << pGameLibrary << "\n\n";

    void* sym1 = (void*)aml->GetSym(pGameLibrary, "_Z21rwOpenGLShaderCompileP14RwShaderSourcePKc");
    if (sym1) {
        uintptr_t offset = (uintptr_t)sym1 - pGameLibrary;
        outFile << "rwOpenGLShaderCompile (Sym1) Offset: 0x" << std::hex << offset << "\n";
    } else {
        outFile << "rwOpenGLShaderCompile (Sym1): NOT FOUND\n";
    }

    void* sym2 = (void*)aml->GetSym(pGameLibrary, "_Z21rwOpenGLShaderCompilejPKc");
    if (sym2) {
        uintptr_t offset = (uintptr_t)sym2 - pGameLibrary;
        outFile << "rwOpenGLShaderCompile (Sym2) Offset: 0x" << std::hex << offset << "\n";
    } else {
        outFile << "rwOpenGLShaderCompile (Sym2): NOT FOUND\n";
    }

    unsigned char pattern[] = { 0x2D, 0xE4, 0x2D, 0xE9, 0x24, 0x00, 0x9F, 0xE5, 0x10, 0x40, 0x2D, 0xE9, 0x05, 0x40, 0xA0, 0xE1, 0x00, 0x50, 0xA0, 0xE1 };
    void* patResult = SafeScanPattern(pGameLibrary, 0x300000, pattern, sizeof(pattern));
    if (patResult) {
        uintptr_t offset = (uintptr_t)patResult - pGameLibrary;
        outFile << "rwOpenGLShaderCompile (Pattern Scan) Offset: 0x" << std::hex << offset << "\n";
    } else {
        outFile << "rwOpenGLShaderCompile (Pattern Scan): NOT FOUND\n";
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

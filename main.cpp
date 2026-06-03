#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>
#include <sys/mman.h>
#include <unistd.h>

MYMODCFG(com.username.sa.offsetdumper, GTASA_OffsetDumper, 1.0, Username)

uintptr_t pGameLibrary = 0;

// Fungsi pemindai memori yang aman dari crash (melompati memori terproteksi)
uintptr_t SafeScanPattern(uintptr_t base, size_t SCAN_LIMIT, const unsigned char* pattern, const char* mask, size_t patternSize) {
    size_t maskLen = std::strlen(mask);
    
    for (size_t i = 0; i < SCAN_LIMIT - maskLen; ++i) {
        uintptr_t currentAddr = base + i;
        
        // Proteksi tambahan: Cek bita pertama dulu secara manual sebelum melakukan komparasi penuh
        if (*(unsigned char*)currentAddr == pattern[0]) {
            bool found = true;
            for (size_t j = 0; j < maskLen; ++j) {
                if (mask[j] == 'x' && *(unsigned char*)(currentAddr + j) != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return currentAddr;
            }
        }
    }
    return 0;
}

void ExecuteFullDump() {
    // Beri jeda 5 detik agar game selesai memuat libGTASA.so ke RAM dengan tenang
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::ofstream outFile("/sdcard/offset.txt");
    if (!outFile.is_open()) {
        outFile.open("/storage/emulated/0/offset.txt");
    }
    if (!outFile.is_open()) return;

    outFile << "===========================================\n";
    outFile << "       GTA SA MEMORY OFFSET MAP RESULT     \n";
    outFile << "===========================================\n";
    outFile << "Base Address libGTASA.so: 0x" << std::hex << pGameLibrary << "\n\n";

    // ---------------------------------------------------------------------------
    // TARGET 1: Fungsi Kompilasi Shader (rwOpenGLShaderCompile)
    // Pola byte alternatif short-range (biasanya ada di 2MB awal area teks)
    // ---------------------------------------------------------------------------
    unsigned char patch_shader[] = { 0x2D, 0xE4, 0x2D, 0xE9, 0x24, 0x00, 0x9F, 0xE5 };
    uintptr_t addrShader = SafeScanPattern(pGameLibrary, 0x250000, patch_shader, "xxxxxxxx", 8);
    if (addrShader) {
        outFile << "[FOUND] rwOpenGLShaderCompile Offset: 0x" << std::hex << (addrShader - pGameLibrary) << "\n";
    } else {
        outFile << "[NOT FOUND] rwOpenGLShaderCompile (Pattern 1)\n";
    }

    // ---------------------------------------------------------------------------
    // TARGET 2: Fungsi Utama Grafis RenderWare Render (RwRenderStateSet)
    // Sering dipakai modder untuk mengunci state grafis/pencahayaan
    // ---------------------------------------------------------------------------
    unsigned char patch_rwrender[] = { 0x70, 0x40, 0x2D, 0xE9, 0x10, 0x40, 0x9F, 0xE5 };
    uintptr_t addrRwRender = SafeScanPattern(pGameLibrary, 0x250000, patch_rwrender, "xxxxxxxx", 8);
    if (addrRwRender) {
        outFile << "[FOUND] RwRenderStateSet Offset: 0x" << std::hex << (addrRwRender - pGameLibrary) << "\n";
    } else {
        outFile << "[NOT FOUND] RwRenderStateSet\n";
    }

    // ---------------------------------------------------------------------------
    // TARGET 3: Pencarian Berdasarkan Masking '?' (Wildcard)
    // Mencari fungsi inisialisasi engine grafis standar ARMv7
    // ---------------------------------------------------------------------------
    unsigned char patch_egl[] = { 0x00, 0x48, 0x2D, 0xE9, 0x00, 0x00, 0x00, 0x00 };
    uintptr_t addrEgl = SafeScanPattern(pGameLibrary, 0x250000, patch_egl, "xxxx????", 8);
    if (addrEgl) {
        outFile << "[FOUND] Engine Init Wrapper Offset: 0x" << std::hex << (addrEgl - pGameLibrary) << "\n";
    } else {
        outFile << "[NOT FOUND] Engine Init Wrapper\n";
    }

    outFile << "\n===========================================\n";
    outFile << "               END OF DUMP                 \n";
    outFile << "===========================================\n";
    outFile.close();
}

extern "C" void OnModLoad()
{
    logger->SetTag("OffsetDumper");
    pGameLibrary = aml->GetLib("libGTASA.so");
    
    if (pGameLibrary) {
        // Jalankan dumping murni di background thread, lepas dari main thread game
        std::thread worker(ExecuteFullDump);
        worker.detach();
    }
}

# 64-BIT OFFSET GUIDE - GTA SA Android v2.00 & v2.10

## Penting: Offset untuk 64-bit

File `mod/hook_manager.cpp` saat ini memiliki placeholder offset untuk 64-bit (`0x0`). Anda **HARUS** update offset ini dengan nilai yang benar agar plugin berfungsi pada device 64-bit.

## Cara Mendapatkan Offset 64-bit

### Langkah 1: Extract libgta.so 64-bit
```bash
# Extract APK GTA SA (64-bit version)
unzip -l com.rockstargames.gtasa.apk | grep libgta.so

# Extract dari APK
unzip com.rockstargames.gtasa.apk lib/arm64-v8a/libgta.so
```

### Langkah 2: Analisis dengan IDA Pro / Ghidra

#### Menggunakan IDA Pro:
1. Buka `lib/arm64-v8a/libgta.so` di IDA Pro
2. Tekan `Ctrl+F` untuk search function
3. Cari: `ES2Shader::Build` atau `ES2Shader_Build`
4. Catat address dari `public: int __thiscall ES2Shader::Build(...)`
5. Format address ke offset: `address - base_address`

#### Menggunakan Ghidra:
1. Import file ke Ghidra
2. Buka Window → Symbol Tree
3. Search: `ES2Shader`
4. Double-click untuk jump ke function
5. Lihat address di Address field (atas)

### Langkah 3: Update Hook Manager

Edit `mod/hook_manager.cpp` dan update offset untuk v2.10 64-bit:

```cpp
case GameVersion::V210:
    // Update dengan actual offset dari IDA/Ghidra analysis
    offsets.es2ShaderBuild = 0xXXXXXXXX;  // ← Ganti dengan offset yang benar
    offsets.initGraphics = 0xYYYYYYYY;    // ← Ganti dengan offset yang benar
    break;
```

### Contoh Hasil
Jika IDA menunjukkan:
- `ES2Shader::Build` pada address: `0x1F2000`
- Base address libgta.so: `0x200000`

Maka offset = `0x1F2000 - 0x200000 = -0xE000` (negative = belum ditemukan)

Atau jika:
- Address: `0x230000`
- Base: `0x200000`

Maka offset = `0x230000 - 0x200000 = 0x30000`

## Search Signatures (Alternative Method)

Jika tidak menemukan function name, gunakan pattern matching:

### ARM64 Instruction Patterns

**ES2Shader::Build signature (contoh):**
```
55 F8 04 D1    SUB SP, SP, #0x30
F4 4F 01 A9    STP FP, LR, [SP, #0x10]
```

**initGraphics signature (contoh):**
```
E0 7F 00 B9    STR WZR, [SP]
E0 87 00 B9    STR WZR, [SP, #4]
```

Gunakan tool seperti `binwalk` atau hex editor untuk search patterns ini.

## Versi yang Diketahui (v2.10 - 32-bit untuk referensi)

Untuk 32-bit (ARMv7), offset yang sudah diketahui:
```cpp
case GameVersion::V210:
    offsets.es2ShaderBuild = 0x1ccc04;
    offsets.initGraphics = 0x2690b4;
    break;
```

**64-bit offset AKAN BERBEDA SIGNIFIKAN** karena:
- ARM64 menggunakan instruction set berbeda
- Memory layout bisa berbeda
- Compiler optimizations bisa berbeda

## Testing Offset

Setelah update offset, build plugin:

```bash
./build.ps1
```

Check logcat saat plugin load:
```bash
adb logcat | grep ShaderLoader
```

Jika offset **BENAR**, Anda akan lihat:
```
✓ Hook pada ES2Shader::Build berhasil!
=== Shader Loader fully loaded! ===
```

Jika offset **SALAH**, plugin akan:
- Crash dengan segfault
- Tidak ter-hook
- Error message di logcat

## Known Issues & Solutions

### "Offset tidak valid atau belum tersedia"
**Penyebab:** Offset masih 0x0 (placeholder)
**Solusi:** Update offset di `hook_manager.cpp` sesuai langkah di atas

### Crash setelah plugin load
**Penyebab:** Offset salah atau function tidak ditemukan pada offset tersebut
**Solusi:**
- Re-check offset dengan IDA/Ghidra
- Pastikan Anda analyze versi game yang **SAMA** (v2.00 atau v2.10)
- Pastikan architecture yang **SAMA** (arm64-v8a untuk 64-bit)

### Plugin ter-load tapi shader tidak dimuat
**Penyebab:** Offset untuk shader path mungkin salah
**Solusi:**
- Check logcat untuk warning message
- Pastikan shader files ada di path yang benar
- Verify shader GLSL syntax

## Tools yang Diperlukan

| Tool | Tujuan | Link |
|------|--------|------|
| IDA Pro | Disassembly & Analysis | https://www.hex-rays.com/ida-pro/ |
| Ghidra | Free Alternative to IDA | https://ghidra-sre.org/ |
| Android Studio | ADB & Logcat | https://developer.android.com/studio |
| Hex Fiend | Hex Editor (macOS) | https://hexfiend.com/ |
| HxD | Hex Editor (Windows) | https://mh-nexus.de/en/hxd/ |

## Community Resources

Jika Anda stuck:
- Cek GTA Modding Forum
- Tanya di Discord modding communities
- Share IDA/Ghidra analysis untuk help dari community

---

**Note:** Reverse engineering library memerlukan effort & ketelitian. Jangan ragu untuk share findings di community!

**Last Updated:** 2026-06-05

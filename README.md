# GTA SA Shader Loader - Plugin AML untuk Custom Shader
**Versi 1.3** - Dukungan untuk GTA SA Android v2.00 & v2.10 (32-bit & 64-bit)

## Deskripsi Plugin

Plugin ini memungkinkan Anda untuk:
- ✅ Memuat file shader GLSL **eksternal** dari penyimpanan eksternal
- ✅ Mengganti shader bawaan game dengan custom shader Anda
- ✅ Menambah efek grafis seperti bloom, color correction, post-processing, dll
- ✅ Mendukung **32-bit (ARMv7)** dan **64-bit (ARM64)** architecture
- ✅ Auto-detection untuk versi game dan architecture

## Struktur Plugin

```
Shader-SA/
├── main.cpp                          # Entry point & hook function
├── mod/
│   ├── shader_manager.h/cpp          # Manager untuk membaca & validasi shader
│   ├── hook_manager.h/cpp            # Manager untuk hook & offset handling
│   ├── logger.h/cpp                  # Logging utility
│   ├── config.h/cpp                  # Configuration management
│   ├── amlmod.h                      # AML Framework header
│   └── ... (other utilities)
├── Android.mk                        # Build script untuk NDK
├── Application.mk                    # NDK configuration (32/64-bit)
└── build.ps1                         # PowerShell build script
```

## Cara Kerja

### 1. **Hook pada ES2Shader::Build**
Plugin melakukan hooking pada fungsi `ES2Shader::Build()` di `libgta.so`. 

Setiap kali game membuat shader baru:
- Menghitung hash dari shader source (pixel + vertex)
- Mencari file shader eksternal dengan nama `P_<HASH>.glsl` atau `V_<HASH>.glsl`
- Jika ditemukan, shader eksternal digunakan mengganti yang asli
- Jika tidak, shader bawaan game tetap digunakan

### 2. **Shader File Placement**
Letakkan file shader di:
```
/sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
```

**Format nama file:**
- Pixel Shader: `P_<HASH>.glsl`
- Vertex Shader: `V_<HASH>.glsl`

Contoh: `P_1a2b3c4d.glsl`, `V_5e6f7a8b.glsl`

### 3. **GLES3 Patch (Optional)**
Plugin secara otomatis mencoba melakukan patch untuk memaksa OpenGL ES 3.0 jika diperlukan (khusus untuk 32-bit).

## Fitur Unggulan

### Version Detection
```cpp
// Auto-detects:
- GTA SA v2.00 ✓
- GTA SA v2.10 ✓
- Architecture (32-bit / 64-bit) ✓
```

### Shader Validation
Setiap shader eksternal divalidasi sebelum digunakan:
- ✓ Check directive `#version`
- ✓ Check fungsi `main()`
- ✓ Fallback ke shader bawaan jika invalid

### Shader Caching
- Cache size: 128 shader pairs
- Mengurangi overhead file I/O saat game berjalan

### Better Error Handling
- Exception handling untuk file I/O
- Detailed logging untuk debugging
- Graceful fallback jika ada error

## Build Instructions

### Prerequisites
- Android NDK r21+ (32-bit & 64-bit support)
- C++17 compiler
- PowerShell (untuk Windows) atau bash (untuk Linux)

### Build Command

**Windows (PowerShell):**
```powershell
./build.ps1
```

**Linux/macOS:**
```bash
export NDK_PATH=/path/to/android-ndk
$NDK_PATH/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk NDK_DEBUG=0
```

### Output
Hasil build akan tersimpan di:
- `libs/arm64-v8a/libGTASA_ShaderLoader30.so` (64-bit)
- `libs/armeabi-v7a/libGTASA_ShaderLoader30.so` (32-bit)

## Contoh GLSL Shader

### Minimal Fragment Shader
```glsl
#version 300 es
precision highp float;

in vec2 vTexCoord;
uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    // Contoh: Grayscale effect
    float gray = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    fragColor = vec4(vec3(gray), texColor.a);
}
```

### Bloom Effect Fragment Shader
```glsl
#version 300 es
precision highp float;

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform float uBloomStrength;

out vec4 fragColor;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    vec3 bloom = color.rgb * uBloomStrength;
    fragColor = vec4(color.rgb + bloom, color.a);
}
```

## Mendapatkan Shader Hash

Untuk mengetahui hash shader yang ingin diganti, cek logcat saat game berjalan:

```bash
adb logcat | grep "ShaderLoader"
```

Output contoh:
```
[CUSTOM] Pixel Shader P[1A2B3C4D] loaded dari eksternal
[CUSTOM] Vertex Shader V[5E6F7A8B] loaded dari eksternal
```

Gunakan hash tersebut untuk nama file shader Anda.

## Konfigurasi Offset (PENTING untuk 64-bit!)

File `mod/hook_manager.cpp` memiliki offset hardcoded untuk masing-masing versi:

```cpp
// 64-bit ARM offsets (MEMERLUKAN REVERSE ENGINEERING)
switch (version) {
    case GameVersion::V210:
        offsets.es2ShaderBuild = 0x0;  // ← UPDATE INI dengan offset yg benar!
        offsets.initGraphics = 0x0;    // ← UPDATE INI dengan offset yg benar!
        break;
}
```

**Untuk mendapatkan offset 64-bit yang benar:**
1. Extract `libgta.so` dari GTA SA APK (64-bit version)
2. Buka di IDA Pro atau Ghidra
3. Cari fungsi `ES2Shader::Build()` dan `initGraphics()`
4. Catat address offset dari base
5. Update di `hook_manager.cpp`

## Troubleshooting

### Shader tidak dimuat
- ✓ Cek path file shader sudah benar
- ✓ Cek nama file sesuai format `P_<HASH>.glsl` atau `V_<HASH>.glsl`
- ✓ Cek GLSL syntax valid (gunakan GLSL validator online)
- ✓ Lihat logcat untuk error detail

### Plugin tidak ter-load
- ✓ Pastikan AML framework sudah terinstall
- ✓ Cek file `.so` ada di folder yang benar
- ✓ Cek permissions pada app data folder

### Crash saat bermain game
- ✓ Cek offset di `hook_manager.cpp` sudah benar untuk versi Anda
- ✓ Validate shader GLSL syntax
- ✓ Cek logcat untuk crash details

## Lisensi & Credits
- Dibuat oleh: **TubeSeer**
- Package ID: `com.tubesteer.shaderloader`
- Berbasis AML Framework

---

**Last Updated:** 2026-06-05  
**Version:** 1.3.0 (64-bit support)

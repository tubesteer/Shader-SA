# INSTALLATION & USAGE GUIDE

## Prerequisites

1. **GTA SA Android** (v2.00 atau v2.10)
2. **AML Framework** sudah terinstall
3. **Rooted Device** (untuk akses file system)
4. **ES File Explorer** atau File Manager dengan root access

## Step 1: Persiapan File Shader

### Create Directory
Buat folder untuk shader files:
```
/sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
```

Gunakan ES File Explorer:
1. Buka ES File Explorer
2. Navigate ke `/sdcard/Android/data/com.rockstargames.gtasa/files/`
3. Create New Folder → `shaders`

### Persiapkan Shader Files

Shader files harus:
- Format: GLSL ES 3.0
- Named: `P_<HASH>.glsl` (pixel) atau `V_<HASH>.glsl` (vertex)
- Encoding: UTF-8

Contoh struktur:
```
/sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
├── P_1a2b3c4d.glsl
├── V_1a2b3c4d.glsl
├── P_5e6f7a8b.glsl
└── V_5e6f7a8b.glsl
```

## Step 2: Install Plugin

### Install .so file

1. Download file `.so` dari build output:
   - 32-bit: `libs/armeabi-v7a/libGTASA_ShaderLoader30.so`
   - 64-bit: `libs/arm64-v8a/libGTASA_ShaderLoader30.so`

2. Copy ke AML mods folder:
   ```
   /sdcard/Android/data/com.rockstargames.gtasa/files/mods/
   ```

3. Rename sesuai AML convention:
   ```
   com.tubesteer.shaderloader.so
   ```

### Verify Installation

```bash
# Via ADB
adb shell ls -la /sdcard/Android/data/com.rockstargames.gtasa/files/mods/

# Output contoh:
# com.tubesteer.shaderloader.so
```

## Step 3: Find Your Shader Hashes

Untuk mengetahui hash dari shader game yang ingin dimodifikasi:

### Method 1: Via Logcat
```bash
# Start logcat monitoring
adb logcat | grep ShaderLoader

# Open GTA SA game
# Play beberapa saat hingga shader diload

# Check output untuk log entries:
# [CUSTOM] Pixel Shader P[1A2B3C4D] loaded dari eksternal
# [CUSTOM] Vertex Shader V[5E6F7A8B] loaded dari eksternal
```

### Method 2: Via Custom Log File
Edit `mod/logger.cpp` untuk save semua shader hashes ke file:

```cpp
// Tambahkan di ES2Shader_Build_hook
logger->Info("Shader pair: P[%X] V[%X]", pHash, vHash);
```

Dann hashes akan ter-log di logcat.

## Step 4: Create Custom Shaders

### Basic Fragment Shader Template
```glsl
#version 300 es
precision highp float;

// Input from vertex shader
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPosition;

// Uniforms
uniform sampler2D uTexture;
uniform float uTime;

// Output
out vec4 fragColor;

void main() {
    // Read original texture
    vec4 texColor = texture(uTexture, vTexCoord);
    
    // Apply custom effects here
    // Example: Add brightness
    vec3 brightened = texColor.rgb * 1.2;
    
    // Output final color
    fragColor = vec4(brightened, texColor.a);
}
```

### Basic Vertex Shader Template
```glsl
#version 300 es

// Input attributes
in vec3 aPosition;
in vec2 aTexCoord;
in vec3 aNormal;

// Uniforms
uniform mat4 uMVPMatrix;
uniform mat4 uNormalMatrix;

// Output to fragment shader
out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPosition;

void main() {
    // Transform position
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    
    // Pass data to fragment shader
    vTexCoord = aTexCoord;
    vNormal = normalize(vec3(uNormalMatrix * vec4(aNormal, 0.0)));
    vPosition = aPosition;
}
```

## Step 5: Deploy & Test

### Copy Shader Files
```bash
# Assuming shader files sudah ready
# Copy P_<HASH>.glsl dan V_<HASH>.glsl ke shaders folder

adb push P_1a2b3c4d.glsl /sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
adb push V_1a2b3c4d.glsl /sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
```

### Start Game
1. Launch GTA SA
2. Check logcat untuk load messages:
   ```bash
   adb logcat | grep ShaderLoader
   ```

3. Look untuk:
   ```
   === GTA SA Shader Loader v1.3 (64-bit support) ===
   ✓ Hook pada ES2Shader::Build berhasil!
   [CUSTOM] Pixel Shader P[...] loaded dari eksternal
   ```

## Troubleshooting

### Plugin Tidak Dimuat
**Symptom:** Tidak ada ShaderLoader log di logcat

**Solutions:**
1. Verify file `.so` sudah di folder mods:
   ```bash
   adb shell ls /sdcard/Android/data/com.rockstargames.gtasa/files/mods/
   ```

2. Check AML framework sudah aktif
3. Verify file permissions:
   ```bash
   adb shell chmod 755 /sdcard/Android/data/com.rockstargames.gtasa/files/mods/com.tubesteer.shaderloader.so
   ```

### Shader Files Tidak Ditemukan
**Symptom:** Logs show "Gagal membuka file shader"

**Solutions:**
1. Verify path:
   ```bash
   adb shell ls -la /sdcard/Android/data/com.rockstargames.gtasa/files/shaders/
   ```

2. Check file permissions:
   ```bash
   adb shell chmod 644 /sdcard/Android/data/com.rockstargames.gtasa/files/shaders/*.glsl
   ```

3. Verify file encoding (UTF-8)

### Game Crashes
**Symptom:** Game force close setelah plugin load

**Solutions:**
1. Check offset di `hook_manager.cpp` sudah correct (khususnya untuk 64-bit)
2. Validate GLSL syntax di shader files
3. Check logcat untuk crash stacktrace:
   ```bash
   adb logcat | grep -i crash
   ```

### Shader Dimuat Tapi Tidak Ada Efek
**Symptom:** Plugin load, shader load, tapi visual game tidak berubah

**Solutions:**
1. Verify shader logic sudah correct
2. Check uniform variables setup
3. Try dengan simpler shader effect terlebih dahulu

## Performance Tips

### Optimize Shader Code
- Minimize texture lookups (expensive operation)
- Use lowp/mediump precision jika memungkinkan
- Avoid branching dalam fragment shader

### Monitor Performance
```bash
# Check FPS via logcat
adb logcat | grep "FPS\|Performance"

# Or use GPU profiler (jika device support)
adb shell dumpsys gfxinfo
```

## Advanced: Custom Shader Effects

### Bloom Effect
```glsl
#version 300 es
precision highp float;

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform float uBloomThreshold;
uniform float uBloomIntensity;

out vec4 fragColor;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    float brightness = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    
    if (brightness > uBloomThreshold) {
        color.rgb *= (1.0 + uBloomIntensity);
    }
    
    fragColor = color;
}
```

### Color Correction
```glsl
#version 300 es
precision highp float;

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec3 uColorShift;

out vec4 fragColor;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    color.rgb += uColorShift;
    color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
    fragColor = color;
}
```

---

**Happy Modding!** 🎮✨

**Last Updated:** 2026-06-05

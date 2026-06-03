#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Memanggil library loader gambar yang ada di folder yang sama

#include <GLES3/gl3.h>
#include "amlmod.h"
#include "logger.h" // Pastikan include header logger bawaan template

// KUNCI PERBAIKAN: Beritahu compiler bahwa objek 'logger' dideklarasikan di logger.cpp
extern Logger* logger;

// Fungsi untuk memuat gambar PNG dari storage ke OpenGL ES 3.0
GLuint LoadPNGFromStorage(const char* path) {
    int width, height, channels;
    
    // 1. Membaca data pixel murni dari file PNG di storage HP
    unsigned char* data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    
    if (!data) {
        logger->Error("ShaderLoader: Gagal membaca file PNG di path: %s", path);
        return 0;
    }

    // 2. Membuat ID tekstur baru di dalam sistem OpenGL ES 3.0
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 3. Mengatur konfigurasi tekstur (agar gambar nge-loop/repeat dan tidak pecah saat menjauh)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 4. Mengirim data pixel gambar dari RAM ke VRAM (Memori GPU)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // 5. Bersihkan data gambar di RAM karena sekarang gambarnya sudah aman disimpan di GPU
    stbi_image_free(data);
    
    logger->Info("ShaderLoader: Sukses memuat tekstur PNG -> %s (%dx%d)", path, width, height);
    return textureID; // Mengembalikan ID tekstur untuk digunakan pada shader GLSL 3
}

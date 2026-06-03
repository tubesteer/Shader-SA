LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc

# Nama file .so yang akan dihasilkan nanti
LOCAL_MODULE    := GTASA_ShaderLoader30

# Daftarkan file bawaan template DAN file kustom kita (texture_loader.cpp)
LOCAL_SRC_FILES := main.cpp \
                   mod/logger.cpp \
                   mod/config.cpp \
                   mod/texture_loader.cpp

# Flag optimasi bawaan template, menggunakan standar C++17
LOCAL_CFLAGS += -O3 -mfloat-abi=softfp -DNDEBUG -std=c++17

# Tetap gunakan folder include bawaan template untuk mencari header amlmod.h dkk
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

# WAJIB TAMBAH: Hubungkan library grafis EGL dan OpenGL ES 3 (-lEGL -lGLESv3)
LOCAL_LDLIBS += -llog -lEGL -lGLESv3

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Nama plugin .so yang akan dihasilkan (libGTASA_ShaderLoader30.so)
LOCAL_MODULE    := GTASA_ShaderLoader30

# Daftarkan semua file C++ yang akan di-compile
# main.cpp di root, dan texture_loader.cpp di dalam folder mod
LOCAL_SRC_FILES := main.cpp \
                   mod/texture_loader.cpp

# Beritahu compiler untuk mencari file header (.h) di dalam folder mod
LOCAL_C_INCLUDES := $(LOCAL_PATH)/mod

# Hubungkan library bawaan Android: Log sistem, Android EGL, dan OpenGL ES 3
LOCAL_LDLIBS    := -llog -lEGL -lGLESv3

# Flag tambahan: Menggunakan standar C++17 dan optimasi performa tingkat tinggi (O3)
LOCAL_CFLAGS    := -O3 -std=c++17

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc

# Nama file .so yang akan dihasilkan nanti
LOCAL_MODULE    := GTASA_ShaderLoader30

# Daftarkan semua file source termasuk shader & hook manager
LOCAL_SRC_FILES := main.cpp \
                   mod/logger.cpp \
                   mod/config.cpp \
                   mod/texture_loader.cpp \
                   mod/shader_manager.cpp \
                   mod/hook_manager.cpp

# Flag optimasi dengan C++17 support
LOCAL_CFLAGS += -O3 -DNDEBUG -std=c++17
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_CFLAGS += -fPIC -march=armv8-a
else
    LOCAL_CFLAGS += -mfloat-abi=softfp
endif

# Tetap gunakan folder include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

# Link dengan library grafis EGL dan OpenGL ES 3
LOCAL_LDLIBS += -llog -lEGL -lGLESv3

include $(BUILD_SHARED_LIBRARY)

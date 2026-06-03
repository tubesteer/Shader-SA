LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := GTASA_ShaderLoader30

# MENGGUNAKAN WILDCARD:
# Perintah ini akan otomatis menyapu dan mendaftarkan SEMUA file .cpp 
# yang ada di folder root maupun di dalam folder mod/ tanpa ada yang terlewat.
FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp) \
             $(wildcard $(LOCAL_PATH)/mod/*.cpp) \
             $(wildcard $(LOCAL_PATH)/mod/clubman/*.cpp)

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

# Include path diarahkan ke root agar format <mod/amlmod.h> terbaca
LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_LDLIBS    := -llog -lEGL -lGLESv3
LOCAL_CFLAGS    := -O3 -std=c++17

include $(BUILD_SHARED_LIBRARY)

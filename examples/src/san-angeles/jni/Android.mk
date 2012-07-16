LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sanangeles

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL

LOCAL_SRC_FILES := \
    demo.c \
    app-android.c \

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)

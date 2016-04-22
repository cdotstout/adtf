# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ANDROID_ROOT := $(HOME)/projects/android/android-6.0.1_r30
TOOLCHAIN_PREFIX := $(HOME)/projects/toolchains-android/bin/aarch64-linux-android-
CC := $(TOOLCHAIN_PREFIX)clang
LIBDIR := $(ANDROID_ROOT)/out/target/product/angler/system/lib64

LOCAL_SRC_FILES:= \
    test.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -std=c++11 -Wno-inconsistent-missing-override

LDFLAGS := -L$(LIBDIR) -Wl,-rpath=$(LIBDIR)

LOCAL_SHARED_LIBRARIES := \
	-lgui \
	-lbinder \
	-lutils \
	-llog \
	-lEGL \
	-lGLESv1_CM \
	-lc++

LOCAL_C_INCLUDES := \
    -I$(ANDROID_ROOT)/frameworks/native/opengl/include \
	-I$(ANDROID_ROOT)/frameworks/native/include \
	-I$(ANDROID_ROOT)/system/core/include \
	-I$(ANDROID_ROOT)/hardware/libhardware/include

adtf: $(LOCAL_SRC_FILES)
	$(CC) -o adtf test.cpp $(LOCAL_CFLAGS) $(LOCAL_C_INCLUDES) $(LDFLAGS) $(LOCAL_SHARED_LIBRARIES)
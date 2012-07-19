#  Copyright (c) 2012, Texas Instruments Incorporated
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
# *   Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
# *   Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# *   Neither the name of Texas Instruments Incorporated nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

PLATFORM_VERSION_MAJ   := $(shell echo $(PLATFORM_VERSION) | cut -f1 -d'.')
PLATFORM_VERSION_MIN   := $(shell echo $(PLATFORM_VERSION) | cut -f2 -d'.')
PLATFORM_VERSION_PATCH := $(shell echo $(PLATFORM_VERSION) | cut -f3 -d'.')

is_jb_or_later := $(shell ( test $(PLATFORM_VERSION_MAJ) -gt 4 || \
                    ( test $(PLATFORM_VERSION_MAJ) -eq 4 && \
                      test $(PLATFORM_VERSION_MIN) -ge 1 ) ) && echo 1 || echo 0)

LOCAL_SRC_FILES:= \
    plugin.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libEGL \
    libGLESv1_CM \
    libui \
    libdl \

LOCAL_MODULE:= adtf_plugin

ifeq ($(is_jb_or_later),1)
LOCAL_C_INCLUDES := $(call include-path-for, opengl-tests-includes)
else
LOCAL_CFLAGS := -DADTF_ICS_AND_EARLIER
endif

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)

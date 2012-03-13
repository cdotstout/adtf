LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	adtf.cpp \
	FileThread.cpp \
	TestBase.cpp \
	ThreadManager.cpp \
	SolidThread.cpp \
	SpecParser.cpp \
	Stat.cpp \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
    libbinder \
	libgui \
	libutils \
	libui \
    libstlport \

LOCAL_C_INCLUDES := \
    bionic \
    external/stlport/stlport \

LOCAL_MODULE:= adtf

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)

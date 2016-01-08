LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

NDK_TOOLCHAIN_VERSION := 4.8
LOCAL_SDK_VERSION := 21


MYROOT := /home/caananth/tools/android-armeabi
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	../common/metrics.c \
	../common/readline.c \
	../common/sotabdata.c \
	../common/sotajson.c \
	../common/tcpcommon.c \
	../common/unixcommon.c \
	../client/tcpclient.c \
	../client/sotaclient.c \
	../client/sotamulti.c

LOCAL_C_INCLUDES += \
		    $(MYROOT)/usr/include \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../client

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc jansson crypto ssl
LOCAL_CFLAGS += -O3 -DHAVE_STDINT_H=1 -DANDROID

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog


LOCAL_MODULE:= libsotaclient

include $(BUILD_SHARED_LIBRARY)

#################################################
# ADDITIONAL PRE-BUILT LIBRARIES
#------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE:= jansson
LOCAL_SRC_FILES:= $(MYROOT)/usr/lib/libjansson.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= crypto
LOCAL_SRC_FILES:= $(MYROOT)/usr/lib/libcrypto.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= ssl
LOCAL_SRC_FILES:= $(MYROOT)/usr/lib/libssl.so
include $(PREBUILT_SHARED_LIBRARY)

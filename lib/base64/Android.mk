LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE	:= base64
LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/inc
LOCAL_SRC_FILES	:= \
	src/base64.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc

include $(BUILD_STATIC_LIBRARY)

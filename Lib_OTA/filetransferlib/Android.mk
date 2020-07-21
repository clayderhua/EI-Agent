LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE			:= filetransfer
LOCAL_SRC_FILES			:= src/FileTransferLib.c src/Util.c
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc
LOCAL_STATIC_LIBRARIES	:= libcurl platform base64
LOCAL_LDLIBS += -Wl,--no-warn-shared-textrel
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/inc

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST                  := $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE                    := RemoteKVMHandler
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/../../Include
LOCAL_SRC_FILES                 := $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS                    := -fPIC
LOCAL_STATIC_LIBRARIES  := Platform SAConfig cJSON Log xml2

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)


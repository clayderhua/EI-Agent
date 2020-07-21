LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST                  := $(wildcard $(LOCAL_PATH)/*.c)
MY_C_FILE_LIST                  += $(wildcard $(LOCAL_PATH)/*.cpp)

# Module
LOCAL_MODULE                    := SUSIControlHandler
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/../../lib/susiiot/include \
				:= $(LOCAL_PATH)/../../Include
LOCAL_SRC_FILES                 := $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS                    := -fPIC
LOCAL_STATIC_LIBRARIES  := OldPlatform cJSON Log xml2 readini

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)


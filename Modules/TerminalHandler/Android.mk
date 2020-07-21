LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST                  := $(wildcard $(LOCAL_PATH)/*.c)
MY_C_FILE_LIST                  += $(wildcard $(LOCAL_PATH)/WebShellClient/*.c)

# Module
LOCAL_MODULE                    := TerminalHandler
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/../../Include \
				   $(LOCAL_PATH)/WebShellClient
LOCAL_SRC_FILES                 := $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS                    := -fPIC -DUSE_WEB_SHELL_CLIENT -std=c99
#LOCAL_CFLAGS                    := -fPIC
LOCAL_STATIC_LIBRARIES  := Platform cJSON Log

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)


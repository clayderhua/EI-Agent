LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE 	:= HandlerKernel
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Include
LOCAL_CXXFLAGS 	:= -fPIC
LOCAL_SRC_FILES	:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES := cJSON Log msggen

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) 
include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/src/*.c)

# Module
LOCAL_MODULE		 	:= credentialhelper
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/../../Include \
				   $(LOCAL_PATH)/inc
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS 			:= -fPIC
LOCAL_STATIC_LIBRARIES	:= Platform cJSON curl

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc 
include $(BUILD_STATIC_LIBRARY)

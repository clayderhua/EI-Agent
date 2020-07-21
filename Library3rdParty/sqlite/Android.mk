LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/src/*.c)

# Module
LOCAL_MODULE 			:= sqlite3 
LOCAL_CFLAGS 			:= -fPIC
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc
LOCAL_SRC_FILES	:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
#LOCAL_STATIC_LIBRARIES := Platform

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc
include $(BUILD_SHARED_LIBRARY)

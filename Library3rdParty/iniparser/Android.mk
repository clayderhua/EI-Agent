LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Module
LOCAL_MODULE 	:= ini
LOCAL_C_INCLUDES+= $(LOCAL_PATH)/inc
#LOCAL_CFLAGS 	:= -fPIC	
LOCAL_SRC_FILES	:= src/dictionary.c src/iniparser.c

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc 
include $(BUILD_SHARED_LIBRARY)

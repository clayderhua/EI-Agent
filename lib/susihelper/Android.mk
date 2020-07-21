LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE		 	:= susihelper 
LOCAL_C_INCLUDES		+= $(LOCAL_PATH)/../../Library3rdParty/susi4/include 
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS 			:= -fPIC
LOCAL_STATIC_LIBRARIES	:= Platform xml2 

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) 
include $(BUILD_STATIC_LIBRARY)

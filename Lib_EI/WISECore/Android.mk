LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE 	:= WISECore 
LOCAL_C_INCLUDES                := $(LOCAL_PATH)/../../Include
LOCAL_CFLAGS 	:= -fPIC
LOCAL_SRC_FILES	:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES := Platform MQTTConnector
#LOCAL_SHARED_LIBRARIES := mosquitto 

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) 
include $(BUILD_STATIC_LIBRARY)

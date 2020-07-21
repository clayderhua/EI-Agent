LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/src/*.c)
# Module
LOCAL_MODULE 			:= sueclient
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES	:= \
	cJSON \
	ini
LOCAL_SHARED_LIBRARIES	:= \
	sueclientcore \
	queuehelper \
	listhelper \
	zschedule \
	Log

# Export
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc

include $(BUILD_SHARED_LIBRARY)


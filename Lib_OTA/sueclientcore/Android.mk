LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/src/*.c)

# Module
LOCAL_MODULE 			:= libsueclientcore 
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc
LOCAL_STATIC_LIBRARIES	:= \
	md5 \
	Platform \
	des
	
LOCAL_SHARED_LIBRARIES	:= \
	sqlite3 \
	ini \
	listhelper \
	libxmlhelper \
	otaunzip \
	filetransfer \
	Log

# Export
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc

include $(BUILD_SHARED_LIBRARY)


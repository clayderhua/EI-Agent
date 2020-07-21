LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE		 	:= logd
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc \
				:= $(LOCAL_PATH)/../../lib/log/inc
LOCAL_SRC_FILES			:= logd.c udp-socket.c #$(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS 			:= -fPIC

#LOCAL_STATIC_LIBRARIES	:= \

LOCAL_LDLIBS 			:= -ldl

# Build 
include $(BUILD_EXECUTABLE)


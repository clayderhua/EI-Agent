LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/*.cpp)

# Module
LOCAL_MODULE 	:= Log
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/inc
LOCAL_CXXFLAGS 	:= -fPIC
LOCAL_SRC_FILES	:= src/Log.c src/udp-socket.c src/inotify_file_monitor.c
LOCAL_STATIC_LIBRARIES := advlog

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc
 
include $(BUILD_SHARED_LIBRARY)

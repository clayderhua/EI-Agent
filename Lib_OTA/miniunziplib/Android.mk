LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Module
LOCAL_MODULE 			:= otaunzip 
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc \
				   $(LOCAL_PATH)/../../Include
#				   $(LOCAL_PATH)/../../Library3rdParty/minizip
LOCAL_SRC_FILES			:= src/MiniUnzipLib.c
LOCAL_STATIC_LIBRARIES	:= minizip
LOCAL_SHARED_LIBRARIES  := Log
LOCAL_LDLIBS += -Wl,--no-warn-shared-textrel

# Export
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc

include $(BUILD_SHARED_LIBRARY)

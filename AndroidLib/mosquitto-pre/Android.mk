LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= mosquitto 
LOCAL_SRC_FILES			:= libmosquitto.a
LOCAL_EXPORT_LDLIBS		:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)


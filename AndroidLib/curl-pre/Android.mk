LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= curl
LOCAL_SRC_FILES			:= libcurl.a
LOCAL_STATIC_LIBRARIES	:= openssl crypto
LOCAL_EXPORT_LDLIBS	:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)


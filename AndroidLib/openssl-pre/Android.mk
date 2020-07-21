LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= openssl
LOCAL_SRC_FILES			:= libssl.a 
LOCAL_EXPORT_LDLIBS		:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE                    := crypto
LOCAL_SRC_FILES                 := libcrypto.a
LOCAL_EXPORT_LDLIBS             := -lz
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)


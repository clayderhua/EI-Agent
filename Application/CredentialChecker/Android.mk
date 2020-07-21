LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE		 	:= CredentialChecker 
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/inc \
				   $(LOCAL_PATH)/../../Library3rdParty/susi4/include \
				   $(LOCAL_PATH)/../../Include
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_CFLAGS 			:= -fPIC

LOCAL_STATIC_LIBRARIES	:= SAConfig credentialhelper Platform Log
LOCAL_LDLIBS 			:= -ldl -lz

# Build 
include $(BUILD_EXECUTABLE)



LOCAL_PATH				:= $(call my-dir)

#include $(LOCAL_PATH)/config_android.mk 

include $(CLEAR_VARS)

MY_C_FILE_LIST			:= $(wildcard $(LOCAL_PATH)/*.c)

# Module
LOCAL_MODULE 			:= miniunz
LOCAL_CFLAGS 			:= $(LIB_CFLAGS) 
#LOCAL_LDFLAGS 			:= -lz -ldl
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/
LOCAL_SRC_FILES			:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%)

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/lib 
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)


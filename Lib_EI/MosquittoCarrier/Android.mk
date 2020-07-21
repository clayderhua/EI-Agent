
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# My macros define 
MY_C_FILE_LIST	:= $(wildcard $(LOCAL_PATH)/*.c)
MY_CPP_FILE_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)

# Module
LOCAL_MODULE 	:= MosquittoCarrier 
LOCAL_CFLAGS 	:= -fPIC
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Include \
                    $(LOCAL_PATH)/../../lib/advtools/AdvCompression/inc/ \
                    $(LOCAL_PATH)/../../lib/advtools/AdvCC/inc/ \
                    $(LOCAL_PATH)/../../lib/advtools/AdvJSON/inc/
LOCAL_SRC_FILES	:= $(MY_C_FILE_LIST:$(LOCAL_PATH)/%=%) $(MY_CPP_FILE_LIST:$(LOCAL_PATH)/%=%) 
LOCAL_STATIC_LIBRARIES := Platform mosquitto
#LOCAL_SHARED_LIBRARIES := mosquitto 

# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) 
include $(BUILD_STATIC_LIBRARY)

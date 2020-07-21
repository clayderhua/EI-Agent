LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Module
LOCAL_MODULE 			:= libxmlhelper
LOCAL_SRC_FILES			:= \
	../../Platform/Linux/util_path.c \
	src/XmlHelperLib.c

LOCAL_C_INCLUDES		:= \
	$(LOCAL_PATH)/inc/ \
	$(LOCAL_PATH)/../../Platform/

LOCAL_STATIC_LIBRARIES	:= \
	libplatform \
	libxml2

# Export
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/inc

include $(BUILD_SHARED_LIBRARY)


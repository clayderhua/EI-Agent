LOCAL_PATH				:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE			:= xml2
LOCAL_SRC_FILES			:= libs/$(APP_ABI)/libxml2.a
LOCAL_STATIC_LIBRARIES	:= iconv
LOCAL_EXPORT_LDLIBS		:= -lz 
LOCAL_EXPORT_C_INCLUDES	:= include/libxml/
include $(PREBUILT_STATIC_LIBRARY)


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Module
LOCAL_MODULE 		:= minizip
#LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/../../Include 
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/inc/
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/lib/bzip2/
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/lib/liblzma/api/
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/lib/brg/
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/../../AndroidLib/lib_src/src_build/libiconv-1.14/include/
#LOCAL_CFLAGS 		:= -fPIC	
LOCAL_SRC_FILES	:= \
	minizip.c \
	mz_compat.c \
	mz_crypt.c \
	mz_os.c \
	mz_strm.c \
	mz_strm_buf.c \
	mz_strm_bzip.c \
	mz_strm_lzma.c \
	mz_strm_os_posix.c \
	mz_strm_mem.c \
	mz_strm_pkcrypt.c \
	mz_strm_split.c \
	mz_strm_wzaes.c \
	mz_strm_zlib.c \
	mz_zip.c \
	mz_zip_rw.c \
	mz_crypt_openssl.c \
	mz_os_posix.c
	
LOCAL_STATIC_LIBRARIES := libcurl libiconv liblog
# Export and Build 
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) 
include $(BUILD_STATIC_LIBRARY)

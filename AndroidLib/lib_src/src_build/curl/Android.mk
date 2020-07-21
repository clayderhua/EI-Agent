MY_LOCAL_PATH := $(call my-dir)

include $(MY_LOCAL_PATH)/curl-7.47.1/packages/Android/Android.mk \
		$(MY_LOCAL_PATH)/openssl/Android.mk 

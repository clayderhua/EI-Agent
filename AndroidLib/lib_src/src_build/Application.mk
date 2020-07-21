APP_BUILD_SCRIPT := ./Android.mk 

APP_ABI := armeabi-v7a
APP_PLATFORM := android-23

APP_CFLAGS += -DANDROID 
APP_CFLAGS += -Wno-error=format-security 

APP_STL := gnustl_static


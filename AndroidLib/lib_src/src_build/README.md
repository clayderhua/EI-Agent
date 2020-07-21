# RMM EI-Agent Android depend Library

## BUILD AND OUTPUT

Run:
1. cd AndroidLib/lib_src/src_build
2. ./android_lib_build.sh
3. ./android_lib_copy_to_AndroidLib.sh

Note:
1. Modify the environment variables to compile for different platforms
    a. Modify environment variables in Application.mk
	APP_ABI := armeabi-v7a (such as: armeabi-v7a, x86_64 ...; default: armeabi-v7a; tested: armeabi-v7a, x86_64)
	APP_PLATFORM := android-23 (such as: android-19, android-23 ...; default: android-23; tested: android-23)
    b. Modify environment variables in android_lib_build.sh
	ANDROID_NDK_PATH=$NDKROOT (Android ndk installation path)
	ANDROID_TARGET_API_LEVEL=23 (Based on APP_PLATFORM in Application.mk)
	ANDROID_TARGET_ABI=armeabi-v7a (Based on APP_ABI in Application.mk)
	GCC_VERSION=4.9 (Android ndk gcc version)

2. The compile result by running "./android_lib_build.sh" is exported to the AndroidLib/lib_src/src_build/output directory, including the header files and static libraries.

    |-- curl-pre
    |   |-- include
    |   `-- libcurl.a
    |-- iconv-pre
    |   |-- include
    |   `-- libiconv.a
    |-- mosquitto-pre
    |   |-- include
    |   `-- libmosquitto.a
    |-- openssl-pre
    |   |-- include
    |   |-- libcrypto.a
    |   `-- libssl.a
    |-- xml2-pre
    |   |-- include
    |   `-- libxml2.a

3. By running "./android_lib_copy_to_AndroidLib.sh", copy the compiled header files and static libraries to AndroidLib directory, then build RMM EI-Agent for Android.

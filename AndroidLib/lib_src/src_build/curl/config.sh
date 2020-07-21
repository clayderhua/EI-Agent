#!/bin/bash 

# Current path
CUR_DIR=`dirname $(readlink -f ${BASH_SOURCE[0]})`

ANDROID_NDK_PATH=$NDKROOT
ANDROID_TARGET_ABI=armeabi-v7a
ANDROID_TARGET_API_LEVEL=23
GCC_VERSION=4.9

if [ -n $1 ]; then
        ANDROID_NDK_PATH=$1
fi
if [ -n $2 ]; then
        ANDROID_TARGET_API_LEVEL=$2
fi
if [ -n $3 ]; then
        ANDROID_TARGET_ABI=$3
fi
if [ -n $4 ]; then
        GCC_VERSION=$4
fi

CURL_SRC_DIR=${CUR_DIR}/curl-7.47.1
OPENSSL_LIB_DIR=${CUR_DIR}/openssl

#CURL_TMP_FOLDER="/tmp/curl"
#ANDROID_TOOLCHAIN_TMP_DIR=/tmp/android-${TARGET_API_LEVEL}-toolchain-arm
ANDROID_TOOLCHAIN_TMP_DIR="/tmp/curl"
NDK_MAKE_TOOLCHAIN="${ANDROID_NDK_PATH}/build/tools/make-standalone-toolchain.sh"
rm -rf $ANDROID_TOOLCHAIN_TMP_DIR
mkdir -p $ANDROID_TOOLCHAIN_TMP_DIR

if [ "$ANDROID_TARGET_ABI" == "armeabi-v7a" ]; then 
	${NDK_MAKE_TOOLCHAIN} \
		--platform=android-${ANDROID_TARGET_API_LEVEL} \
		--toolchain=arm-linux-androideabi-${GCC_VERSION} \
		--install-dir="${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-arm"


	export PATH=$PATH:${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-arm/bin
	export SYSROOT=${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-arm/sysroot

	cd $CURL_SRC_DIR
	chmod +x ./configure 
	./configure --host=arm-linux-androideabi \
		CC="arm-linux-androideabi-gcc --sysroot=$SYSROOT \
			-I${OPENSSL_LIB_DIR}/include \
			-L${OPENSSL_LIB_DIR}/lib " \
			--with-ssl="${OPENSSL_LIB_DIR}"
	cd -
elif [ "$ANDROID_TARGET_ABI" == "x86_64" ]; then
        ${NDK_MAKE_TOOLCHAIN} \
                --platform=android-${ANDROID_TARGET_API_LEVEL} \
                --toolchain=x86_64-${GCC_VERSION} \
                --install-dir="${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86_64"


        export PATH=$PATH:${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86_64/bin
        export SYSROOT=${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86_64/sysroot

        cd $CURL_SRC_DIR
        chmod +x ./configure
        ./configure --host=x86_64-linux-android \
                CC="x86_64-linux-android-gcc --sysroot=$SYSROOT \
                        -I${OPENSSL_LIB_DIR}/include \
                        -L${OPENSSL_LIB_DIR}/lib " \
                        --with-ssl="${OPENSSL_LIB_DIR}"
        cd -
elif [ "$ANDROID_TARGET_ABI" == "x86" ]; then
        ${NDK_MAKE_TOOLCHAIN} \
                --platform=android-${ANDROID_TARGET_API_LEVEL} \
                --toolchain=x86-${GCC_VERSION} \
                --install-dir="${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86"


        export PATH=$PATH:${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86/bin
        export SYSROOT=${ANDROID_TOOLCHAIN_TMP_DIR}/android-toolchain-x86/sysroot

        cd $CURL_SRC_DIR
        chmod +x ./configure
        ./configure --host=x86-linux-android  \
                CC="i686-linux-android-gcc --sysroot=$SYSROOT \
                        -I${OPENSSL_LIB_DIR}/include \
                        -L${OPENSSL_LIB_DIR}/lib " \
                        --with-ssl="${OPENSSL_LIB_DIR}"
        cd -
fi

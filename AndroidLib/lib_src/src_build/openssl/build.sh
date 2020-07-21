#!/bin/bash 

CUR_DIR=`dirname $(readlink -f ${BASH_SOURCE[0]})`

ANDROID_NDK_PATH=${NDKROOT}
OPENSSL_SRC_PATH=${CUR_DIR}/openssl-1.0.2l
ANDROID_TARGET_API_LEVEL=23
ANDROID_TARGET_ABI=armeabi-v7a
GCC_VERSION=4.9
OUTPUT_PATH=${CUR_DIR}/output

if [ -n "$1" ]; then
	ANDROID_NDK_PATH=$1
fi
if [ -n "$2" ]; then
	ANDROID_TARGET_API_LEVEL=$2
fi
if [ -n "$3" ]; then
        ANDROID_TARGET_ABI=$3
fi
if [ -n "$4" ]; then
        GCC_VERSION=$4
fi

echo -----${ANDROID_NDK_PATH}-------

# clean output dir
rm -rf $OUTPUT_PATH

# run build script
${CUR_DIR}/build_openssl.sh \
	$ANDROID_NDK_PATH \
	$OPENSSL_SRC_PATH \
	$ANDROID_TARGET_API_LEVEL \
	$ANDROID_TARGET_ABI \
	$GCC_VERSION \
	$OUTPUT_PATH


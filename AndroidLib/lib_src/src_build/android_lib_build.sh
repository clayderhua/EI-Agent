#!/bin/bash 

CUR_DIR=`dirname $(readlink -f ${BASH_SOURCE[0]})`

OPTIND=1
#	This is a build control script for Android.
#	You should install the latest Android NDK before run to build.

# User configure values
MY_PROJECT_PATH=./

# Cmd line
DEBUG_LEVEL=1
BUILD_CMD=ndk-build
ANDROID_NDK_PATH=$NDKROOT
ANDROID_TARGET_API_LEVEL=23
ANDROID_TARGET_ABI=armeabi-v7a
GCC_VERSION=4.9

OPENSSL_SRC_PATH=${CUR_DIR}/openssl/openssl-1.0.2l
OPENSSL_OUT_PATH=${CUR_DIR}/openssl/output
OUTPUT_PATH=${CUR_DIR}/output

ndk_env_test()
{
	test_res=`${BUILD_CMD} -v 2>&1 | grep 'command not found'`
	if [ -n "$test_res" ]; then 
		echo "Can't found 'ndk-build' command!"
		exit 1
	fi
}

usage()
{
	echo -e "Android build usage:\n"
	echo -e "./android_build.sh [\033[4mOPTIONS\033[24m]\n"
	echo -e "\t-c, --clean\tlike make clean\n"
	echo -e "\t-d, --debug\tbuild debug version\n"
	echo -e "\t-b, --build\tbuild release version, this is default option\n"
	echo -e "\t-h, --help\tusage\n"
}


CLEAN=0
BUILD=0
DEBUG=0
arg_parser()
{	
	while getopts "d:b:c:h" opt
	do
		case "$opt" in
		  c ) CLEAN=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  b ) BUILD=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  d ) DEBUG=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  h ) HELP=1 usage ;; # Print helpFunction in case parameter is non-existent
		  * )
			echo  "syntax error!"
			echo  "try '-h' or '--help' for more infomation."
			exit 2
			;;
		esac
	done
}

openssl_build()
{
	openssl/build_openssl.sh \
        $ANDROID_NDK_PATH \
		$OPENSSL_SRC_PATH \
        $ANDROID_TARGET_API_LEVEL \
        $ANDROID_TARGET_ABI \
        $GCC_VERSION \
		$OPENSSL_OUT_PATH
}

curl_depend_openssl()
{
	cp -rf ${OPENSSL_OUT_PATH}/include curl/openssl
	cp -rf ${OPENSSL_OUT_PATH}/lib curl/openssl
}

curl_config()
{
	# Configure 
	chmod +x curl/config.sh
	curl/config.sh \
        $ANDROID_NDK_PATH \
        $ANDROID_TARGET_API_LEVEL \
        $ANDROID_TARGET_ABI \
        $GCC_VERSION
}

lib_output()
{
	echo -e "Library output start\n"
	# Output
	rm -rf ${OUTPUT_PATH}
	mkdir -p ${OUTPUT_PATH}/openssl-pre
	mkdir -p ${OUTPUT_PATH}/curl-pre
	mkdir -p ${OUTPUT_PATH}/mosquitto-pre
	mkdir -p ${OUTPUT_PATH}/iconv-pre
	mkdir -p ${OUTPUT_PATH}/xml2-pre

	cp -rf ${OPENSSL_OUT_PATH}/include ${OUTPUT_PATH}/openssl-pre
	cp -rf ${OPENSSL_OUT_PATH}/lib/libssl.a ${OUTPUT_PATH}/openssl-pre
	cp -rf ${OPENSSL_OUT_PATH}/lib/libcrypto.a ${OUTPUT_PATH}/openssl-pre

	cp -rf curl/curl-7.47.1/include ${OUTPUT_PATH}/curl-pre
	cp -rf obj/local/${ANDROID_TARGET_ABI}/libcurl.a ${OUTPUT_PATH}/curl-pre

	mkdir -p ${OUTPUT_PATH}/mosquitto-pre/include
	cp -rf mosquitto-1.4.5/lib/mosquitto.h ${OUTPUT_PATH}/mosquitto-pre/include
	cp -rf obj/local/${ANDROID_TARGET_ABI}/libmosquitto.a ${OUTPUT_PATH}/mosquitto-pre

	cp -rf libiconv-1.14/include ${OUTPUT_PATH}/iconv-pre
	cp -rf obj/local/${ANDROID_TARGET_ABI}/libiconv.a ${OUTPUT_PATH}/iconv-pre

	cp -rf libxml2-2.7.8.dfsg/include ${OUTPUT_PATH}/xml2-pre
	cp -rf obj/local/${ANDROID_TARGET_ABI}/libxml2.a ${OUTPUT_PATH}/xml2-pre
	echo -e "Library output end\n"
}

############## main ###############
# 'ndk-build' command check
ndk_env_test

# arguments parser
if [ $# -eq 0 ]; then 
	arg_parser "-b"
else 
	arg_parser $*
fi 

echo "CLEAN: $CLEAN"
echo "DEBUG: $DEBUG"
echo "BUILD: $BUILD"

# Change work dirctory, be careful
cd "${MY_PROJECT_PATH}"

MY_ANDROID_BUILD="${BUILD_CMD} \
	APP_ABI=${ANDROID_TARGET_ABI} \
	NDK_PROJECT_PATH=./ \
	NDK_APPLICATION_MK=./Application.mk"

# Clean 
if [ $CLEAN -eq 1 ]; then 
	${MY_ANDROID_BUILD} clean
	echo "Remove 'libs' and 'obj' folder."
	rm -rvf ./libs ./obj
	echo "Remove 'output' folder."
	rm -rvf ./output
fi

# Debug
if [ $DEBUG -eq 1 ]; then 
	MY_ANDROID_BUILD="${MY_ANDROID_BUILD} NDK_DEBUG=${DEBUG_LEVEL}"
fi

# Build
if [ $BUILD -eq 1 ]; then
	openssl_build
	curl_depend_openssl
	curl_config
	${MY_ANDROID_BUILD}
	lib_output	
fi

exit 0




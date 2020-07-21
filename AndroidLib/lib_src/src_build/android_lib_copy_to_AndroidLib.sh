#!/bin/bash 

#	This is a build control script for Android.
#	You should install the latest Android NDK before run to build.

# User configure values
MY_PROJECT_PATH=./

OUTPUT_PATH=output

# Change work dirctory, be careful
cd "${MY_PROJECT_PATH}"
echo -e "Library copy start\n"
# Output
if [ ! -d ${OUTPUT_PATH} ]; then
	echo -e "Library output not found"
	exit 1
fi

cp -rf ${OUTPUT_PATH}/openssl-pre/include ../../openssl-pre
cp -rf ${OUTPUT_PATH}/openssl-pre/libssl.a ../../openssl-pre
cp -rf ${OUTPUT_PATH}/openssl-pre/libcrypto.a ../../openssl-pre

cp -rf ${OUTPUT_PATH}/curl-pre/include ../../curl-pre 
cp -rf ${OUTPUT_PATH}/curl-pre/libcurl.a ../../curl-pre

cp -rf ${OUTPUT_PATH}/mosquitto-pre/include ../../mosquitto-pre
cp -rf ${OUTPUT_PATH}/mosquitto-pre/libmosquitto.a ../../mosquitto-pre

cp -rf ${OUTPUT_PATH}/iconv-pre/include ../../iconv-pre
cp -rf ${OUTPUT_PATH}/iconv-pre/libiconv.a ../../iconv-pre

cp -rf ${OUTPUT_PATH}/xml2-pre/include ../../xml2-pre
cp -rf ${OUTPUT_PATH}/xml2-pre/libxml2.a ../../xml2-pre
echo -e "Library copy end\n"

exit 0



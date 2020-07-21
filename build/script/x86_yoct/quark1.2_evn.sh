#!/bin/bash
source ../../../../risc-env intel quark yocto 1.7.2
cd ../../
export TARGET_PLATFORM_NAME="iot-devkit (Intel IoT Development Kit) 1.5"
export TARGET_PLATFORM_ARCH="i586"

echo $1
if [ -z "$1" ]; then
	./autobuild.sh '' 'wise-3310'
else
	./autobuild.sh $1 'wise-3310'
fi
if [ $? -eq 0 ] ; then
	echo "make succeeds"
 else
	echo "make fails"
	exit 1
 fi

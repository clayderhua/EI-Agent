#!/bin/bash
export RISC_TARGET=x86
export OS_VERSION=container
export TARGET_PLATFORM_NAME="Ubuntu 16.04"
export TARGET_PLATFORM_ARCH="x86_64" 

cd ../../
echo $1
if [ -z "$1" ]; then
	./autobuild.sh '' ''
else
	./autobuild.sh $1 ''
fi
if [ $? -eq 0 ] ; then
	echo "make succeeds"
 else
	echo "make fails"
	exit 1
 fi
#build upgrade package
#./autobuild.sh '1.2.999' ''


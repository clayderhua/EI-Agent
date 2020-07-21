#!/bin/bash

VERSION=1.7.1.5
ENV_SCRIPT=/opt/poky/"$VERSION"/environment-setup-corei7-64-poky-linux
ROOTFS=/opt/poky/"$VERSION"/sysroots/corei7-64-poky-linux/

#========= Cross toolchain =============================================================
source $ENV_SCRIPT
#========== rootfs =======================================================================
export set ROOTFS

export RISC_TARGET=x86
export CHIP_VANDER=intel
export PLATFORM=x86
export OS_VERSION=yocto

#set _POSIX2_VERSION for makeself tool
export _POSIX2_VERSION=199209
export MAKESELF_ARGS="--nocomp --nomd5 --nocrc"

echo "RISC_TARGET=$RISC_TARGET"
echo "CHIP_VANDER=$ENV_VANDER"
echo "PLATFORM=$PLATFORM"
echo "OS_VERSION=$OS_VERSION"
echo "MAKESELF_ARGS=$MAKESELF_ARGS"


cd ../../
export TARGET_PLATFORM_NAME="Poky (Yocto Project Reference Distro) 1.7.1"
export TARGET_PLATFORM_ARCH="x86_64"
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
#./autobuild.sh '3.3.999' ''


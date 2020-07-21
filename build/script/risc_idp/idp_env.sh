#!/bin/bash
source ../../../../risc-env intel baytrail idp
cd ../../

IDP_PLATFORM=$1
IDP_ARCH=x64


if [ "IDP_PLATFORM" == "quark" ] ; then
	IDP_ARCH=x86
fi


IDP_PROJECT="idp-$IDP_PLATFORM-$IDP_ARCH"


export TARGET_PLATFORM_NAME="Linux-WR-IDP-XT-3.1-$IDP_PLATFORM"
export TARGET_PLATFORM_ARCH="$IDP_ARCH"
./autobuild.sh '' 'idp'

#build upgrade package
./autobuild.sh '3.3.999' 'idp'


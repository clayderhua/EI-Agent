#!/bin/bash

export DISABLE_HDD=1
export DISABLE_X11=1
export DISABLE_MCAFEE=1
export DISABLE_ACRONIS=1

source ../../../../risc-env wise openwrt yocto 1.0.0
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
#./autobuild.sh '3.3.999' 'wise-3310'


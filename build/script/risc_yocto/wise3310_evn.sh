#!/bin/bash
source ../../../../risc-env fsl imx6 yocto
cd ../../

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
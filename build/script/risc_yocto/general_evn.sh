#!/bin/bash
source ../../../../risc-env fsl imx6 yocto
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

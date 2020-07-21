#!/bin/bash
source ../../../../risc-env qualcomm dragon yocto
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


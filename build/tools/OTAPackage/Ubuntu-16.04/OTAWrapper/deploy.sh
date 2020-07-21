#!/bin/bash
set -x

TARGET_FILE=`find . -name '*.run'`
echo "$TARGET_FILE"
chmod 755 "$TARGET_FILE"
sh "$TARGET_FILE"
echo $? > result
exit $?

#!/bin/bash
set -x

TARGET_DIR=/usr/local/AgentService
TARGET_APP=cagent

if [ -f "result" ]; then
	value=`cat result`
	if [ $value -eq 1 ]; then
		echo Check Fail >> checklog.txt
		exit 1
	fi
fi

if [ -f $TARGET_DIR/$TARGET_APP ]; then
	echo Check Success >> checklog.txt
	exit 0
else
	echo Check Fail >> checklog.txt
	exit 1
fi

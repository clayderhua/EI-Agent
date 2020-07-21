#!/bin/bash
NAME=$1
CMA_SCRIPT=/etc/init.d/$NAME
RETVAL=0

agent_stop ()
{
	# echo "sending request to stop $NAME" 
	$CMA_SCRIPT stop >/dev/null 2>&1
#	RETVAL=$?
	RETVAL=0
	if [ $RETVAL == 0 ]; then
		echo " OK "
	else
		echo " Fail "
	fi
}

agent_start ()
{
	# echo "sending request to start $NAME"
	$CMA_SCRIPT restart >/dev/null 2>&1
#	RETVAL=$?
	RETVAL=0
	if [ $RETVAL == 0 ]; then
		echo " OK "
	else
		echo " Fail "
	fi
}

agent_status ()
{
	# echo "sending request to get $NAME status" 
	$CMA_SCRIPT status 2>&1
}

if [ "$2" == "start" ]; then
	agent_start;
elif [ "$2" == "stop" ]; then
	agent_stop;
else
	agent_status;
fi

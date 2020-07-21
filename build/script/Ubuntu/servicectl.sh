#!/bin/bash
NAME=$1

PATH_SYSTEMD="/lib/systemd/system"
PATH_SYSTEMD_ETC="/etc/systemd/system/"
PATH_SYSTEMD_AUTO="/etc/systemd/system/multi-user.target.wants/"
PATH_UPSTART="/etc/init"
PATH_SYSV="/etc/init.d"

SERVICE_NAME=$1
SERVICE_SYSTEMD="$SERVICE_NAME.service"
SERVICE_UPSTART="$SERVICE_NAME.conf"
SERVICE_SYSV="$SERVICE_NAME"

function cma_script()
{
	STATUS=$1
	# => start stop restart status
	if [ -d "$PATH_SYSTEMD" ]; then
		if [ -n "`command -v systemctl`" ]; then
			systemctl daemon-reload
			sleep 1
			systemctl $STATUS $SERVICE_SYSTEMD
		else
			initctl $STATUS $SERVICE_NAME
		fi
	elif [ -d "$PATH_UPSTART" ]; then
		initctl $STATUS $SERVICE_NAME
	else
		$PATH_SYSV/$SERVICE_SYSV $STATUS
	fi
}

agent_stop ()
{
	# echo "sending request to stop $NAME" 
	cma_script stop >/dev/null 2>&1
	RETVAL=$?
	if [ $RETVAL -eq 0 ]; then
		echo " OK "
	else
		echo " Fail "
	fi
}

agent_start ()
{
	# echo "sending request to start $NAME" 
	cma_script start >/dev/null 2>&1
	RETVAL=$?
	if [ $RETVAL -eq 0 ]; then
		echo " OK "
	else
		echo " Fail "
	fi
}

agent_status ()
{
	# echo "sending request to get $NAME status" 
	cma_script status 2>&1
}

if [ "$2" == "start" ]; then
	agent_start;
elif [ "$2" == "stop" ]; then
	agent_stop;
else
	agent_status;
fi

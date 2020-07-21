#!/bin/bash
NAME=$1
CMA_SCRIPT=/etc/init.d/$NAME
SAAGENT_LOCK="/var/lock/subsys/cagent"

PATH_SYSTEMD="/lib/systemd/system"
PATH_SYSTEMD_ETC="/etc/systemd/system/"
PATH_SYSTEMD_AUTO="/etc/systemd/system/multi-user.target.wants/"
PATH_UPSTART="/etc/init"
PATH_SYSV="/etc/init.d"

function cma_script()
{
	STATUS=$1
	SERVICE_NAME=$2
	SERVICE_SYSTEMD="$SERVICE_NAME.service"
	SERVICE_UPSTART="$SERVICE_NAME.conf"
	SERVICE_SYSV="$SERVICE_NAME"
	
	# => start stop restart status
	if [ -d "$PATH_SYSTEMD" ]; then
		if [ -n "`command -v systemctl`" ]; then
			systemctl daemon-reload
			sleep 1
			systemctl $STATUS $SERVICE_SYSTEMD
		#elif [ -n "`command -v initctl`" ]; then
		#	initctl $STATUS $SERVICE_NAME
		else
			$PATH_SYSV/$SERVICE_SYSV $STATUS
		fi
	elif [ -d "$PATH_UPSTART" ]; then
		if [ -n "`command -v init-checkconf`" ]; then
			initctl $STATUS $SERVICE_NAME
		else
			$PATH_SYSV/$SERVICE_SYSV $STATUS
		fi
	else
		$PATH_SYSV/$SERVICE_SYSV $STATUS
	fi
}

agent_stop ()
{
	# echo "sending request to stop $NAME" 
	#$CMA_SCRIPT stop >/dev/null 2>&1
	cma_script stop $NAME  >/dev/null 2>&1
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
	if [ -f ${SAAGENT_LOCK} ]; then
		#$CMA_SCRIPT stop >/dev/null 2>&1
		cma_script stop $NAME  >/dev/null 2>&1
	fi
	#$CMA_SCRIPT start >/dev/null 2>&1
	cma_script start $NAME  >/dev/null 2>&1
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
	#$CMA_SCRIPT status 2>&1
	cma_script status $NAME 2>&1
}

if [ "$2" == "start" ]; then
	agent_start;
elif [ "$2" == "stop" ]; then
	agent_stop;
else
	agent_status;
fi

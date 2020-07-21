#!/bin/sh
echo "Uninstall AgentService." 

#Add for Yocto
PATH_SYSTEMD="/etc/systemd/system"
PATH_SYSTEMD_ETC="/etc/systemd/system"
PATH_SYSTEMD_AUTO="/etc/systemd/system/multi-user.target.wants"
PATH_UPSTART="/etc/init"
PATH_SYSV="/etc/init.d"

SERVICE_NAME="saagent"
SERVICE_SYSTEMD="$SERVICE_NAME.service"
SERVICE_UPSTART="$SERVICE_NAME.conf"
SERVICE_SYSV="$SERVICE_NAME"

WATCHDOG_NAME="sawatchdog"
WATCHDOG_SYSTEMD="$WATCHDOG_NAME.service"
WATCHDOG_UPSTART="$WATCHDOG_NAME.conf"
WATCHDOG_SYSV="$WATCHDOG_NAME"

LOGD_NAME="logd"
LOGD_SYSTEMD="$LOGD_NAME.service"
LOGD_SYSV="$LOGD_NAME"

ACCOUNT_ROOT="`who | grep "root" | cut -d ' ' -f 1 | head -n 1`"

if [ $ACCOUNT_ROOT == "root" ] || [ -z "`command -v sudo`" ]; then 
	alias sudo=''
fi

function check_yocto()
{
	local retval=""
	V_YOCTO=`cat /etc/issue |grep Yocto`
	V_WRL=`cat /etc/issue |grep "Wind River Linux"`
	
	if [ -n "$V_YOCTO" ] || [ -n "$V_WRL" ];then # Yocto
		retval="yocto"
	else
		retval="x86"
	fi
	eval "$1=$retval"
}

function cma_script()
{
	STATUS=$1 
	# => start stop restart status
	#if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			systemctl daemon-reload
			sleep 1
			systemctl $STATUS $LOGD_SYSTEMD
			systemctl $STATUS $SERVICE_SYSTEMD
			systemctl $STATUS $WATCHDOG_SYSTEMD
		elif [ -d "$PATH_UPSTART" ]; then
			echo "initctl $STATUS $SERVICE_NAME"
			echo "initctl $STATUS $WATCHDOG_NAME"
		else
			$PATH_SYSV/$LOGD_SYSTEMD $STATUS
			$PATH_SYSV/$SERVICE_SYSV $STATUS
			$PATH_SYSV/$WATCHDOG_SYSV $STATUS
		fi
	#else
	#	$PATH_SYSV/$SERVICE_SYSV $STATUS
	#	$PATH_SYSV/$WATCHDOG_SYSV $STATUS			
	#fi	
}

#check_yocto check_ver
path="/usr/local/AgentService"
SAAGENT_CONF="${path}/agent_config.xml"
servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $SAAGENT_CONF)
file="/var/run/$servername.pid"

if [ -f "$file" ]
then
	addpid=`cat "$file"`
	echo "Pid: $addpid"
	
	appdir=$(sudo readlink /proc/$addpid/cwd)
	if [ -d "$appdir" ]; then
	      echo "find app dir $appdir."
	      path=$appdir
	fi
else
	echo "$file not found."
fi
echo "AgentService Path: $path"
if [ -d "$path" ]; then
	#if [ "$check_ver" != "yocto" ]; then
  	#   sudo $PATH_SYSV/$SERVICE_SYSV stop
	#   sudo $PATH_SYSV/$WATCHDOG_SYSV stop
	#else
		cma_script stop
	#fi	
  
	if [  -n "`command -v insserv`" ]; then
		sudo /sbin/insserv -r $LOGD_SYSV
		sudo /sbin/insserv -r $SERVICE_SYSV
		sudo /sbin/insserv -r $WATCHDOG_SYSV
	elif [ -n "`command -v update-rc.d`" ]; then
		sudo update-rc.d -f $LOGD_SYSV remove
		sudo update-rc.d -f $SERVICE_SYSV remove
		sudo update-rc.d -f $WATCHDOG_SYSV remove
	elif [ -n "`command -v chkconfig`" ]; then
		sudo chkconfig --del $LOGD_SYSV
		sudo chkconfig --del $SERVICE_SYSV
		sudo chkconfig --del $WATCHDOG_SYSV
	else 
		echo "Please install insserv or update-rc.d tool."
	fi 
	sleep 3
	#if [ "$check_ver" != "yocto" ]; then
	#	sudo systemctl --system daemon-reload
        #	sudo rm $PATH_SYSV/$SERVICE_SYSV
	#	sudo rm $PATH_SYSV/$WATCHDOG_SYSV
	#else
		if [ -d "$PATH_SYSTEMD" ]; then
			rm $PATH_SYSTEMD/$LOGD_SYSTEMD
			rm $PATH_SYSTEMD/$SERVICE_SYSTEMD
			rm $PATH_SYSTEMD/$WATCHDOG_SYSTEMD
			rm $PATH_SYSTEMD_ETC/$LOGD_SYSTEMD
			rm $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD
			rm $PATH_SYSTEMD_ETC/$WATCHDOG_SYSTEMD
			rm $PATH_SYSTEMD_AUTO/$LOGD_SYSTEMD
			rm $PATH_SYSTEMD_AUTO/$SERVICE_SYSTEMD
			rm $PATH_SYSTEMD_AUTO/$WATCHDOG_SYSTEMD
		elif [ -d "$PATH_UPSTART" ]; then
			echo "rm $PATH_UPSTART/$SERVICE_UPSTART"
			echo "rm $PATH_UPSTART/$WATCHDOG_UPSTART"
		else
		  rm $PATH_SYSV/$LOGD_SYSV
		  rm $PATH_SYSV/$SERVICE_SYSV
		  rm $PATH_SYSV/$WATCHDOG_SYSV
		fi
	#fi
  
  echo "remove folder $path"
  rm -rf "$path"

  killall -s 9 cagent
  killall -s 9 sawatchdog
  
  sudo rm -f /var/run/AgentService.pid
  sudo rm -f /var/lock/subsys/cagent

  sudo rm -f /var/run/SAWatchdog.pid
  sudo rm -f /var/lock/subsys/sawatchdog

  
fi
echo "done."

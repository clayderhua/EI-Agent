#!/bin/sh
echo "Uninstall AgentService." 

#Add for Yocto
PATH_SYSTEMD="/usr/lib/systemd/system"  #or "/etc/systemd/system"
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

WEBSOCKIFY_NAME="sawebsockify"
WEBSOCKIFY_SYSTEMD="$WEBSOCKIFY_NAME.service"
WEBSOCKIFY_UPSTART="$WEBSOCKIFY_NAME.conf"
WEBSOCKIFY_SYSV="$WEBSOCKIFY_NAME"

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
	if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			echo "systemctl $STATUS $SERVICE_NAME"
			echo "systemctl $STATUS $WATCHDOG_NAME"
			echo "systemctl $STATUS $WEBSOCKIFY_NAME"
		elif [ -d "$PATH_UPSTART" ]; then
			echo "initctl $STATUS $SERVICE_NAME"
			echo "initctl $STATUS $WATCHDOG_NAME"
			echo "initctl $STATUS $WEBSOCKIFY_NAME"
		else
			$PATH_SYSV/$SERVICE_SYSV $STATUS
			$PATH_SYSV/$WATCHDOG_SYSV $STATUS
			$PATH_SYSV/$WEBSOCKIFY_SYSV $STATUS
		fi
	else
		$PATH_SYSV/$SERVICE_SYSV $STATUS
		$PATH_SYSV/$WATCHDOG_SYSV $STATUS
		$PATH_SYSV/$WEBSOCKIFY_SYSV $STATUS			
	fi	
}

check_yocto check_ver
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
	if [ "$check_ver" != "yocto" ]; then
  	   sudo $PATH_SYSV/$SERVICE_SYSV stop
	   sudo $PATH_SYSV/$WATCHDOG_SYSV stop
	   sudo $PATH_SYSV/$WEBSOCKIFY_SYSV stop
	else
		cma_script stop
	fi	
  
	if [  -n "`command -v insserv`" ]; then
		sudo /sbin/insserv -r $SERVICE_SYSV
		sudo /sbin/insserv -r $WATCHDOG_SYSV
		sudo /sbin/insserv -r $WEBSOCKIFY_SYSV
	elif [ -n "`command -v update-rc.d`" ]; then
		sudo update-rc.d -f $SERVICE_SYSV remove
		sudo update-rc.d -f $WATCHDOG_SYSV remove
		sudo update-rc.d -f $WEBSOCKIFY_SYSV remove
	elif [ -n "`command -v chkconfig`" ]; then
		sudo chkconfig --del $SERVICE_SYSV
		sudo chkconfig --del $WATCHDOG_SYSV
		sudo chkconfig --del $WEBSOCKIFY_SYSV
	else 
		echo "Please install insserv or update-rc.d tool."
	fi 
	sleep 3
	if [ "$check_ver" != "yocto" ]; then
		sudo systemctl --system daemon-reload
        	sudo rm $PATH_SYSV/$SERVICE_SYSV
		sudo rm $PATH_SYSV/$WATCHDOG_SYSV
		sudo rm $PATH_SYSV/$WEBSOCKIFY_SYSV
	else
		if [ -d "$PATH_SYSTEMD" ]; then
			echo "rm $PATH_SYSTEMD/$SERVICE_SYSTEMD"
			echo "rm $PATH_SYSTEMD/$WATCHDOG_SYSTEMD"
			echo "rm $PATH_SYSTEMD/$WEBSOCKIFY_SYSTEMD"
		elif [ -d "$PATH_UPSTART" ]; then
			echo "rm $PATH_UPSTART/$SERVICE_UPSTART"
			echo "rm $PATH_UPSTART/$WATCHDOG_UPSTART"
			echo "rm $PATH_UPSTART/$WEBSOCKIFY_UPSTART"
		else
		  rm $PATH_SYSV/$SERVICE_SYSV	
		  rm $PATH_SYSV/$WATCHDOG_SYSV
		  rm $PATH_SYSV/$WEBSOCKIFY_SYSV
		fi
	fi
  
  echo "remove folder $path"
  rm -rf "$path"

  killall -s 9 cagent
  killall -s 9 sawatchdog
  killall -s 9 websockify
  
  sudo rm -f /var/run/AgentService.pid
  sudo rm -f /var/lock/subsys/cagent

  sudo rm -f /var/run/SAWatchdog.pid
  sudo rm -f /var/lock/subsys/sawatchdog

  sudo rm -f /var/run/SAWebsockify.pid
  sudo rm -f /var/lock/subsys/websockify
  
fi
echo "done."

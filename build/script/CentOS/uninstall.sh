#!/bin/sh
echo "Uninstall AgentService." 

PATH_SYSTEMD="/lib/systemd/system"
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

WEBSOCKET_NAME="sawebsockify"
WEBSOCKET_SYSTEMD="$WEBSOCKET_NAME.service"
WEBSOCKET_UPSTART="$WEBSOCKET_NAME.conf"
WEBSOCKET_SYSV="$WEBSOCKET_NAME"

LOGD_NAME="logd"
LOGD_SYSTEMD="$LOGD_NAME.service"
LOGD_SYSV="$LOGD_NAME"

function cma_script()
{
	STATUS=$1 
	# => start stop restart status
	#if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			if [ -n "`command -v systemctl`" ]; then
				systemctl daemon-reload
				sleep 1
				systemctl $STATUS $LOGD_SYSTEMD
				systemctl $STATUS $SERVICE_SYSTEMD
				systemctl $STATUS $WEBSOCKET_SYSTEMD
				systemctl $STATUS $WATCHDOG_SYSTEMD
			#elif [ -n "`command -v initctl`" ]; then
			#	initctl $STATUS $SERVICE_NAME
			#	initctl $STATUS $WEBSOCKET_NAME
			#	initctl $STATUS $WATCHDOG_NAME
			else
				$PATH_SYSV/$LOGD_SYSV $STATUS
				$PATH_SYSV/$SERVICE_SYSV $STATUS
				$PATH_SYSV/$WEBSOCKET_SYSV $STATUS
				$PATH_SYSV/$WATCHDOG_SYSV $STATUS
			fi
		elif [ -d "$PATH_UPSTART" ]; then
			if [ -n "`command -v init-checkconf`" ]; then
				initctl $STATUS $SERVICE_NAME
				initctl $STATUS $WEBSOCKET_NAME
				initctl $STATUS $WATCHDOG_NAME
			else
				$PATH_SYSV/$LOGD_SYSV $STATUS
				$PATH_SYSV/$SERVICE_SYSV $STATUS
				$PATH_SYSV/$WEBSOCKET_SYSV $STATUS
				$PATH_SYSV/$WATCHDOG_SYSV $STATUS
			fi
		else
			$PATH_SYSV/$LOGD_SYSV $STATUS
			$PATH_SYSV/$SERVICE_SYSV $STATUS
			$PATH_SYSV/$WEBSOCKET_SYSV $STATUS
			$PATH_SYSV/$WATCHDOG_SYSV $STATUS
		fi
	#else
	#	$PATH_SYSV/$SERVICE_SYSV $STATUS
	#	$PATH_SYSV/$WATCHDOG_SYSV $STATUS			
	#fi	
}

# user check; must be root
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "You must be root !"
	exit -1
fi

path="/usr/local/AgentService"
SAAGENT_CONF="${path}/agent_config.xml"
servername=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $SAAGENT_CONF)
file="/var/run/$servername.pid"
if [ -f "$file" ]
then
	addpid=$(cat "$file")
	echo "Pid: $addpid"
	
	appdir=$(readlink /proc/$addpid/cwd)
	if [ -d "$appdir" ]; then
	      echo "find app dir $appdir."
	      path=$appdir
	fi
else
	echo "$file not found."
fi
echo "AgentService Path: $path"

if [ -d "$path" ]; then
	cma_script stop
	  
	if [  -n "`command -v insserv`" ]; then
		sudo /sbin/insserv -r $LOGD_SYSV
		sudo /sbin/insserv -r $SERVICE_SYSV
		sudo /sbin/insserv -r $WEBSOCKET_SYSV
		sudo /sbin/insserv -r $WATCHDOG_SYSV
	elif [ -n "`command -v update-rc.d`" ]; then
		sudo update-rc.d -f $LOGD_SYSV remove
		sudo update-rc.d -f $SERVICE_SYSV remove
		sudo update-rc.d -f $WEBSOCKET_SYSV remove
		sudo update-rc.d -f $WATCHDOG_SYSV remove
	elif [ -n "`command -v chkconfig`" ]; then
		sudo chkconfig --del $LOGD_SYSV
		sudo chkconfig --del $SERVICE_SYSV
		sudo chkconfig --del $WEBSOCKET_SYSV
		sudo chkconfig --del $WATCHDOG_SYSV
	else 
		echo "Please install insserv or update-rc.d tool."
	fi 
	sleep 3
	if [ -d "$PATH_SYSTEMD" ]; then
		rm $PATH_SYSTEMD/$LOGD_SYSTEMD
		rm $PATH_SYSTEMD/$SERVICE_SYSTEMD
		rm $PATH_SYSTEMD/$WEBSOCKET_SYSTEMD
		rm $PATH_SYSTEMD/$WATCHDOG_SYSTEMD
		rm $PATH_SYSTEMD_ETC/$LOGD_SYSTEMD
		rm $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD
		rm $PATH_SYSTEMD_ETC/$WATCHDOG_SYSTEMD
		rm $PATH_SYSTEMD_ETC/$WEBSOCKET_SYSTEMD
		rm $PATH_SYSTEMD_AUTO/$LOGD_SYSTEMD
		rm $PATH_SYSTEMD_AUTO/$SERVICE_SYSTEMD
		rm $PATH_SYSTEMD_AUTO/$WEBSOCKET_SYSTEMD
		rm $PATH_SYSTEMD_AUTO/$WATCHDOG_SYSTEMD
	elif [ -d "$PATH_UPSTART" ]; then
		echo "rm $PATH_UPSTART/$SERVICE_UPSTART"
		rm -f $PATH_UPSTART/$SERVICE_UPSTART
		echo "rm $PATH_UPSTART/$WEBSOCKET_UPSTART"
		rm -f $PATH_UPSTART/$WEBSOCKET_UPSTART
		echo "rm $PATH_UPSTART/$WATCHDOG_UPSTART"
		rm -f $PATH_UPSTART/$WATCHDOG_UPSTART
	else
		rm $PATH_SYSV/$LOGD_SYSV
	  	rm $PATH_SYSV/$SERVICE_SYSV
	  	rm $PATH_SYSV/$WEBSOCKET_SYSV
		rm $PATH_SYSV/$WATCHDOG_SYSV
	fi
  
  echo "remove folder $path"
  rm -rf "$path"

  killall -s 9 $SERVICE_NAME
  killall -s 9 $WEBSOCKET_NAME
  killall -s 9 $WATCHDOG_NAME
  
  rm -f /var/run/AgentService.pid
  rm -f /var/lock/subsys/cagent
  
  rm -f /var/lock/subsys/sawebsockify
  rm -f /var/run/sawebsockify.pid

  rm -f /var/run/SAWatchdog.pid
  rm -f /var/lock/subsys/sawatchdog
fi
echo "done."

#!/bin/bash
echo "Install AgentService." 
CURDIR=${PWD} 
echo $CURDIR
TDIR=`mktemp -d`
cfgtmp=$TDIR/

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

LOGD_NAME="logd"
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

function modulebackup()
{
	if [[ $curversion == $newversion ]]; then
		echo "backup module files"
		cp -vfr $APPDIR/module $cfgtmp/module
	else
		local IFS=.
		local i ver1=($curversion) ver2=($newversion)
		tmp1=${ver1[0]}.${ver1[1]}
		tmp2=${ver2[0]}.${ver2[1]}
		#read -p "$tmp1 $tmp2"
		if [[ $tmp1 == $tmp2 ]]; then
			echo "backup module files"
			cp -vfr $APPDIR/module $cfgtmp/module
		else
			echo "Different version, modules cannot reused"
		fi
	fi
}

function modulerestore()
{
	if [ -d "$cfgtmp/module" ]
	then
		echo "restore module files"
		"$APPDIR"ModuleMerge "$APPDIR"module "$cfgtmp"module
		#cp -vuR $cfgtmp/module/*.so* "$APPDIR"module
	fi
}

function pkgbackup()
{
	cp -a ${APPDIR}/Pkg ${cfgtmp}/
}

function pkgrestore()
{
	cp -a ${cfgtmp}/Pkg ${APPDIR}/
}

function clearcfgtmp()
{
	if [ -f "$cfgtmp" ]
	then
		echo "remove tmp dir $cfgtmp."
		sudo rm -rf "$cfgtmp"
	fi
}

function cfgbackup()
{
	echo "backup config files"
	
	clearcfgtmp
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.xml' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.xml') "$cfgtmp"
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.ini' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.ini') "$cfgtmp"
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.cfg' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.cfg') "$cfgtmp"
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.db' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.db') "$cfgtmp"
	modulebackup
	pkgbackup
}

function cfgrestore()
{
	if [ -d "$cfgtmp" ]
	then
		modulerestore
		pkgrestore
		echo "restore config files"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.xml' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.xml') "$APPDIR"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.ini' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.ini') "$APPDIR"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.cfg' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.cfg') "$APPDIR"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.db' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.db') "$APPDIR"
		#rm -rf "$cfgtmp"
	fi
}

#Main()
check_yocto check_ver
# Collect new config file information
INSTALL_AGENT_CONFIG="${CURDIR}/AgentService/agent_config.xml"
if [ -e ${INSTALL_AGENT_CONFIG} ]; then
	servicename=$(sed -n 's:.*<ServiceName>\(.*\)</ServiceName>.*:\1:p' $INSTALL_AGENT_CONFIG)
	newversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $INSTALL_AGENT_CONFIG)
	serverport=$(sed -n 's:.*<ServerPort>\(.*\)</ServerPort>.*:\1:p' $INSTALL_AGENT_CONFIG)
	osVersion=$(sed -n 's:.*<osVersion>\(.*\)</osVersion>.*:\1:p' $INSTALL_AGENT_CONFIG)
	osArch=$(sed -n 's:.*<osArch>\(.*\)</osArch>.*:\1:p' $INSTALL_AGENT_CONFIG)
else
	echo "not found: ${INSTALL_AGENT_CONFIG}"
fi
# check current config file information
APPDIR="/usr/local/AgentService/"
APP_UNINSTALL="${APPDIR}uninstall.sh"
APP_AGENT_CONFIG="${APPDIR}agent_config.xml"
curversion=0.0.0.0
if [ -d ${APPDIR} ]; then
	if [ -f ${APP_AGENT_CONFIG} ]; then
		curversion=$(sed -n 's:.*<SWVersion>\(.*\)</SWVersion>.*:\1:p' $APP_AGENT_CONFIG)
	fi

	$PATH_SYSV/$SERVICE_SYSV stop
	$PATH_SYSV/$LOGD_SYSV stop
	cfgbackup

	if  [ -f ${APP_UNINSTALL} ]; then	
		echo "Found RMM agent already installed. Remove and install the new ..."
		chmod +x "${APP_UNINSTALL}"
		${APP_UNINSTALL}
	fi
fi

echo "Copy AgentService to /usr/local."
if [ ! -d /usr/local ]; then
	mkdir -p /usr/local/
fi
cp -avr ./AgentService /usr/local

# combine Agent.config as deault setting
CREDENTIAL_URL="${1}"
IOT_KEY="${2}"
if [ -n "${CREDENTIAL_URL}" ]; then
	echo "Combine Agent.conf"
	combine_xml ${APP_AGENT_CONFIG}
	cp -f combine.xml ${APP_AGENT_CONFIG}
fi

# restore config if avaliable
cfgrestore;

# Overwrite Server Port and Service Name
if [[ $curversion == 3.0.* ]]; then
	if [[ $newversion == 3.0.* ]]; then
		echo "Not Overwrite Server Port"
	else
		echo "Overwrite Server Port"
		sed -i "s|\(<ServerPort>\)[^<>]*\(</ServerPort>\)|\1$serverport\2|" $APP_AGENT_CONFIG
		sed -i "s|\(<ServiceName>\)[^<>]*\(</ServiceName>\)|\1$servicename\2|" $APP_AGENT_CONFIG
	fi
fi

# Overwrite OS info.

if  [[ ! -z $osVersion ]]; then
	tmposver=$(sed -n 's:.*<osVersion>\(.*\)</osVersion>.*:\1:p' $APP_AGENT_CONFIG)
	if [ -z "$tmposver" ]; then
		tmposver="<osVersion>$osVersion</osVersion>\n"
		sed -i "s|</Profiles>|$tmposver</Profiles>|g" $APP_AGENT_CONFIG
		echo "Insert to <Profiles/> $tmposver"
	else
		sed -i "s|\(<osVersion>\)[^<>]*\(</osVersion>\)|\1$osVersion\2|" $APP_AGENT_CONFIG
		sed -i "s|<osVersion/>|<osVersion>$osVersion</osVersion>|g" $APP_AGENT_CONFIG
		echo "Update <osVersion/> $osVersion"
	fi
fi

if  [[ ! -z $osArch ]]; then
	tmposarch=$(sed -n 's:.*<osArch>\(.*\)</osArch>.*:\1:p' $APP_AGENT_CONFIG)
	if [ -z "$tmposarch" ]; then
		tmposarch="<osArch>$osArch</osArch>\n"
		sed -i "s|</Profiles>|$tmposarch</Profiles>|g" $APP_AGENT_CONFIG
		echo "Insert to <Profiles/> $tmposarch"
	else
		sed -i "s|\(<osArch>\)[^<>]*\(</osArch>\)|\1$osArch\2|" $APP_AGENT_CONFIG
		sed -i "s|<osArch/>|<osArch>$osArch</osArch>|g" $APP_AGENT_CONFIG
		echo "Update <osArch/> $osArch"
	fi
fi

#if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "exec systemd init"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "exec upstart init"
	else
		cp ./$LOGD_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$LOGD_SYSV
		chmod 0750 $PATH_SYSV/$LOGD_SYSV
		cp ./$SERVICE_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$SERVICE_SYSV
		chmod 0750 $PATH_SYSV/$SERVICE_SYSV
	fi
else
	cp ./$LOGD_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$LOGD_SYSV
	chmod 0750 $PATH_SYSV/$LOGD_SYSV
	cp ./$SERVICE_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$SERVICE_SYSV
	chmod 0750 $PATH_SYSV/$SERVICE_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "move $SERVICE_SYSTEMD to $PATH_SYSTEMD"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "move $SERVICE_UPSTART to $PATH_UPSTART"
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $LOGD_SYSV
			/sbin/insserv -v $PATH_SYSV/$LOGD_SYSV
			/sbin/insserv -r $SERVICE_SYSV
			/sbin/insserv -v $PATH_SYSV/$SERVICE_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $LOGD_SYSV remove
			update-rc.d $LOGD_SYSV defaults 98 21
			update-rc.d -f $SERVICE_SYSV remove
			update-rc.d $SERVICE_SYSV defaults 99 20
		elif [ -n "`command -v chkconfig`" ]; then
			chkconfig --del $LOGD_SYSV
			chkconfig --add $LOGD_SYSV
			chkconfig --del $SERVICE_SYSV
			chkconfig --add $SERVICE_SYSV
		else 
			echo "Please install insserv/update-rc.d/chkconfig tool."
		fi
	fi		
else
	sudo /sbin/insserv -v $PATH_SYSV/$LOGD_SYSV
	sudo /sbin/insserv -v $PATH_SYSV/$SERVICE_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "exec systemd init"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "exec upstart init"
	else
		cp ./$WATCHDOG_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$WATCHDOG_SYSV
		chmod 0750 $PATH_SYSV/$WATCHDOG_SYSV
	fi
else
	sudo cp ./$WATCHDOG_SYSV $PATH_SYSV
	sudo chown root:root $PATH_SYSV/$WATCHDOG_SYSV
	sudo chmod 0750 $PATH_SYSV/$WATCHDOG_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "move $WATCHDOG_SYSTEMD to $PATH_SYSTEMD"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "move $WATCHDOG_UPSTART to $PATH_UPSTART"
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $WATCHDOG_SYSV
			/sbin/insserv -v $PATH_SYSV/$WATCHDOG_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $WATCHDOG_SYSV remove
			update-rc.d $WATCHDOG_SYSV defaults 99 20
		elif [ -n "`command -v chkconfig`" ]; then
			chkconfig --del $WATCHDOG_SYSV
			chkconfig --add $WATCHDOG_SYSV
		else 
			echo "Please install insserv/update-rc.d/chkconfig tool."
		fi
	fi		
else
	sudo /sbin/insserv -v $PATH_SYSV/$WATCHDOG_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "exec systemd init"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "exec upstart init"
	else
		cp ./$WEBSOCKIFY_SYSV $PATH_SYSV
		chown root:root $PATH_SYSV/$WEBSOCKIFY_SYSV
		chmod 0750 $PATH_SYSV/$WEBSOCKIFY_SYSV
	fi
else
	sudo cp ./$WEBSOCKIFY_SYSV $PATH_SYSV
	sudo chown root:root $PATH_SYSV/$WEBSOCKIFY_SYSV
	sudo chmod 0750 $PATH_SYSV/$WEBSOCKIFY_SYSV
fi

if [ "$check_ver" == "yocto" ]; then
	if [ -d "$PATH_SYSTEMD" ]; then
		echo "move $WEBSOCKIFY_SYSTEMD to $PATH_SYSTEMD"
	elif [ -d "$PATH_UPSTART" ]; then
		echo "move $WEBSOCKIFY_UPSTART to $PATH_UPSTART"
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $WEBSOCKIFY_SYSV
			/sbin/insserv -v $PATH_SYSV/$WEBSOCKIFY_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $WEBSOCKIFY_SYSV remove
			update-rc.d $WEBSOCKIFY_SYSV defaults 99 20
		elif [ -n "`command -v chkconfig`" ]; then
			chkconfig --del $WEBSOCKIFY_SYSV
			chkconfig --add $WEBSOCKIFY_SYSV
		else 
			echo "Please install insserv/update-rc.d/chkconfig tool."
		fi
	fi		
else
	sudo /sbin/insserv -v $PATH_SYSV/$WEBSOCKIFY_SYSV
fi

if [ "$check_ver" != "yocto" ]; then
	sudo systemctl --system daemon-reload
fi

if [ "$check_ver" != "yocto" ]; then
	find /home/ -name 'Autostart' -print0 | while IFS= read -r -d $'\0' line; do
		echo "$line"
		cp ./AgentService/xhostshare.sh "$line"
		chown --reference="$line" "$line/xhostshare.sh"
		chmod 755 "$line/xhostshare.sh"
	done
fi

read -t 10 -p "Setup Agent [y/n](default:n): " ans

if [ "$ans" == "y" ]; then
	sudo /usr/local/AgentService/setup.sh
else
	if [ "$check_ver" == "yocto" ]; then
		if [ -d "$PATH_SYSTEMD" ]; then
			echo "systemctl start $SERVICE_NAME"
		elif [ -d "$PATH_UPSTART" ]; then
			echo "initctl start $SERVICE_NAME"
		else
			echo "Start Logd."
			$PATH_SYSV/$LOGD_SYSV start

			echo "Start WISE-Agent."
			$PATH_SYSV/$SERVICE_SYSV start

			echo "Start Watchdog Agent."
			$PATH_SYSV/$WATCHDOG_SYSV start

			echo "Start Websockify Agent."
			$PATH_SYSV/$WEBSOCKIFY_SYSV start
		fi
	else
		echo "Start WISE-Agent."
		echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
		sudo $PATH_SYSV/$LOGD_SYSV start
		sudo $PATH_SYSV/$SERVICE_SYSV start
		sudo $PATH_SYSV/$WATCHDOG_SYSV start
		sudo $PATH_SYSV/$WEBSOCKIFY_SYSV start
	fi	
fi

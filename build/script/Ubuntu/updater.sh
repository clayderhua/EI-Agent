#!/bin/bash
echo "Install AgentService." 
CURDIR=${PWD} 
echo $CURDIR
TDIR=`mktemp -d`
cfgtmp=$TDIR/

PATH_SYSTEMD="/lib/systemd/system"
PATH_SYSTEMD_ETC="/etc/systemd/system/"
PATH_SYSTEMD_AUTO="/etc/systemd/system/multi-user.target.wants/"
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

#====== Some function that you need modify ======#
platformchk()
{
	if [ -f "package_info" ]; then
		source package_info
		PLATFORM="$(lsb_release -is | sed 's/\"//g') $(lsb_release -rs | sed 's/\"//g')"
		ARCH=$(uname -m)
		if [[ $ORIG_ARCH == $ARCH ]]; then
			if [[ $ORIG_PLATFORM == $PLATFORM ]]; then
				echo "INFORMATION: Target device ($PLATFORM) matched with ($ORIG_PLATFORM)."
			else
				echo "WARNING: Target device ($PLATFORM) do not match with ($ORIG_PLATFORM), some functions may not work!!"
			fi
		else
			echo "ERROR: Target device ($ARCH) do not match with ($ORIG_ARCH)!!"
			exit -1
		fi
	fi
}

setxhost()
{	
	local destDir="/etc/profile.d/"
	echo -n "xhost set:"
	if [ -d "$destDir" ]; then
		cp -vf ./AgentService/xhostshare.sh $destDir
		chmod a+x ${destDir}xhostshare.sh
		${destDir}xhostshare.sh
	else
		echo "The folder $destDir is not exist!"
	fi
}

modulebackup()
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

modulerestore()
{
	if [ -d "$cfgtmp/module" ]
	then
		echo "restore module files"
		"$APPDIR"ModuleMerge "$APPDIR"module "$cfgtmp"module
		#cp -vR $cfgtmp/module/*.so* "$APPDIR"module
	fi
}

pkgbackup()
{
	cp -a ${APPDIR}/Pkg ${cfgtmp}/
}

pkgrestore()
{
	cp -a ${cfgtmp}/Pkg ${APPDIR}/
}

clearcfgtmp()
{
	if [ -f "$cfgtmp" ]
	then
		echo "remove tmp dir $cfgtmp."
		sudo rm -rf "$cfgtmp"
	fi
}

cfgbackup()
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

cfgrestore()
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

# read xml dom
function read_dom ()
{
    local IFS=\>
    read -d \< ENTITY CONTENT
}

# combine xml and Agent.config INI
function combine_xml()
{
	rm -f combine.xml
	while read_dom; do
		if [[ "${ENTITY}" == "CredentialURL" && -n "${CREDENTIAL_URL}" ]]; then
			echo -ne "<${ENTITY}>${CREDENTIAL_URL}" >> combine.xml
		elif [[ "${ENTITY}" == "IoTKey" && -n "${IOT_KEY}" ]]; then
			echo -ne "<${ENTITY}>${IOT_KEY}" >> combine.xml
		elif [[ -n "${ENTITY}" ]]; then
			echo -ne "<${ENTITY}>${CONTENT}" >> combine.xml
		fi
	done < ${1}

	echo -ne "<${ENTITY}>" >> combine.xml
}


#===================== main =====================#

platformchk;

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

	systemctl stop saagent
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

# Set xhost for Screenshot and KVM
setxhost

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

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		cp ./$LOGD_SYSTEMD $PATH_SYSTEMD
		chown root:root $PATH_SYSTEMD/$LOGD_SYSTEMD
		chmod 0644 $PATH_SYSTEMD/$LOGD_SYSTEMD
		cp ./$SERVICE_SYSTEMD $PATH_SYSTEMD
		chown root:root $PATH_SYSTEMD/$SERVICE_SYSTEMD
		chmod 0644 $PATH_SYSTEMD/$SERVICE_SYSTEMD
	else
		cp ./$SERVICE_UPSTART $PATH_UPSTART
		chown root:root $PATH_UPSTART/$SERVICE_UPSTART
		chmod 0644 $PATH_UPSTART/$SERVICE_UPSTART
	fi	
elif [ -d "$PATH_UPSTART" ]; then
	cp ./$SERVICE_UPSTART $PATH_UPSTART
	chown root:root $PATH_UPSTART/$SERVICE_UPSTART
	chmod 0644 $PATH_UPSTART/$SERVICE_UPSTART
else
	cp ./$LOGD_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$LOGD_SYSV
	chmod 0750 $PATH_SYSV/$LOGD_SYSV
	cp ./$SERVICE_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$SERVICE_SYSV
	chmod 0750 $PATH_SYSV/$SERVICE_SYSV
fi

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		ln -s $PATH_SYSTEMD/$LOGD_SYSTEMD $PATH_SYSTEMD_ETC/$LOGD_SYSTEMD
		ln -s $PATH_SYSTEMD_ETC/$LOGD_SYSTEMD $PATH_SYSTEMD_AUTO/$LOGD_SYSTEMD
		ln -s $PATH_SYSTEMD/$SERVICE_SYSTEMD $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD
		ln -s $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD $PATH_SYSTEMD_AUTO/$SERVICE_SYSTEMD
	else
		initctl check-config $SERVICE_NAME
	fi		
elif [ -d "$PATH_UPSTART" ]; then
	initctl check-config $SERVICE_NAME
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

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		cp ./$WEBSOCKET_SYSTEMD $PATH_SYSTEMD
		chown root:root $PATH_SYSTEMD/$WEBSOCKET_SYSTEMD
		chmod 0750 $PATH_SYSTEMD/$WEBSOCKET_SYSTEMD
	else
		cp ./$WEBSOCKET_UPSTART $PATH_UPSTART
		chown root:root $PATH_UPSTART/$WEBSOCKET_UPSTART
		chmod 0644 $PATH_UPSTART/$WEBSOCKET_UPSTART
	fi	
elif [ -d "$PATH_UPSTART" ]; then
	cp ./$WEBSOCKET_NAME $PATH_UPSTART
	chown root:root $PATH_UPSTART/$WEBSOCKET_NAME
	chmod 0644 $PATH_UPSTART/$WEBSOCKET_NAME
else
	cp ./$WEBSOCKET_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$WEBSOCKET_SYSV
	chmod 0750 $PATH_SYSV/$WEBSOCKET_SYSV
fi

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		ln -s $PATH_SYSTEMD/$WEBSOCKET_SYSTEMD $PATH_SYSTEMD_ETC/$WEBSOCKET_SYSTEMD
		ln -s $PATH_SYSTEMD_ETC/$WEBSOCKET_SYSTEMD $PATH_SYSTEMD_AUTO/$WEBSOCKET_SYSTEMD	
	else
		initctl check-config $WEBSOCKET_NAME
	fi	
elif [ -d "$PATH_UPSTART" ]; then
	initctl check-config $WEBSOCKET_NAME
else
	if [ -n "`command -v insserv`" ]; then
		/sbin/insserv -r $WEBSOCKET_SYSV
		/sbin/insserv -v $PATH_SYSV/$WEBSOCKET_SYSV
	elif [ -n "`command -v update-rc.d`" ]; then
		update-rc.d -f $WEBSOCKET_SYSV remove
		update-rc.d $WEBSOCKET_SYSV defaults 99 20
	elif [ -n "`command -v chkconfig`" ]; then
		chkconfig --del $WEBSOCKET_SYSV
		chkconfig --add $WEBSOCKET_SYSV
	else 
		echo "Please install insserv/update-rc.d/chkconfig tool."
	fi
fi		

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		cp ./$WATCHDOG_SYSTEMD $PATH_SYSTEMD
		chown root:root $PATH_SYSTEMD/$WATCHDOG_SYSTEMD
		chmod 0750 $PATH_SYSTEMD/$WATCHDOG_SYSTEMD
	else
		cp ./$WATCHDOG_UPSTART $PATH_UPSTART
		chown root:root $PATH_UPSTART/$WATCHDOG_UPSTART
		chmod 0644 $PATH_UPSTART/$WATCHDOG_UPSTART
	fi	
elif [ -d "$PATH_UPSTART" ]; then
	cp ./$WATCHDOG_UPSTART $PATH_UPSTART
	chown root:root $PATH_UPSTART/$WATCHDOG_UPSTART
	chmod 0644 $PATH_UPSTART/$WATCHDOG_UPSTART
else
	cp ./$WATCHDOG_SYSV $PATH_SYSV
	chown root:root $PATH_SYSV/$WATCHDOG_SYSV
	chmod 0750 $PATH_SYSV/$WATCHDOG_SYSV
fi

if [ -d "$PATH_SYSTEMD" ]; then
	if [ -n "`command -v systemctl`" ]; then
		ln -s $PATH_SYSTEMD/$WATCHDOG_SYSTEMD $PATH_SYSTEMD_ETC/$WATCHDOG_SYSTEMD
		ln -s $PATH_SYSTEMD_ETC/$WATCHDOG_SYSTEMD $PATH_SYSTEMD_AUTO/$WATCHDOG_SYSTEMD		
	else
		initctl check-config $WATCHDOG_NAME
	fi
elif [ -d "$PATH_UPSTART" ]; then
	initctl check-config $WATCHDOG_NAME
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

# pre-install 
PRE_INSTALL_FILE="/usr/local/AgentService/pre-install_chk.sh"
if [ -f "${PRE_INSTALL_FILE}" ]; then
	chmod +x $PRE_INSTALL_FILE
	$PRE_INSTALL_FILE
fi


read -t 10 -p "Setup Agent [y/n](default:n): " ans
if [ "$ans" == "y" ]; then
	sudo /usr/local/AgentService/setup.sh
else
	echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
	echo "Start WISE-Agent."
	if [ -d "$PATH_SYSTEMD" ]; then
		if [ -n "`command -v systemctl`" ]; then
			systemctl daemon-reload
			sleep 1
			systemctl start $LOGD_SYSTEMD
			systemctl start $SERVICE_SYSTEMD
			systemctl start $WEBSOCKET_SYSTEMD
			systemctl start $WATCHDOG_SYSTEMD
		else
			initctl start $SERVICE_NAME
			initctl start $WEBSOCKET_NAME
			initctl start $WATCHDOG_NAME
		fi
	elif [ -d "$PATH_UPSTART" ]; then
		echo "initctl start $SERVICE_NAME"
		initctl start $SERVICE_NAME
		echo "initctl start $WEBSOCKET_NAME"
		initctl start $WEBSOCKET_NAME
		echo "initctl start $WATCHDOG_NAME"
		initctl start $WATCHDOG_NAME
	else
		echo "Start Logd."
		$PATH_SYSV/$LOGD_SYSV start
		echo "Start WISE-Agent."
		$PATH_SYSV/$SERVICE_SYSV start
		echo "Start RMM SAWebsockify"
		$PATH_SYSV/$WEBSOCKET_SYSV start
		echo "Start Watchdog Agent."
		$PATH_SYSV/$WATCHDOG_SYSV start
	fi
fi

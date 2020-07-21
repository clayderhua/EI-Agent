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

xhost +

function platformchk()
{
	if [ -f "package_info" ]; then
		source ${CURDIR}/package_info
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

function cma_script()
{
	STATUS=$1
	SERVICE_NAME=$2
	SERVICE_SYSTEMD="$SERVICE_NAME.service"
	SERVICE_UPSTART="$SERVICE_NAME.conf"
	SERVICE_SYSV="$SERVICE_NAME"
	
	LOGD_NAME="logd"
	LOGD_SYSTEMD="$LOGD_NAME.service"
	LOGD_SYSV="$LOGD_NAME"
	
	# => start stop restart status
	if [ -d "$PATH_SYSTEMD" ]; then
		if [ -n "`command -v systemctl`" ]; then
			systemctl daemon-reload
			sleep 1
			systemctl $STATUS $LOGD_SYSTEMD
			systemctl $STATUS $SERVICE_SYSTEMD
		#elif [ -n "`command -v initctl`" ]; then
		#	initctl $STATUS $SERVICE_NAME
		else
			$PATH_SYSV/$LOGD_SYSV $STATUS
			$PATH_SYSV/$SERVICE_SYSV $STATUS
		fi
	elif [ -d "$PATH_UPSTART" ]; then
		if [ -n "`command -v init-checkconf`" ]; then
			initctl $STATUS $SERVICE_NAME
		else
			$PATH_SYSV/$LOGD_SYSV $STATUS
			$PATH_SYSV/$SERVICE_SYSV $STATUS
		fi
	else
		$PATH_SYSV/$LOGD_SYSV $STATUS
		$PATH_SYSV/$SERVICE_SYSV $STATUS
	fi
}

function cma_install()
{
	SERVICE_NAME=$1
	SERVICE_SYSTEMD="$SERVICE_NAME.service"
	SERVICE_UPSTART="$SERVICE_NAME.conf"
	SERVICE_SYSV="$SERVICE_NAME"
	
	LOGD_NAME="logd"
	LOGD_SYSTEMD="$LOGD_NAME.service"
	LOGD_SYSV="$LOGD_NAME"
	
	if [ -d "$PATH_SYSTEMD" ]; then
		if [ -n "`command -v systemctl`" ]; then
			cp ./$LOGD_SYSTEMD $PATH_SYSTEMD
			chown root:root $PATH_SYSTEMD/$LOGD_SYSTEMD
			chmod 0644 $PATH_SYSTEMD/$LOGD_SYSTEMD
			cp ./$SERVICE_SYSTEMD $PATH_SYSTEMD
			chown root:root $PATH_SYSTEMD/$SERVICE_SYSTEMD
			chmod 0644 $PATH_SYSTEMD/$SERVICE_SYSTEMD
		#elif [ -n "`command -v initctl`" ]; then
		#	cp ./$SERVICE_UPSTART $PATH_UPSTART
		#	chown root:root $PATH_UPSTART/$SERVICE_UPSTART
		#	chmod 0644 $PATH_UPSTART/$SERVICE_UPSTART
		else
			cp ./$LOGD_SYSV $PATH_SYSV
			chown root:root $PATH_SYSV/$LOGD_SYSV
			chmod 0750 $PATH_SYSV/$LOGD_SYSV
			cp ./$SERVICE_SYSV $PATH_SYSV
			chown root:root $PATH_SYSV/$SERVICE_SYSV
			chmod 0750 $PATH_SYSV/$SERVICE_SYSV
		fi	
	elif [ -d "$PATH_UPSTART" ]; then
		if [ -n "`command -v init-checkconf`" ]; then
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
			ln -s $PATH_SYSTEMD/$SERVICE_SYSTEMD $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD
			ln -s $PATH_SYSTEMD_ETC/$LOGD_SYSTEMD $PATH_SYSTEMD_AUTO/$LOGD_SYSTEMD
			ln -s $PATH_SYSTEMD_ETC/$SERVICE_SYSTEMD $PATH_SYSTEMD_AUTO/$SERVICE_SYSTEMD
		#elif [ -n "`command -v initctl`" ]; then
		#	initctl check-config $SERVICE_NAME
		else
			chkconfig --del $LOGD_SYSV
			chkconfig --add $LOGD_SYSV
			chkconfig --del $SERVICE_SYSV
			chkconfig --add $SERVICE_SYSV
		fi		
	elif [ -d "$PATH_UPSTART" ]; then
		if [ -n "`command -v init-checkconf`" ]; then
			initctl check-config $SERVICE_NAME
		elif [ -n "`command -v insserv`" ]; then
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
	else
		if [ -n "`command -v insserv`" ]; then
			/sbin/insserv -r $LOGD_SYSV
			/sbin/insserv -v $PATH_SYSV/$LOGD_SYSV
			/sbin/insserv -r $SERVICE_SYSV
			/sbin/insserv -v $PATH_SYSV/$SERVICE_SYSV
		elif [ -n "`command -v update-rc.d`" ]; then
			update-rc.d -f $LOGD_SYSV remove
			update-rc.d $LOGD_SYSV defaults 99 20
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
	if [ -d "$cfgtmp" ]
	then
		echo "remove tmp dir $cfgtmp."
		rm -rf "$cfgtmp*"
	fi
}

function cfgbackup()
{
	echo "backup config files"
	
	clearcfgtmp
	cp -vf $(find $APPDIR  -maxdepth 1 -name '*.xml') "$cfgtmp"
	cp -vf $(find $APPDIR  -maxdepth 1 -name '*.ini') "$cfgtmp"
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.cfg' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.cfg') "$cfgtmp"
	echo "cp -vf find $APPDIR -maxdepth 1 -name '*.db*' $cfgtmp"
	cp -vf $(find $APPDIR -maxdepth 1 -name '*.db*') "$cfgtmp"
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
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.xml') "$APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.ini') "$APPDIR"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.cfg' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.cfg') "$APPDIR"
		echo "cp -vf find $cfgtmp  -maxdepth 1 -name '*.db' $APPDIR"
		cp -vf $(find $cfgtmp -maxdepth 1 -name '*.db') "$APPDIR"
		rm -rf "$cfgtmp"
	fi
}

#++++++++++++++++main++++++++++++++++++++++
# user check; must be root
if [ $UID -gt 0 ] &&[ "`id -un`" != "root" ]; then
	echo "You must be root !"
	exit -1
fi

platformchk

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

	cma_script stop saagent
	cfgbackup

	if  [ -f ${APP_UNINSTALL} ]; then	
		echo "Found RMM agent already installed. Remove and install the new ..."
		chmod +x "${APP_UNINSTALL}"
		${APP_UNINSTALL}
	fi
fi

# Copy Agent Service
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

# Install SAWatchdog service
echo "Install SAWatchdog service"
cma_install sawatchdog
#cp ./sawatchdog /etc/init.d
#chown root:root /etc/init.d/sawatchdog
#chmod 0750 /etc/init.d/sawatchdog
#/sbin/chkconfig --add sawatchdog

# Install SAWebsockify service
echo "Install SAWebsockify service"
cma_install sawebsockify
#cp ./sawebsockify /etc/init.d
#chown root:root /etc/init.d/sawebsockify
#chmod 0750 /etc/init.d/sawebsockify
#/sbin/chkconfig --add sawebsockify

# Install SAAgent service
echo "Install SAAgent service"
cma_install saagent
#cp ./saagent /etc/init.d
#chown root:root /etc/init.d/saagent
#chmod 0750 /etc/init.d/saagent
#/sbin/chkconfig --add saagent

if [ -d "/etc/profile.d/" ]; then
	cp ./AgentService/xhostshare.sh "/etc/profile.d/"
	chmod 0644 /etc/profile.d/xhostshare.sh
fi

# pre-install 
PRE_INSTALL_FILE="/usr/local/AgentService/pre-install_chk.sh"
if [ -f "${PRE_INSTALL_FILE}" ]; then
	chmod +x $PRE_INSTALL_FILE
	$PRE_INSTALL_FILE
fi


SETUP_FILE="/usr/local/AgentService/setup.sh"
read -t 10 -p "Setup Agent [y/n](default:n): " ans
if [ "$ans" == "y" ]; then
    chmod +x $SETUP_FILE
	$SETUP_FILE
else
	echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
	echo "Start WISE-Agent."
	# Start SAWatchdog    #echo "Start RMM SAWatchdog daemon..."
	cma_script start sawatchdog
	#/etc/init.d/sawatchdog start

	# Start WISE-Agent    #echo "Start WISE-Agent daemon..."
	cma_script start saagent
	#/etc/init.d/saagent start
		
	# Start SAWebsockify  #echo "Start RMM SAWebsockify daemon..."
	cma_script start sawebsockify
	#/etc/init.d/sawebsockify start
fi

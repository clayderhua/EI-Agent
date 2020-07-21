#!/bin/bash
echo "Install AgentService." 
CURDIR=${PWD} 
echo $CURDIR
TDIR=`mktemp -d`
cfgtmp=$TDIR/

xhost +

function platformchk()
{
	if [ -f "package_info" ]; then
		source package_info
		PLATFORM=$(lsb_release -ds | sed 's/\"//g')
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
		cp -vf $(find $cfgtmp  -maxdepth 1 -name '*.xml') "$APPDIR"
		cp -vf $(find $cfgtmp  -maxdepth 1 -name '*.ini') "$APPDIR"
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
if [ -f ${INSTALL_AGENT_CONFIG} ]; then
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
cp -avr ./AgentService /usr/local

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
sudo cp ./sawatchdog  /etc/init.d
sudo chown root:root  /etc/init.d/sawatchdog
sudo chmod 0750       /etc/init.d/sawatchdog
sudo /sbin/insserv -v /etc/init.d/sawatchdog

# Install SAWebsockify service
cp ./sawebsockify /etc/init.d
chown root:root   /etc/init.d/sawebsockify
chmod 0750        /etc/init.d/sawebsockify
/sbin/insserv -v  /etc/init.d/sawebsockify

# Install SAAgent service
cp ./saagent      /etc/init.d
chown root:root   /etc/init.d/saagent
chmod 0750        /etc/init.d/saagent
/sbin/insserv -v  /etc/init.d/saagent

systemctl --system daemon-reload

find /home/ -name 'Autostart' -print0 | while IFS= read -r -d $'\0' line; do
	echo "$line"
	cp ./AgentService/xhostshare.sh "$line"
	chown --reference="$line" "$line/xhostshare.sh"
	chmod 755 "$line/xhostshare.sh"
done
# pre-install 
if [ -f "/usr/local/AgentService/pre-install_chk.sh" ]; then
	/usr/local/AgentService/pre-install_chk.sh
fi

read -t 10 -p "Setup Agent [y/n](default:n): " ans

if [ "$ans" == "y" ]; then
	/usr/local/AgentService/setup.sh
else
	echo "Start RMM Watchdog."
	/etc/init.d/sawatchdog start
	echo "Start RMM Websockify."
	/etc/init.d/sawebsockify start
	echo "Start WISE-Agent."
	echo "You can setup agent with command '$ sudo ./setup.sh' in '/usr/local/AgentService' later. "
	/etc/init.d/saagent start
fi

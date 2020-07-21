#!/bin/bash
SRC_ROOT=$(pwd)
OUTPUT_PATH=${SRC_ROOT}/Release
CAGENT_OUTPUT_PATH=${OUTPUT_PATH}/AgentService
DATA_SYNC_DIR=${SRC_ROOT}/Modules/DataSync
BUILD_PATH=${SRC_ROOT}/build
TEMP_RELEASE_DIR=${SRC_ROOT}/.release
CONFIG_PATH=${BUILD_PATH}/config
MODULE_CFG_PATH=${CONFIG_PATH}/module/linux
SCRIPT_PATH=${BUILD_PATH}/script
PACKED_PATH=${SRC_ROOT}/Wrapped
UPDATER_SHELL_FILE=updater.sh
TOOLS_ROOT=${BUILD_PATH}/tools

function title(){
	echo -ne "\033]0;${1}\007"
}

function clean_release()
{
	rm -rf "${OUTPUT_PATH}"
	rm -fr "${TEMP_RELEASE_DIR}"
}

function init_system_env()
{
	title "Get Platform Info"
	if [ "$RISC_TARGET" == "risc" ]; then
		platform="$RISC_TARGET"_"$OS_VERSION"
		ARCH="$CHIP_VANDER"_"$PLATFORM"
		VER="$OS_VERSION"
	elif [ "$OS_VERSION" == "container" ]; then
		platform="$(cut -d' ' -f1 <<< "$TARGET_PLATFORM_NAME")"
		ARCH="$TARGET_PLATFORM_ARCH"
		VER="$(cut -d' ' -f2 <<< "$TARGET_PLATFORM_NAME")"
	else
		if [ ! -z "${TARGET_PLATFORM_NAME}" ] && [ "${TARGET_PLATFORM_NAME}" != " " ]; then # Rename package name to match target platform
			platform=${TARGET_PLATFORM_NAME}
			VER="$(cut -d' ' -f2 <<< "$TARGET_PLATFORM_NAME")"
		else # default system ubuntu
			platform=$(lsb_release -is | sed 's/\"//g')
			VER=$(lsb_release -sr)
		fi

		if [ ! -z "${TARGET_PLATFORM_ARCH}" -a "${TARGET_PLATFORM_ARCH}" != " " ]; then
			ARCH=${TARGET_PLATFORM_ARCH}
		else
			ARCH=$(uname -m)
		fi
	fi

	SCRIPT_TARGET_PLATFORM="${platform}"
	swversion=$(cat ${SRC_ROOT}/Include/svnversion.h | egrep "\bMAIN_VERSION\b|\bSUB_VERSION\b|\bBUILD_VERSION\b|\bSVN_REVISION\b" | awk 'BEGIN { ORS="." }; {print $3}' | sed 's/\.$//')

	echo "Platform: $platform"
	echo "Architecture: $ARCH"
	echo "OS Version: $VER"
	echo "SW Version: ${swversion}"
}

function copy_config_file()
{
	mkdir -p ${CAGENT_OUTPUT_PATH}
	mkdir -p ${CAGENT_OUTPUT_PATH}/module

	# copy ini
	cp -af "${DATA_SYNC_DIR}/DataSync.ini" "${CAGENT_OUTPUT_PATH}/"

	# module_config.xml
	if [ -f "${MODULE_CFG_PATH}/module_config${TARGET_DEVICE}.xml" ]; then
		cp -af "${MODULE_CFG_PATH}/module_config${TARGET_DEVICE}.xml" "${CAGENT_OUTPUT_PATH}/module/module_config.xml"
	else
		cp -af "${MODULE_CFG_PATH}/module_config.xml" "${CAGENT_OUTPUT_PATH}/module/module_config.xml"
	fi

	# SAWatchdog_Config
	echo "ProcName=cagent;CommID=1;StartupCmdLine=saagent" > "${CAGENT_OUTPUT_PATH}/SAWatchdog_Config"
	echo "ProcName=websockify;CommID=2;StartupCmdLine=sawebsockify" >> "${CAGENT_OUTPUT_PATH}/SAWatchdog_Config"

	# change mod
	chmod 755 $(find ${SCRIPT_PATH} -name '*.sh')
	chmod 755 $(find ${SCRIPT_PATH} -name 'saagent')
	chmod 755 $(find ${SCRIPT_PATH} -name 'sawebsockify')

	# copy script to CAGENT_OUTPUT_PATH
	script_list=(uninstall.sh \
				 setup.sh \
				 netInfo.sh \
				 xhostshare.sh \
				 servicectl.sh \
				 McAfeeAddUpdater.sh \
				 pre-install_chk.sh \
				)
	for item in "${script_list[@]}"; do
		cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${item}" "${CAGENT_OUTPUT_PATH}/"
	done

	# copy script to OUTPUT_PATH
	script_list=(${UPDATER_SHELL_FILE} \
				 saagent \
				 saagent.service \
				 saagent.conf \
				 sawebsockify \
				 sawebsockify.conf \
				 sawebsockify.service \
				 sawatchdog \
				 sawatchdog.conf \
				 sawatchdog.service \
				 logd.service \
				)
	for item in "${script_list[@]}"; do
		cp -af "${SCRIPT_PATH}/${SCRIPT_TARGET_PLATFORM}/${item}" "${OUTPUT_PATH}/"
	done

	# copy doc file
	mkdir -p "${CAGENT_OUTPUT_PATH}/doc"
	cp -af ${BUILD_PATH}/doc/${SCRIPT_TARGET_PLATFORM}/* "${CAGENT_OUTPUT_PATH}/doc/"
	find "${BUILD_PATH}/doc" -maxdepth 1 -type f | xargs -n1 -I{} cp -af {} "${CAGENT_OUTPUT_PATH}/doc/"

	# generate package_info
	echo "ORIG_ARCH=\"$ARCH\"" > "${OUTPUT_PATH}/package_info"
	echo "ORIG_PLATFORM=\"$platform $VER\"" >> "${OUTPUT_PATH}/package_info"
}

function new_agent_config()
{
	cp "${CONFIG_PATH}/agent_config.xml" "${CAGENT_OUTPUT_PATH}/agent_config.xml"

	# update swVersion
	sed -i -E "s|<SWVersion>.*</SWVersion>|<SWVersion>${swversion}</SWVersion>|" "${CAGENT_OUTPUT_PATH}/agent_config.xml"

	# update os
	osversion="$platform $VER"
	ret=$(grep "<osVersion>" "${CAGENT_OUTPUT_PATH}/agent_config.xml")
	if [[ "${ret}" == "" ]]; then # insert osVersion
		sed -i "s|</Profiles>|  <osVersion>${osversion}</osVersion>\n  </Profiles>|" "${CAGENT_OUTPUT_PATH}/agent_config.xml"
		#echo "Insert <osVersion> ${osversion}"
	else # update osVersion
		sed -i -E "s|<osVersion>.*</osVersion>|<osVersion>${osversion}</osVersion>|" "${CAGENT_OUTPUT_PATH}/agent_config.xml"
		#echo "Update <osVersion> $platform $VER"
	fi

	# update arch
	ret=$(grep "<osArch>" ${CAGENT_OUTPUT_PATH}/agent_config.xml)
	if [[ "${ret}" == "" ]]; then # insert Arch
		sed -i "s|</Profiles>|  <osArch>${ARCH}</osArch>\n  </Profiles>|" "${CAGENT_OUTPUT_PATH}/agent_config.xml"
		#echo "Insert <osArch> ${ARCH}"
	else # update Arch
		sed -i -E "s|<osArch>.*</osArch>|<osArch>${ARCH}</osArch>|" "${CAGENT_OUTPUT_PATH}/agent_config.xml"
		#echo "Update <osArch> ${ARCH}"
	fi

	cp -af "${CAGENT_OUTPUT_PATH}/agent_config.xml" "${CAGENT_OUTPUT_PATH}/agent_config_def.xml"
}

function copy_prebuilt()
{
	title "Copy 3rd party library"
	PREBUILD_PLUGIN="${SCRIPT_TARGET_PLATFORM}-${VER}-${ARCH}"
	if [ -d "${SRC_ROOT}/PreBuildModules/${PREBUILD_PLUGIN}" ]; then
		cp -avf ${SRC_ROOT}/PreBuildModules/${PREBUILD_PLUGIN}/* "${CAGENT_OUTPUT_PATH}/"
		echo "Load 3rdParty Plugin"
	fi
}

function install_all()
{
	cd ${SRC_ROOT}
	mkdir ${TEMP_RELEASE_DIR}
	make install DESTDIR=${TEMP_RELEASE_DIR} || exit 1

	mkdir -p "${CAGENT_OUTPUT_PATH}"
	# mv specified binary first

	# mv VNC
	mkdir -p ${CAGENT_OUTPUT_PATH}/VNC
	mv ${TEMP_RELEASE_DIR}/usr/local/bin/x11vnc "${CAGENT_OUTPUT_PATH}/VNC/"
	mv ${TEMP_RELEASE_DIR}/usr/local/bin/websockify "${CAGENT_OUTPUT_PATH}/VNC/"

	# mv smartctl
	mv ${TEMP_RELEASE_DIR}/usr/local/sbin/smartctl "${CAGENT_OUTPUT_PATH}/"

	# mv common library
	mv ${TEMP_RELEASE_DIR}/usr/local/lib/lib*.so* "${CAGENT_OUTPUT_PATH}"
	mv ${TEMP_RELEASE_DIR}/usr/local/lib/*.so* "${CAGENT_OUTPUT_PATH}/module"

	# mv specified binary
	bin_list=(AgentEncrypt \
			  cagent \
			  CredentialChecker \
			  ModuleMerge \
			  ScreenshotHelper \
			  sawatchdog \
			  logd \
			  )
	for item in "${bin_list[@]}"; do
		mv ${TEMP_RELEASE_DIR}/usr/local/bin/${item} "${CAGENT_OUTPUT_PATH}"
	done

	#rm -fr ${TEMP_RELEASE_DIR}
}

clean_release

init_system_env
copy_config_file
new_agent_config
copy_prebuilt
install_all

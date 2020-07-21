#!/bin/bash
set -x
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT=$(pwd)

function create_sfx()
{
	if [ ! -z "${TARGET_PLATFORM_ARCH}" -a "${TARGET_PLATFORM_ARCH}" != " " ]; then
		ARCH=${TARGET_PLATFORM_ARCH}
	fi

	PACKED_FILE_NAME="wise-agent-${sfx_platform} ${ARCH}-${VERSION}.run"
	echo "File Name: $PACKED_FILE_NAME"

	#change makeself tool for x86_Yocto
	MAKESELF_TOOL_PATH=${TOOLS_ROOT}/makeself-2.2.0
	
	# *.run will generate at OTAWrapper
	cd ${REPO_ROOT}/build/tools/OTAPackage/Ubuntu-16.04/OTAWrapper
	rm *.run
	"${MAKESELF_TOOL_PATH}/makeself.sh" --nox11 ${MAKESELF_ARGS} "${REPO_ROOT}/Release" "${PACKED_FILE_NAME}" "The Installer for Wise-Agent" "./updater.sh"
	cd -
}

function create_ota_package()
{
	mkdir -p "${REPO_ROOT}/Wrapped"
	rm -f ${REPO_ROOT}/Wrapped/*.zip
	rm -f ${REPO_ROOT}/Wrapped/*.tag
	rm -f ${REPO_ROOT}/Wrapped/*.run

	# cd OTAPackage to reference wise library
	cd ${REPO_ROOT}/build/tools/OTAPackage/Ubuntu-16.04
	${REPO_ROOT}/build/tools/OTAPackage/Ubuntu-16.04/otapackager-cli-x86_64 \
				-n UbuntuWISEAgentSetup \
				-v ${VERSION}.0 \
				-i ${REPO_ROOT}/build/tools/OTAPackage/Ubuntu-16.04/OTAWrapper \
				-b deploy.sh \
				-c checkfile.sh \
				-d ${REPO_ROOT}/Wrapped \
				-g ${OTA_TAGS}
	cd -
}

# $1: local filename
function ftp_upload()
{
	if [ ! -f "$1" ]; then
		return
	fi
	username=$(urlencode ${FTP_USER_NAME})
	password=$(urlencode ${FTP_PASSWORD})

	curl -v -T "$1" ftp://${username}:${password}@${FTP_SERVER}/${FTP_PATH}/
}

function upload_ota_package()
{
	local files=$(find "${REPO_ROOT}/Wrapped" -iname "*.zip")
	for file in ${files}; do
		echo "ftp upload [${file}]"
		ftp_upload "${file}"
	done
}

function init_system_env()
{
	TOOLS_ROOT=${REPO_ROOT}/build/tools
	
	VERSION=$(cat ${REPO_ROOT}/VERSION)
	sfx_platform="Ubuntu 18.04"
	ARCH=x86_64
	OTA_TAGS=Ubuntu18.04,x64
	
	FTP_USER_NAME=admin
	FTP_PASSWORD=sa30Admin
	FTP_SERVER='edgesense4.wise-paas.com:2121'
	FTP_PATH=
}

init_system_env
create_sfx
create_ota_package
upload_ota_package

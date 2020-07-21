#!/bin/bash

COLOR_REST='\e[0m'
COLOR_GREEN='\e[0;32m';
COLOR_RED='\e[0;31m';
COLOR_YELLOW='\e[1;33m';

MODULE="wise-agent"
PUSH_GIT=0

cd ..
SRC_ROOT=$(pwd)

function print_green()
{
	echo -e "${COLOR_GREEN}$1${COLOR_REST}"
}

function print_yellow()
{
	echo -e "${COLOR_YELLOW}$1${COLOR_REST}"
}

function print_red()
{
	echo -e "${COLOR_RED}$1${COLOR_REST}"
}

function print_usage()
{
	print_green "Usage:"
	print_green "\t$0 <option> <agent_version>"
	print_green "Option:"
	print_green "\t-p [Platform version]"
	print_green "\t-c [EI-Connect version]"
	print_green "\t-e [Edge-Connect version|auto]"
	print_green "\t-o [OTA version]"
	print_green "\t-l [LocalProvision version]"
	print_green "\t-r [RuleEngine version]"
	print_green "\t-s silent, default push changes"
	print_green "\t-n no version changed"
	print_green "Note:"
	print_green "1) All the version is combine from 3 numbers, 'main.sub.build'"
	print_green "2) agent_version will increase by 1 if no agent_version input."
	print_green "3) Edge-Connect version will increase by 1 automatically if no version input."
}

# $1: version name
# $2: version number
function valid_version()
{
	if [[ "$2" == "" ]]; then
		return
	fi

	if [[ "$2" != "auto" ]]; then
		local version_main=$(echo "${2}" | awk -F "." '{print $1}')
		local version_sub=$(echo "${2}" | awk -F "." '{print $2}')
		local version_build=$(echo "${2}" | awk -F "." '{print $3}')
		
		if [[ ${version_main} == "" ]] || [[ ${version_sub} == "" ]] || [[ ${version_build} == "" ]]; then
			print_red "Invalid version number, ${1}: ${2}"
			exit 1
		fi
	fi
	print_green "${1}: ${2}"
}

function confirm_version()
{
	print_green "========== confirm version =========="
	valid_version "AGENT_VERSION" "${AGENT_VERSION}"
	valid_version "PLATFORM_VERSION" "${PLATFORM_VERSION}"
	valid_version "EI_CONNECT_VERSION" "${EI_CONNECT_VERSION}"
	valid_version "EDGE_CONNECT_VERSION" "${EDGE_CONNECT_VERSION}"
	valid_version "FEATURE_PLUGIN_VERSION" "${FEATURE_PLUGIN_VERSION}"
	valid_version "OTA_VERSION" "${OTA_VERSION}"
	valid_version "LOCAL_PROVISION_VERSION" "${LOCAL_PROVISION_VERSION}"
	valid_version "RULE_ENGINE_VERSION" "${RULE_ENGINE_VERSION}"
	print_green "====================================="
	
	if ((SILENT_GIT_PUSH == 1)); then
		return
	fi
	
	print_yellow "Confirm the version(y/N)?"
	read message || exit 1
	if [[ "$message" == "y"* ]] || [[ "$message" == "Y"* ]]; then
		return
	else
		print_green "Abort because of version is not confirmed"
		exit 0
	fi
}

# return version+1
# $1: version
function increase_version()
{
	local version_main=$(echo "${1}" | awk -F "." '{print $1}')
	local version_sub=$(echo "${1}" | awk -F "." '{print $2}')
	local version_build=$(echo "${1}" | awk -F "." '{print $3}')
	version_build=$((version_build+1))
	echo "${version_main}.${version_sub}.${version_build}"
}


function arg_parser()
{
	options='p:c:e:f:o:l:r:hsn'
	while getopts $options option
	do
		case "$option" in
			p ) PLATFORM_VERSION="$OPTARG" ;;
			c ) EI_CONNECT_VERSION="$OPTARG" ;;
			e ) EDGE_CONNECT_VERSION="$OPTARG" ;;
			f ) FEATURE_PLUGIN_VERSION="$OPTARG" ;;
			o ) OTA_VERSION="$OPTARG" ;;
			l ) LOCAL_PROVISION_VERSION="$OPTARG" ;;
			r ) RULE_ENGINE_VERSION="$OPTARG" ;;
			s )
				SILENT_GIT_PUSH=1
				PUSH_GIT=1
				;;
			n )
				NO_VERSION_CHANGE=1
				;;
			h ) 
				print_usage
				exit 0
				;;
			\? ) echo "Unknown option: -$OPTARG" >&2; exit 1;;
			: ) echo "Missing option argument for -$OPTARG" >&2; exit 1;;
			* ) echo "Unimplemented option: -$OPTARG" >&2; exit 1;;
		esac
	done

	#if ((OPTIND == 1)); then
	#	echo "No options specified"
	#fi

	shift $((OPTIND - 1))

	if (($# == 0)); then
		# increase agent version by 1
		local version=$(cat ${SRC_ROOT}/Application/CAgent/VERSION)
		if (($NO_VERSION_CHANGE == 1)); then
			AGENT_VERSION=${version}
		else
			AGENT_VERSION=$(increase_version ${version})
		fi
	else
		if (($NO_VERSION_CHANGE == 1)); then
			print_red "coudln't input version with '-n' option!"
			exit 1
		fi
		AGENT_VERSION=$1
	fi
}

function checkout_master()
{
	local branch=$(git branch | grep "* master")
	if [ "${branch}" == "" ]; then
		git checkout master
		git pull
	fi
}

# $1: version string
# $2: file
function write_svnversion()
{
	local version_main=$(echo "${1}" | awk -F "." '{print $1}')
	local version_sub=$(echo "${1}" | awk -F "." '{print $2}')
	local version_build=$(echo "${1}" | awk -F "." '{print $3}')

	sed -i -e "s/define MAIN_VERSION.*/define MAIN_VERSION ${version_main}/" \
		   -e "s/define SUB_VERSION.*/define SUB_VERSION ${version_sub}/" \
		   -e "s/define BUILD_VERSION.*/define BUILD_VERSION ${version_build}/" \
		   "$2"
}

function write_svnversion2()
{
	local version_main=$(echo "${1}" | awk -F "." '{print $1}')
	local version_sub=$(echo "${1}" | awk -F "." '{print $2}')
	local version_build=$(echo "${1}" | awk -F "." '{print $3}')

	sed -i -e "s/define VER_MAJOR.*/define VER_MAJOR ${version_main}/" \
		   -e "s/define VER_MINOR.*/define VER_MINOR ${version_sub}/" \
		   -e "s/define VER_BUILD.*/define VER_BUILD ${version_build}/" \
		   "$2"
}

# $1: psuh module
# $2: version
function git_push_master()
{
	if ((PUSH_GIT != 0)); then
		print_green "git push $(pwd)"
		git push origin master
		if [[ "${2}" != "" ]]; then
			git tag ${2}
			git push origin ${2}
		fi
	fi
}

function change_platform_version()
{
	if [[ "${PLATFORM_VERSION}" == "" ]]; then
		return
	fi

	# change version
	echo "${PLATFORM_VERSION}" > "${SRC_ROOT}/Platform/VERSION"
	write_svnversion ${PLATFORM_VERSION} "${SRC_ROOT}/Platform/svnversion.h"
	
	# checkout master
	cd "${SRC_ROOT}/Platform"
	checkout_master
	
	# push to master
	git add VERSION
	git add svnversion.h
	git status | grep modified | awk '{print $2}' | egrep "VERSION|svnversion.h"
	if (($? == 0)); then
		git commit -m "Update version to ${PLATFORM_VERSION}"
		git_push_master ${PLATFORM_VERSION}
	fi
}

function change_ei_connect_version()
{
	if [[ "${EI_CONNECT_VERSION}" == "" ]]; then
		return
	fi

	# change version
	echo "${EI_CONNECT_VERSION}" > "${SRC_ROOT}/Lib_EI/Include/VERSION"
	write_svnversion ${EI_CONNECT_VERSION} "${SRC_ROOT}/Lib_EI/Include/svnversion.h"

	# checkout master
	cd "${SRC_ROOT}/Lib_EI/Include"
	checkout_master
	
	# push to master
	git add VERSION
	git add svnversion.h
	git status | grep modified | awk '{print $2}' | egrep "VERSION|svnversion.h"
	if (($? == 0)); then
		git commit -m "Update version to ${EI_CONNECT_VERSION}"
		git_push_master ${EI_CONNECT_VERSION}
	fi
}

function change_edge_connect_version()
{
	if [[ "${EDGE_CONNECT_VERSION}" == "" ]] || [[ "${EDGE_CONNECT_VERSION}" == "auto" ]]; then
		return
	fi

	# change version
	echo "${EDGE_CONNECT_VERSION}" > "${SRC_ROOT}/Lib_SRP/Include/VERSION"
	write_svnversion ${EDGE_CONNECT_VERSION} "${SRC_ROOT}/Lib_SRP/Include/svnversion.h"

	# checkout master
	cd "${SRC_ROOT}/Lib_SRP/Include"
	checkout_master
	
	# push to master
	git add VERSION
	git add svnversion.h
	git status | grep modified | awk '{print $2}' | egrep "VERSION|svnversion.h"
	if (($? == 0)); then
		git commit -m "Update version to ${EDGE_CONNECT_VERSION}"
		git_push_master ${EDGE_CONNECT_VERSION}
	fi
}

function change_ota_version()
{
	if [[ "${OTA_VERSION}" == "" ]]; then
		return
	fi

	# change Lib_OTA version
	write_svnversion ${OTA_VERSION} "${SRC_ROOT}/Lib_OTA/filetransferlib/version.h"
	write_svnversion ${OTA_VERSION} "${SRC_ROOT}/Lib_OTA/sueclient/version.h"
	write_svnversion ${OTA_VERSION} "${SRC_ROOT}/Lib_OTA/sueclientcore/version.h"
	write_svnversion ${OTA_VERSION} "${SRC_ROOT}/Lib_OTA/miniunziplib/version.h"

	# checkout master
	cd "${SRC_ROOT}/Lib_OTA"
	checkout_master
	
	# push to master
	git add "filetransferlib/version.h"
	git add "sueclient/version.h"
	git add "sueclientcore/version.h"
	git add "miniunziplib/version.h"
	git status | grep modified | awk '{print $2}' | grep "version.h"
	if (($? == 0)); then
		git commit -m "Update version to ${OTA_VERSION}"
		git_push_master ${OTA_VERSION}
	fi
	
	# change OTAHandler version
	write_svnversion ${OTA_VERSION} "${SRC_ROOT}/Modules/OTAHandler/version.h"
	
	# checkout master
	cd "${SRC_ROOT}/Modules/OTAHandler"
	checkout_master
	
	# push to master
	git add version.h
	git status | grep modified | awk '{print $2}' | grep "version.h"
	if (($? == 0)); then
		git commit -m "Update version to ${OTA_VERSION}"
		git_push_master ${OTA_VERSION}
	fi
}

function change_local_provision_version()
{
	if [[ "${LOCAL_PROVISION_VERSION}" == "" ]]; then
		return
	fi

	# change version
	write_svnversion ${LOCAL_PROVISION_VERSION} "${SRC_ROOT}/Modules/LocalProvision/version.h"

	# checkout master
	cd "${SRC_ROOT}/Modules/LocalProvision"
	checkout_master
	
	# push to master
	git add version.h
	git status | grep modified | awk '{print $2}' | grep "version.h"
	if (($? == 0)); then
		git commit -m "Update version to ${LOCAL_PROVISION_VERSION}"
		git_push_master ${LOCAL_PROVISION_VERSION}
	fi
}

function change_rule_engine_version()
{
	if [[ "${RULE_ENGINE_VERSION}" == "" ]]; then
		return
	fi

	# change version
	echo "${RULE_ENGINE_VERSION}.0" > "${SRC_ROOT}/Modules/RuleEngine/VERSION"
	write_svnversion2 ${RULE_ENGINE_VERSION} "${SRC_ROOT}/Modules/RuleEngine/version.h"

	# checkout master
	cd "${SRC_ROOT}/Modules/RuleEngine"
	checkout_master
	
	# push to master
	git add VERSION
	git add version.h
	git status | grep modified | awk '{print $2}' | egrep "VERSION|version.h"
	if (($? == 0)); then
		git commit -m "Update version to ${RULE_ENGINE_VERSION}"
		git_push_master ${RULE_ENGINE_VERSION}
	fi
}

function change_agent_version()
{
	if [[ "${AGENT_VERSION}" == "" ]]; then
		return
	fi

	# change version
	echo "${AGENT_VERSION}" > "${SRC_ROOT}/Application/CAgent/VERSION"
	write_svnversion ${AGENT_VERSION} "${SRC_ROOT}/Application/CAgent/svnversion.h"

	# checkout master
	cd "${SRC_ROOT}/Application/CAgent"
	checkout_master
	
	# push to master
	git add VERSION
	git add svnversion.h
	git status | grep modified | awk '{print $2}' | egrep "VERSION|svnversion.h"
	if (($? == 0)); then
		git commit -m "Update version to ${AGENT_VERSION}"
		git_push_master ${AGENT_VERSION}
	fi
}

# $1: version
function snapshot_xml()
{
	local xml_file=${MODULE}-v${1}.xml

	cd ${SRC_ROOT}
	repo manifest -r -o ${xml_file}

	cp ${SRC_ROOT}/${xml_file} ${SRC_ROOT}/.repo/manifests/${MODULE}
	cd ${SRC_ROOT}/.repo/manifests/${MODULE}
	checkout_master

	git add ${xml_file}
	git status | egrep "modified|new file" | awk -F ":" '{print $2}' | grep "${xml_file}"
	if (($? == 0)); then
		git commit -m "Update ${MODULE}/${xml_file}"
		git_push_master
	fi
}

# check if we have new version of this repo
function check_update()
{
	# get latest xml
	latest_xml=$(ls ${SRC_ROOT}/.repo/manifests/${MODULE} | sort --version-sort | awk 'END{print}')
	print_green "Latest xml is '${latest_xml}'"

	# get different
	local diff=$(repo diffmanifests --raw ${MODULE}/${latest_xml})
	if [[ "${diff}" == "" ]]; then
		print_yellow "Project has no changed, do you realy want to release (y/N)?"
		read message || exit 1
		if [[ "$message" != "y"* ]] && [[ "$message" != "Y"* ]]; then
			print_green "Abort because of no project changed"
			exit 0
		fi
	else
		# print repo pretty
		repo diffmanifests ${MODULE}/${latest_xml}
	fi
	
	# check edge-connect auto
	if [[ "${EDGE_CONNECT_VERSION}" == "auto" ]]; then
		echo "${diff}" | grep "C Lib_SRP"
		if (($? == 0)); then
			# increase edge-connect version by 1
			local version=$(cat ${SRC_ROOT}/Lib_SRP/Include/VERSION)
			if (($NO_VERSION_CHANGE == 1)); then
				EDGE_CONNECT_VERSION=${version}
			else
				EDGE_CONNECT_VERSION=$(increase_version ${version})
			fi
			print_green "Change edge-connect version to ${EDGE_CONNECT_VERSION}"
		fi
	fi
}

function init_env()
{
	PUSH_GIT=1
	SILENT_GIT_PUSH=0
	EDGE_CONNECT_VERSION="auto"
	NO_VERSION_CHANGE=0
}

init_env
arg_parser "$@"
confirm_version

if ((SILENT_GIT_PUSH == 0)); then
	check_update
fi

if ((NO_VERSION_CHANGE == 0)); then
	change_platform_version
	change_ei_connect_version
	change_edge_connect_version
	change_ota_version
	change_local_provision_version
	change_rule_engine_version
	change_agent_version
fi

snapshot_xml ${AGENT_VERSION}

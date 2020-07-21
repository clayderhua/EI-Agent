#!/bin/bash
cd ..
SRC_ROOT=$(pwd)

MODULE=wise-agent

# $1: svn version file
function show_svn()
{
	local str=$(cat $1)
	local version_main=$(echo "${str}" | grep MAIN_VERSION | awk '{print $3}')
	local version_sub=$(echo "${str}" | grep SUB_VERSION | awk '{print $3}')
	local version_build=$(echo "${str}" | grep BUILD_VERSION | awk '{print $3}')
	echo "${version_main}.${version_sub}.${version_build}"
}

function show_version()
{
	local feature_plugin_version=$(ls ${SRC_ROOT}/.repo/manifests/srp-plugin | sort --version-sort | awk 'END{print}' | egrep -o "[[:digit:]]+.[[:digit:]]+.[[:digit:]]+")

	echo "Version:"
	echo -e "\tAgent:\t\t" $(cat Application/CAgent/VERSION)
	echo -e "\tPlatform:\t" $(cat Platform/VERSION)
	echo -e "\tEI-Connect:\t" $(cat Lib_EI/Include/VERSION)
	echo -e "\tEdge-Connect:\t" $(cat Lib_SRP/Include/VERSION)
	echo -e "\tFeature-Plugin:\t" ${feature_plugin_version}
	echo -e "\tOTA:\t\t" $(show_svn "Modules/OTAHandler/version.h")
	echo -e "\tLocalProvision:\t" $(show_svn "Modules/LocalProvision/version.h")
	echo -e "\tRuleEngine:\t" $(cat Modules/RuleEngine/VERSION)
	echo -e "\tHDDPMQ:\t\t" $(show_svn "Modules/HDDPMQ/pmqversion.h")
	
}

function show_different()
{
	# get latest xml
	local latest_xml=$(ls ${SRC_ROOT}/.repo/manifests/${MODULE} | sort --version-sort | awk 'END{print}')
	echo "Latest xml is '${latest_xml}'"

	# get different
	repo diffmanifests ${MODULE}/${latest_xml}
}

show_version

echo -e "\nshow different? (Y/n)"
read message || exit 1
if [[ "$message" == "n"* ]] || [[ "$message" == "N"* ]]; then
	exit 0
fi
show_different
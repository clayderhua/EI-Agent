#!/bin/bash 

#	This is a build control script for Android.
#
#	You should install the latest Android NDK before run to build.
#	You need to configure the configurable variables!

# Current path
CUR_DIR=`dirname $(readlink -f ${BASH_SOURCE[0]})`

##########################
# Configurable Variables #
##########################
MY_PROJECT_PATH=${CUR_DIR}
DEBUG_LEVEL=1

ndk_env_test()
{
	test_res=`${BUILD_CMD} -v 2>&1 | grep 'command not found'`
	if [ -n "$test_res" ]; then 
		echo "Can't found 'ndk-build' command!"
		exit 1
	fi
}

usage()
{
	echo -e "Android build usage:\n"
	echo -e "./android_build.sh [\033[4mOPTIONS\033[24m]\n"
	echo -e "\t-c, --clean\tlike make clean\n"
	echo -e "\t-d, --debug\tbuild debug version\n"
	echo -e "\t-b, --build\tbuild release version, this is default option\n"
	echo -e "\t-h, --help\tusage\n"
}


arg_parser()
{
	local clean_flag=0
	local build_flag=0
	local debug_flag=0

	while [ $# -gt 0 ]; do 
		key="$1"
		case $key in 
			-c|--clean)
				clean_flag=1
				shift
				;;
			-d|--debug)
				debug_flag=1
				build_flag=1
				shift
				;;
			-b|--build)
				build_flag=1
				shift
				;;
			-h|--help)
				usage
				exit 0
				;;
			*)
				echo  "syntax error!"
				echo  "try '-h' or '--help' for more infomation."
				exit 2
				;;
		esac
	done

	export CLEAN=$clean_flag
	export BUILD=$build_flag
	export DEBUG=$debug_flag
}

############## main ###############

# Cmd line
BUILD_CMD=ndk-build
MY_ANDROID_BUILD="${BUILD_CMD} \
	NDK_PROJECT_PATH=./ \
	NDK_APPLICATION_MK=./Application.mk"

# 'ndk-build' command check
ndk_env_test

# arguments parser
if [ $# -eq 0 ]; then 
	arg_parser "-b"
else 
	arg_parser $*
fi 

echo "CLEAN: $CLEAN"
echo "DEBUG: $DEBUG"
echo "BUILD: $BUILD"

# Configure 
chmod +x ${CUR_DIR}/config.sh
${CUR_DIR}/config.sh

# Change work dirctory, be careful
cd "${MY_PROJECT_PATH}"

# Clean 
if [ $CLEAN -eq 1 ]; then 
	${MY_ANDROID_BUILD} clean
	echo "Remove 'libs' and 'obj' folder."
	rm -rvf ./libs ./obj
fi

# Debug
if [ $DEBUG -eq 1 ]; then 
	MY_ANDROID_BUILD="${MY_ANDROID_BUILD} NDK_DEBUG=${DEBUG_LEVEL}"
fi

# Build
if [ $BUILD -eq 1 ]; then
	${MY_ANDROID_BUILD}
fi

exit 0



#!/bin/bash 

#	This is a build control script for Android.
#	You should install the latest Android NDK before run to build.

# User configure vars
MY_PROJECT_PATH=..

# Cmd line
DEBUG_LEVEL=1
BUILD_CMD=ndk-build
ANDROID_TARGET_ABI=armeabi-v7a

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


CLEAN=0
BUILD=0
DEBUG=0
arg_parser()
{	
	while getopts "d:b:c:h" opt
	do
		case "$opt" in
		  c ) CLEAN=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  b ) BUILD=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  d ) DEBUG=1 ANDROID_TARGET_ABI="$OPTARG" ;;
		  h ) HELP=1 usage ;; # Print helpFunction in case parameter is non-existent
		  * )
			echo  "syntax error!"
			echo  "try '-h' or '--help' for more infomation."
			exit 2
			;;
		esac
	done
}

############## main ###############
# 'ndk-build' command check
ndk_env_test

# arguments parser
if [ $# -eq 0 ]; then 
	arg_parser "-b"
else 
	arg_parser "$@"
fi 

echo "CLEAN: $CLEAN"
echo "DEBUG: $DEBUG"
echo "BUILD: $BUILD"

# Change work dirctory, be careful
cd "${MY_PROJECT_PATH}"

MY_ANDROID_BUILD="${BUILD_CMD} \
	APP_ABI=${ANDROID_TARGET_ABI} \
	NDK_PROJECT_PATH=./ \
	NDK_APPLICATION_MK=./Application.mk"

# Clean 
if [ $CLEAN -eq 1 ]; then 
	${MY_ANDROID_BUILD} clean
	echo "Remove 'libs' and 'obj' folder."
	rm -rvf ./libs ./obj
fi

# Debug
if [ $DEBUG -eq 1 ]; then 
	#MY_ANDROID_BUILD="${MY_ANDROID_BUILD} NDK_DEBUG=${DEBUG_LEVEL}"
	MY_ANDROID_BUILD="${MY_ANDROID_BUILD} -d "
fi

# Build
if [ $BUILD -eq 1 ]; then
	${MY_ANDROID_BUILD}
fi

exit 0



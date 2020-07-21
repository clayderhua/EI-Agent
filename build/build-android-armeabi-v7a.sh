cd ..
SRC_ROOT=$(pwd)
PACKED_PATH=${SRC_ROOT}/Wrapped

abi=armeabi-v7a

function init_system_env()
{
	swversion=$(cat ${SRC_ROOT}/Include/svnversion.h | egrep "\bMAIN_VERSION\b|\bSUB_VERSION\b|\bBUILD_VERSION\b|\bSVN_REVISION\b" | awk 'BEGIN { ORS="." }; {print $3}' | sed 's/\.$//')
}

function build_android()
{
	cd ${SRC_ROOT}/AndroidLib/lib_src/src_build
	./android_lib_build.sh -b $abi
	./android_lib_copy_to_AndroidLib.sh
}

function build_agent()
{
	cd ${SRC_ROOT}/build
	./android_build.sh -b $abi
}

function install_all()
{
	rm -fr ${SRC_ROOT}/Release
	mv ${SRC_ROOT}/obj/local/${abi}/ ${SRC_ROOT}/Release
	rm ${SRC_ROOT}/Release/*.a
	rm -fr ${SRC_ROOT}/Release/objs
	rm -fr ${SRC_ROOT}/obj
}

function create_sfx()
{
	cd ${SRC_ROOT}
	mkdir -p "${SRC_ROOT}/Wrapped"
	tar -zcvf "${SRC_ROOT}/Wrapped/wise-agent-android-${abi}-${swversion}.tar.gz" Release
}

init_system_env
build_android
build_agent
install_all
create_sfx

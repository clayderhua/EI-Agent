#!/bin/bash
EPACK_PATH=../../ePack
source ePack_custom.bash

if [ -z "${EPACK_PATH}" ]; then
    echo "Please define ePack path"
    exit 1
fi

echo "PROJECT NAME is $PROJECT_NAME"

echo "clean last data in ePack"
rm -rf ${EPACK_PATH}/archive

mkdir -p ${EPACK_PATH}/archive/rootfs/usr/local/EdgeSense/$PROJECT_NAME || exit 1
mkdir -p ${EPACK_PATH}/archive/rootfs/etc/systemd/system || exit 1
rm -rf  ${EPACK_PATH}/archive/rootfs/usr/local/EdgeSense/$PROJECT_NAME/* || exit 1

cp -f ePack_custom.bash ${EPACK_PATH}/archive/rootfs/usr/local/EdgeSense/${PROJECT_NAME}/package.info || exit 1
cp -rf ../${PROJECT_NAME}/* ${EPACK_PATH}/archive/rootfs/usr/local/EdgeSense/$PROJECT_NAME || exit 1
cp -f ${PROJECT_NAME}.service ${EPACK_PATH}/archive/rootfs/etc/systemd/system || exit 1
chmod 664 ${EPACK_PATH}/archive/rootfs/etc/systemd/system/"${PROJECT_NAME}.service" || exit 1

cp -f ePack_custom.bash ${EPACK_PATH}/archive || exit 1
cp -f startup_custom.bash ${EPACK_PATH}/archive || exit 1

#make install DESTDIR=${EPACK_PATH}/archive/rootfs || exit 1

cd ${EPACK_PATH} || exit 1
./build.bash

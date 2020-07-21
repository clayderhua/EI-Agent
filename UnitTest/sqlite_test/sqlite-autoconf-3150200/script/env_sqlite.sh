#!/bin/bash
source ../../../../risc-env fsl imx6 yocto
export TARGET_PLATFORM_NAME="Poky (Yocto Project Reference Distro) 1.5.3"
export TARGET_PLATFORM_ARCH="armv7l"
./build_sqlite.sh '' 'wise-3310'



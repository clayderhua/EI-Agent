#!/bin/bash

source ../build-env/risc-env wise openwrt yocto 1.0.0

export DO_AUTORECONF=1

export DISABLE_HDD=1
export DISABLE_X11=1
export DISABLE_MCAFEE=1
export DISABLE_ACRONIS=1

echo 'set(OPENWRT_ICONV_INC_DIR "/opt/openwrt/ipq806x/target-arm_cortex-a7_uClibc-0.9.33.2_eabi/usr/lib/libiconv-stub/include")' >> ../Library3rdParty/minizip/CMakeLists.txt
echo 'include_directories(BEFORE ${OPENWRT_ICONV_INC_DIR})' >> ../Library3rdParty/minizip/CMakeLists.txt

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh

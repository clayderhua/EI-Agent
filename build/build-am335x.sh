#!/bin/bash
SRC_ROOT=..

source ../build-env/risc-env ti am335x yocto 2.4

export DISABLE_X11=1

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

# Customize cmake
export CUST_CMAKE=1
cd ${SRC_ROOT}/Library3rdParty/minizip
cmake . -DUSE_AES=ON -DUSE_CRYPT=ON || exit 1
sed -i 's/message(FATAL_ERROR "Failed to init submodules in:/message(STATUS "Failed to init submodules in:/' zlib/tmp/zlib-gitclone.cmake
sed -i 's/message(FATAL_ERROR "Failed to update submodules in:/message(STATUS "Failed to update submodules in:/' zlib/tmp/zlib-gitclone.cmake
cd -

./build.sh


#!/bin/bash
SRC_ROOT=..

source ../build-env/risc-env ti am335x yocto 2.4

export DISABLE_X11=1

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh


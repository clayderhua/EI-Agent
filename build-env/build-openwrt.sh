#!/bin/bash

source ../build-env/risc-env wise openwrt yocto 1.0.0

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh

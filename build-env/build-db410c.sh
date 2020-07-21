#!/bin/bash
source ../build-env/risc-env qualcomm dragon yocto

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh


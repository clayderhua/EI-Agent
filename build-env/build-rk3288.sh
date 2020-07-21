#!/bin/bash
source ../build-env/risc-env rk 3288 debian

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh


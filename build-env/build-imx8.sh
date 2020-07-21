source ../build-env/risc-env fsl imx8 yocto

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh


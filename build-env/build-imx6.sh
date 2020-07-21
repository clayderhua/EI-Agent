source ../build-env/risc-env fsl imx6 yocto 2.1

export OTA_TAGS="${RISC_TARGET},${CHIP_VANDER},${PLATFORM},${OS_VERSION}"

./build.sh


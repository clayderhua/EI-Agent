#-------------------------------------
# Target Device Info
#-------------------------------------
TARGET_DEVICE=$2
echo "Target Device: $TARGET_DEVICE"
#pause "Target Device: $TARGET_DEVICE"


#-------------------------------------
# Get Platform Info
#-------------------------------------
if [ "$RISC_TARGET" == "risc" ] && [ "$OS_VERSION" == "yocto" ]; then
	platform="$RISC_TARGET"_"$OS_VERSION"
	ARCH="$CHIP_VANDER"_"$PLATFORM"
	VER="$OS_VERSION"
elif [ "$RISC_TARGET" == "x86" ] && [ "$OS_VERSION" == "yocto" ]; then
	platform="$RISC_TARGET"_"$OS_VERSION"
	ARCH="$CHIP_VANDER"_"$PLATFORM"
	VER="$OS_VERSION"
elif [ "$RISC_TARGET" == "risc" ] && [ "$OS_VERSION" == "idp" ]; then
	platform="$RISC_TARGET"_"$OS_VERSION"
	ARCH="$CHIP_VANDER"_"$PLATFORM"
	VER="$OS_VERSION"
else
	platform=$(lsb_release -ds | sed 's/\"//g')
	ARCH=$(uname -m)
	#ARCH=$(uname -m | sed 's/x86_//;s/i[3-6]86/32/')
	VER=$(lsb_release -sr)
fi

SCRIPT_TARGET_PLATFORM="windows"
case $platform in
  openSUSE* )
    SCRIPT_TARGET_PLATFORM="openSUSE"
    ;;
  CentOS* )
    SCRIPT_TARGET_PLATFORM="CentOS"
    ;;  
  Ubuntu* )
    SCRIPT_TARGET_PLATFORM="Ubuntu"
    ;;
  risc_yoct* )
    SCRIPT_TARGET_PLATFORM="RISC_Yocto"
    ;;
  x86_yoct* )
    SCRIPT_TARGET_PLATFORM="x86_Yocto"
    ;;
  risc_idp* )
    SCRIPT_TARGET_PLATFORM="idp"
    ;;      
esac

echo "Platform: $platform"
echo "Architecture: $ARCH"
echo "OS Version: $VER"


#-------------------------------------
# Configure Environment
#-------------------------------------
LIB_SQLITE_DIR=..\

if [ "${RISC_TARGET}" == "risc" ] ; then
  if [ "$CHIP_VANDER" == "fsl" ] && [ "$PLATFORM" == "imx6" ]; then
       HOST="arm-poky-linux-gnueabi"
  elif [ "$CHIP_VANDER" == "intel" ] && [ "$OS_VERSION" == "yocto" ] && [ "$PLATFORM" == "quark" ]; then
       HOST="i586-poky-linux"
  elif [ "$CHIP_VANDER" == "intel" ] && [ "$OS_VERSION" == "idp" ] && [ "$PLATFORM" == "baytrail" ]; then
       HOST="x86_64-wrs-linux"
  fi
  for d in ${LIB_SQLITE_DIR}; do (cd "$d" && autoreconf -if && ./configure --host $HOST --prefix=/usr/local); done
elif [ "${RISC_TARGET}" == "x86" ] ; then
  if [ "$CHIP_VANDER" == "intel" ] && [ "$PLATFORM" == "x86" ]; then
       HOST="x86_64-poky-linux"
  fi
  for d in ${LIB_SQLITE_DIR}; do (cd "$d" && autoreconf -if && ./configure --host $HOST --prefix=/usr/local); done
else
  for d in ${LIB_SQLITE_DIR}; do (cd "$d" && autoreconf -if && ./configure --prefix=/usr/local); done
fi


#-------------------------------------
# make sqlite
#-------------------------------------
make -C "${LIB_SQLITE_DIR}" clean
make -C "${LIB_SQLITE_DIR}"
if [ $? -eq 0 ] ; then
	echo "make succeeds"
else
	echo "make fails"
	if [ "${RISC_TARGET}" != "risc" ] ; then
		exit
	fi
fi


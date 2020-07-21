#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE

# Checks for programs.

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_SUBDIRS([Platform
                   Library3rdParty/sqlite
                   Library3rdParty/cjson
                   Library3rdParty/libmodbus
                   Library3rdParty/iniparser
                   Library3rdParty/curl
                   lib/advtools/AdvCC
                   lib/advtools/AdvCompression
                   lib/base64
                   lib/des
                   lib/ftphelper
                   lib/log
                   lib/md5
                   lib/messagegenerator
                   lib/handlerkernel
                   lib/readini
                   lib/susihelper
                   lib/xmlhelperlib
                   lib/listhelper
                   lib/queuehelper
                   lib/datatransform
                   lib/mcast-socket
                   lib/simple-tcp-socket
                   lib/aes
                   lib/credentialhelper
                   lib/advcarehelper
                   lib/sagatherinfo
                   Lib_EI
                   Lib_SRP
                   Lib_OTA
                   Application/CredentialChecker
                   Application/ModuleMerge
                   Application/AgentEncrypt
                   Application/CAgent
                   Application/Logd
                   Application/WatchdogL])
AC_CONFIG_FILES([Makefile])

# PLUGINS
AM_CONDITIONAL([WITH_PLUGINS], [test "x$DISABLE_PLUGINS" != "x1"])
if test "x$DISABLE_PLUGINS" != x1; then
    # X11
    AM_CONDITIONAL([WITH_X11], [test "x$DISABLE_X11" != "x1"])
    if test "x$DISABLE_X11" != x1; then
        AC_CONFIG_SUBDIRS([Library3rdParty/jpeg-8d1
                           Library3rdParty/x11vnc
                           Library3rdParty/websockify
                           Modules/RemoteKVMHandler
                           Modules/ScreenshotHandler
                           Application/ScreenshotHelperL])
    fi

    # HDD Smart
    AM_CONDITIONAL([WITH_HDDSMART], [test "x$DISABLE_HDD" != "x1"])
    if test "x$DISABLE_HDD" != x1; then
        AC_CONFIG_SUBDIRS([lib/sqflashhelper
                           Library3rdParty/smarttools
                           Modules/HDDHandler
                           Modules/HDDPMQ])
    fi

    # MCAFEE
    AM_CONDITIONAL([WITH_MCAFEE], [test "x$DISABLE_MCAFEE" != "x1"])
    if test "x$DISABLE_MCAFEE" != x1; then
        AC_CONFIG_SUBDIRS([Modules/ProtectHandler])
    fi

    # ACRONIS
    AM_CONDITIONAL([WITH_ACRONIS], [test "x$DISABLE_ACRONIS" != "x1"])
    if test "x$DISABLE_ACRONIS" != x1; then
        AC_CONFIG_SUBDIRS([Modules/RecoveryHandler])
    fi

    AC_CONFIG_SUBDIRS([Modules/NetMonitorV2
                       Modules/ProcessMonitorV2
                       Modules/SUSIControlHandler
                       Modules/OTAHandler
                       Modules/Modbus_Handler
                       Modules/PowerOnOffHandler
                       Modules/TerminalHandler
                       Modules/SoftwareMonitorHandler
                       Modules/IoTGWHandler
                       Modules/ServiceHandler
                       Modules/RuleEngine
                       Modules/LocalProvision
                       Modules/DataSync])
fi

AC_OUTPUT

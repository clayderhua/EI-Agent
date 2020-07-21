SUBDIRS = Platform \
          Library3rdParty/sqlite \
          Library3rdParty/cjson \
          Library3rdParty/libmodbus \
          Library3rdParty/iniparser \
          Library3rdParty/curl \
          lib/advtools/AdvCC \
          lib/advtools/AdvCompression \
          lib/log \
          lib/base64 \
          lib/des \
          lib/ftphelper \
          lib/md5 \
          lib/messagegenerator \
          lib/handlerkernel \
          lib/readini \
          lib/susihelper \
          lib/xmlhelperlib \
          lib/listhelper \
          lib/queuehelper \
          lib/datatransform \
          lib/mcast-socket \
          lib/simple-tcp-socket \
          lib/aes \
          lib/credentialhelper \
          lib/advcarehelper \
          lib/sagatherinfo \
          Lib_EI \
          Lib_SRP \
          Application/CredentialChecker \
          Application/ModuleMerge \
          Application/AgentEncrypt \
          Application/CAgent \
          Application/Logd \
          Application/WatchdogL

if WITH_PLUGINS
SUBDIRS += Lib_OTA \
           Modules/PowerOnOffHandler \
           Modules/TerminalHandler \
           Modules/RuleEngine \
           Modules/NetMonitorV2 \
           Modules/ProcessMonitorV2 \
           Modules/SoftwareMonitorHandler \
           Modules/SUSIControlHandler \
           Modules/IoTGWHandler \
           Modules/Modbus_Handler \
           Modules/ServiceHandler \
           Modules/OTAHandler \
           Modules/LocalProvision \
           Modules/DataSync

if WITH_X11
SUBDIRS += Library3rdParty/jpeg-8d1 \
           Library3rdParty/x11vnc \
           Library3rdParty/websockify \
           Modules/RemoteKVMHandler \
           Modules/ScreenshotHandler \
           Application/ScreenshotHelperL
endif

if WITH_HDDSMART
SUBDIRS += lib/sqflashhelper \
           Library3rdParty/smarttools \
           Modules/HDDHandler \
           Modules/HDDPMQ
endif

if WITH_MCAFEE
SUBDIRS += Modules/ProtectHandler
endif

if WITH_ACRONIS
SUBDIRS += Modules/RecoveryHandler
endif

endif


AM_CPPFLAGS = -I../../Old_Platforms/Linux/ \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/md5/inc \
			  -I../../lib/ftphelper/inc \
			  -I../../lib/base64/inc \
			  -I../../lib/des/inc \
			  -D PRTT_LINUX_OS_VERSION="\"$(LINUX_OS)$(OS_VERSION)\"" \
              -Wall -fPIC 
lib_LTLIBRARIES = ProtectHandler.la
ProtectHandler_la_LIBADD = ../../lib/log/src/libLog.la
ProtectHandler_la_SOURCES = activate_prtt.c parser_prtt.h public_prtt.h \
			    activate_prtt.h ProtectHandler.c status_prtt.c \
			    capability_prtt.c ProtectHandler.h status_prtt.h \
			    capability_prtt.h ProtectHandlerLog.h unprotect_prtt.c \
			    install_update_prtt.c protect_prtt.c unprotect_prtt.h \
			    install_update_prtt.h protect_prtt.h parser_prtt.c public_prtt.c \
			    ../../Old_Platforms/Linux/platform.c ../../Old_Platforms/Linux/common.c
ProtectHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
							-L../../lib/md5/src -lmd5 \
							-L../../lib/ftphelper/src -lftphelper \
							-L../../lib/base64/src -lbase64 \
							-L../../lib/des/src -ldes \
							-lpthread -ldl -module \
							-release $$(cat ../../VERSION) \
							-Wl,-rpath,. 

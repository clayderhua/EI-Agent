AM_CPPFLAGS = -I../../Old_Platforms/Linux \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/md5/inc \
			  -I../../lib/ftphelper/inc \
			  -I../../lib/base64/inc \
			  -I../../lib/des/inc \
			  -I../../Library3rdParty/sqlite/inc \
			  -D RCVY_LINUX_OS_VERSION="\"$(LINUX_OS)$(OS_VERSION)\"" \
			  -Wall -fPIC 
lib_LTLIBRARIES = RecoveryHandler.la
RecoveryHandler_la_LIBADD = ../../lib/log/src/libLog.la \
							../../Library3rdParty/sqlite/src/libsqlite3.la
RecoveryHandler_la_SOURCES = activate_rcvy.c capability_rcvy.c public_rcvy.c restore_rcvy.h \
			     activate_rcvy.h capability_rcvy.h public_rcvy.h status_rcvy.c \
			     asz_rcvy.c install_update_rcvy.c RecoveryHandler.c status_rcvy.h \
			     asz_rcvy.h install_update_rcvy.h RecoveryHandler.h \
			     backup_rcvy.c parser_rcvy.c RecoveryLog.h \
			     backup_rcvy.h parser_rcvy.h restore_rcvy.c \
			     ../../Old_Platforms/Linux/platform.c ../../Old_Platforms/Linux/common.c
RecoveryHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
							 -L../../lib/md5/src -lmd5 \
							 -L../../lib/ftphelper/src -lftphelper \
							 -L../../lib/base64/src -lbase64 \
							 -L../../lib/des/src -ldes \
							 -lpthread -ldl -module \
							 -release $$(cat ../../VERSION) \
							 -Wl,-rpath,. 
# LDADD = ../../Library/SAConfig/libSAConfig.la $(INTLLIBS)

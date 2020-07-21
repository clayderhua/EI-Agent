AM_CPPFLAGS = -I../../Platform \
			  -I../../Platform/Linux \
			  -I../../Include \
			  -I../../Lib_SRP/SAConfig \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/base64/inc \
			  $(XML_CFLAGS) \
			  -D KVM_LINUX_OS_VERSION="\"$(LINUX_OS)\"" \
			  -Wall -fPIC 
lib_LTLIBRARIES = RemoteKVMHandler.la
RemoteKVMHandler_la_SOURCES = Parser.c kvmconfig.c RemoteKVMHandler.c Parser.h kvmconfig.h RemoteKVMHandler.h
RemoteKVMHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
							 ../../Platform/libWISEUtil.la \
							 ../../lib/log/src/libLog.la
RemoteKVMHandler_la_LDFLAGS = $(XML_LIBS) \
						      -L../../Library3rdParty/cjson/src -lcJSON \
							  -L../../lib/base64/src -lbase64 \
							  -lpthread -ldl -module \
							  -release $$(cat ../../VERSION) \
							  -Wl,-rpath,. 
#LDADD = $(top_srcdir)/Lib_SRP/SAConfig/libSAConfig.la $(INTLLIBS)

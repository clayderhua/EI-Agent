ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux \
              -I../../Include \
              -I../../lib/log/inc \
              -I../../Lib_SRP/SAConfig \
              -I../../lib/sagatherinfo \
              -I../../Lib_SRP/SAClient \
              $(XML_CFLAGS) \
              $(RMM3X) \
              -Wall -fPIC
bin_PROGRAMS = cagent
cagent_SOURCES = cagent.c cagent.h
cagent_LDADD = ../../Platform/libWISEPlatform.la \
               ../../Platform/libWISEUtil.la \
               ../../lib/log/src/libLog.la \
               ../../Lib_SRP/SAConfig/libSAConfig.la \
               ../../Lib_SRP/SAClient/libSAClient.la \
               ../../lib/sagatherinfo/libSAGatherInfo.la \
               ../../Lib_EI/WISECore/libWISECore.la \
               ../../Lib_EI/MQTTConnector/libMQTTConnector.la \
               ../../Lib_EI/MosquittoCarrier/libMosquittoCarrier.la \
               ../../lib/advtools/AdvCompression/src/libAdvCompression.la \
               ../../lib/advtools/AdvCC/src/libAdvCC.la
cagent_LDFLAGS = -lpthread -ldl \
                 -lssl -lcrypto \
                 -Wl,-rpath,./ -lstdc++
LDADD = $(INTLLIBS)

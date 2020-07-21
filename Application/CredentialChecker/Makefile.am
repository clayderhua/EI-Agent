AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../lib/credentialhelper/inc \
              -I../../Library3rdParty/susi4/include \
              -I../../Lib_SRP/SAConfig \
              -I../../Include \
              -I../../lib/log/inc \
              -I../../Library3rdParty/curl/include \
              -Wall -fPIC
bin_PROGRAMS = CredentialChecker
CredentialChecker_SOURCES = CredentialChecker.c
CredentialChecker_LDADD = ../../Platform/libWISEPlatform.la \
                          ../../Platform/libWISEUtil.la \
                          ../../lib/credentialhelper/src/libcredentialhelper.la \
                          ../../Lib_SRP/SAConfig/libSAConfig.la \
                          ../../lib/log/src/libLog.la \
                          -L../../Library3rdParty/curl/lib/.libs -lcurl
CredentialChecker_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
                            -lcrypto -lssl \
                            -lpthread -ldl -lm \
                            -Wl,-rpath,./ -lstdc++
#LDADD = $(INTLLIBS)

AM_CPPFLAGS = -I../../Platform/Linux \
              -I../../lib/log/inc \
              -I../../lib/base64/inc \
              -I../../lib/des/inc \
              -DENC_ONLY \
              -Wall -fPIC
bin_PROGRAMS = AgentEncrypt
AgentEncrypt_SOURCES = AgentEncrypt.c
AgentEncrypt_LDADD = ../../Platform/libWISEPlatform.la \
                     ../../Platform/libWISEUtil.la \
                     ../../lib/log/src/libLog.la
AgentEncrypt_LDFLAGS =  -L../../lib/base64/src/ -lbase64 \
                        -L../../lib/des/src/ -ldes \
                        -lpthread -ldl \
                        -version-info 1:0:1 \
                        -Wl,-rpath,./:./AgentService

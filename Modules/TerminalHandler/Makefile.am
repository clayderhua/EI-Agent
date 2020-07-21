AM_CPPFLAGS = -I./WebShellClient/ \
              -I../../Platform/ \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -DUSE_WEB_SHELL_CLIENT \
              -Wall -fPIC 
lib_LTLIBRARIES = TerminalHandler.la
TerminalHandler_la_SOURCES = Parser.c Parser.h TerminalHandler.c TerminalHandler.h WebShellClient/check.c WebShellClient/check.h WebShellClient/config.h WebShellClient/hashmap.c WebShellClient/hashmap.h WebShellClient/launcher.c WebShellClient/launcher.h WebShellClient/privileges.c WebShellClient/privileges.h WebShellClient/service.c WebShellClient/service.h WebShellClient/session.c WebShellClient/session.h WebShellClient/SSHClient.c WebShellClient/SSHClient.h
TerminalHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
                            ../../Platform/libWISEUtil.la \
                            ../../lib/log/src/libLog.la
TerminalHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
                             -lpthread -ldl -module \
                             -release $$(cat ../../VERSION) \
                             -Wl,-rpath,.

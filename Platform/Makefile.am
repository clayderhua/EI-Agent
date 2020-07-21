AM_CPPFLAGS = -Wall -fPIC -I$(top_builddir)/Linux
lib_LTLIBRARIES = libWISEPlatform.la libWISEUtil.la
libWISEPlatform_la_SOURCES = Linux/binding.h Linux/wrapper.h WISEPlatform.h export.h
libWISEPlatform_la_LDFLAGS = -ldl -release $$(cat VERSION | tr -d '\n')
libWISEUtil_la_SOURCES = basequeue.c basequeue.h Linux/util_path.c util_path.h Linux/util_string.c util_string.h Linux/util_libloader.c util_libloader.h Linux/NamedPipeClient.c NamedPipeClient.h  Linux/NamedPipeServer.c NamedPipeServer.h IPCCommData.h Linux/network.c network.h Linux/util_os.c util_os.h Linux/util_process.c util_process.h Linux/service.c service.h Linux/util_power.c util_power.h Linux/util_storage.c util_storage.h
libWISEUtil_la_LDFLAGS = -ldl -lrt -release $$(cat VERSION | tr -d '\n')

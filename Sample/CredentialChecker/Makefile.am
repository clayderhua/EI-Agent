AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Lib_Util/CredentialHelper
bin_PROGRAMS = CredentialChecker
CredentialChecker_SOURCES = CredentialChecker.c
CredentialChecker_LDFLAGS = -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_Util/CredentialHelper -lcredentialhelper -lpthread -lm -ldl -Wl,-rpath,./ 
#LDADD = $(INTLLIBS)

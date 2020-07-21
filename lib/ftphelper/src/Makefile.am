ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../../../Library3rdParty/curl/include \
            -I../inc \
			-I../../log/inc \
            -Wall -fPIC \
			-L../../../Library3rdParty/curl/lib/.libs -lcurl
lib_LIBRARIES = libftphelper.a
libftphelper_a_SOURCES = FtpDownload.c ftphelper.c FtpUpload.c

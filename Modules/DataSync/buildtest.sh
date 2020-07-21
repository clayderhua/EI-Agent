depbase=`echo TestHandler.lo | sed 's|[^/]*$|.deps/&|;s|\.lo$||'`;/bin/bash ../../libtool  --tag=CXX   --mode=compile g++ -DHAVE_CONFIG_H -I. -I../..  -Wall -fPIC -I../../Platform -I../../Platform/Linux/ -I../../Include -I../../Lib_Util/cJSON -I../../Lib_Util/Log -I../../Lib_Util/ReadINI -I../../Library/SQLite -I../../Lib_Util/HandlerKernel -I../../Lib_Util/MessageGenerator/ -I../../Lib_Util/BufferPool   -g -O2 -MT TestHandler.lo -MD -MP -MF $depbase.Tpo -c -o TestHandler.lo TestHandler.c && mv -f $depbase.Tpo $depbase.Plo
/bin/bash ../../libtool  --tag=CXX   --mode=link g++  -g -O2 -L../../Platform -lWISEPlatform -L../../Lib_Util/cJSON -lcJSON -L../../Lib_Util/Log -lLog -L../../Lib_Util/ReadINI -lReadINI -L../../Library/SQLite -lsqlite3 -L../../Lib_Util/HandlerKernel -lHandlerKernel -L../../Lib_Util/MessageGenerator/ -lmsggen -L../../Lib_Util/BufferPool/ -lbp -lpthread -ldl -release 1.1.3.0  -o libTestHandler.la -rpath /usr/local/lib TestHandler.lo  -lssl -lcrypto
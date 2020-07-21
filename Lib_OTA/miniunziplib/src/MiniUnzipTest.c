#include "MiniUnzipLib.h"
#include <stdio.h>

#define DEF_ZIP_PATH       "aa.zip"
#define DEF_UNZIP_PATH     "./aa"
#define DEF_PWD            "111"

int main(int argc, char * argv[])
{
   if(MiniUnzip(DEF_ZIP_PATH, DEF_UNZIP_PATH, DEF_PWD) != 0)
	{
		printf("MiniUnzip failed!\n");
	}
	else
	{
		printf("MiniUnzip success!\n");
	}
	printf("Press any key quit!\n");
	getchar();
	return 0;
}
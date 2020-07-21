#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <getopt.h>
#include "fplatform.h"
#include "tool.h"
#include "configure.h"
#include "AdvLog.h"
#include "AdvJSON.h"

using namespace std;

typedef struct {
		//information
		int pid;
		int flush;
		char configureFile[256];
		char configureName[256];
		
		//configure
		int staticLevel;
		int staticInfo;
		int staticGray;
		int dynamicLevel;
		int dynamicInfo;
		int dynamicGray;
        char hide[256];
        
        char path[256];
		int files;
        int limit;
		int number;
		int cacheSize;
		
		int elsLevel;
		int elsInfo;
		int elsGray;
		char elsServer[64];
		int elsPort;
		
} AdvLogConf;

AdvLogConf defaultconf = {
    
    /***Fundamental***/
	-1,					//pid
	0,					//flush
	"",					//configureFile
	"default",			//configureName

    /***Display***/
	5,					//staticLevel
	1,					//staticInfo
	0,					//StaticGray
	5,					//dynamicLevel
	0,					//dynamicInfo
	0,					//dynamicGray
	"",					//hide
    
    /***Limitation***/
    "./logs",			//path
    100,                //files
	102400,				//limit
	2,					//number
	BUFSIZ,

    /***ELS***/
	5,                  // elsLevel
	0,                  // elsInfo
	0,                  // elsGray
	"127.0.0.1",        // elsServer
	9200				// elsPort
};

void *mountKeyMemory(unsigned int sharedKey, int size) {
	char *ptr = NULL;
	//printf("sharedKey = 0x%08X\n",sharedKey);
	if (sharedKey == 0xFFFFFFFF) {
		printf("<%s,%s,%d> key:%08X failed\n", __FILE__, __func__, __LINE__, sharedKey);
		return (void *)ptr;
	}
	long shm_id;
	shm_id = shmget(sharedKey, size, 0666 | IPC_CREAT | IPC_EXCL);
	//printf("shm_id = %d\n",shm_id);
	if (shm_id == -1) {
		//printf("size = %d\n",size);
		shm_id = shmget(sharedKey, size, 0666);
		//printf("shm_id = %d\n",shm_id);
		if (shm_id == -1) {
			printf("<%s,%s,%d> key:%08X failed\n", __FILE__, __func__, __LINE__, sharedKey);
			return (void *)ptr;
		}
		else {
			ptr = (char *)shmat(shm_id, NULL, 0);
			shmctl(shm_id, IPC_RMID, 0);
			return (void *)ptr;
		}
	}
	else {
		ptr = (char *)shmat(shm_id, NULL, 0);
		memset(ptr, 0, size);
		shmctl(shm_id, IPC_RMID, 0);
		return (void *)ptr;
	}
	return (void *)ptr;
}


void unmountKeyMemory(void *addr) {
	shmdt(addr);
}

static AdvLogConf *configure = &defaultconf;

static void PrintHelp(int pid) {
	printf("Option -p [pid]: Set pid *(MUST be ASSIGNED)*\n");
	printf("\n");

    /***Display***/
	printf("Option -s [level]: Set STATIC level ------------------------------------------------ (default:5) (positive means HTML mode, negative means text mode)\n");
	printf("Option -i [0|1]: Enable STATIC info ------------------------------------------------ (default:1) (0: disable, 1: full text, 2: only timestamp)\n");
	printf("\n");
	printf("Option -d [level]: Set DYNAMIC level ----------------------------------------------- (default:5) (positive means colorful mode, negative means gray mode)\n");
	printf("Option -j [0|1]: Enable DYNAMIC info ----------------------------------------------- (default:0) (0: disable, 1: full text, 2: only timestamp)\n");
	printf("Option -b {level string}: Hide DYNAMIC message ------------------------------------- (default:\"\", example:\"1,3,5\")\n");
	printf("\n");
    /***Limitation***/
	printf("Option -x [path]: set log files path ----------------------------------------------- (default:./logs)\n");
    printf("Option -a [files]: set the max existing file number of all the files in the log path (a.k.a amount) (default:100, min:30, max:200)\n");
    printf("Option -l [limit]: set the max rolling threshold byte of this process -------------- (unit: byte)   (default:102400, min:10240)\n");
    printf("Option -n [number]: set the max rolling file number of this process ---------------- (a.k.a count)  (default:2, min:2, max:20)\n");
	printf("Option -c [size]: set the cache size of the write buffer --------------------------- (unit: byte)   (default:%d, min:%d, max:65536)\n", BUFSIZ, BUFSIZ);
    printf("\n");

	printf("Option -f: import configure file\n");
	printf("Option -e: export configure file\n");
    printf("\n");
	printf("Option -v: Show all parameter\n");
}

void PrintConf(AdvLogConf *conf) {
    printf("[s]Static Level: %d\n", conf->staticGray == 0 ? conf->staticLevel : -conf->staticLevel);
    printf("[i]Static Info: %d\n", conf->staticInfo);
    printf("[d]Dynamic Level: %d\n", conf->dynamicGray == 0 ? conf->dynamicLevel : -conf->dynamicLevel);
    printf("[j]Dynamic Info: %d\n", conf->dynamicInfo);
    printf("[b]Hide: %s\n", conf->hide);
    printf("[x]Path: %s\n", conf->path);
    printf("[a]Files: %d\n", conf->files);
    printf("[l]Limit: %d\n", conf->limit);
    printf("[n]Number: %d\n", conf->number);
	printf("[c]CacheSize: %d\n", conf->cacheSize);
    printf("[f]Configure file: %s\n", conf->configureFile);
    printf("[m]Configure name: %s\n", conf->configureName);
}

inline static void AdvLog_Configure_Default(AdvLogConf *conf) {
    /***Fundamental***/
	conf->flush = defaultconf.flush;
	strcpy(conf->configureFile,defaultconf.configureFile);
	strcpy(conf->configureName,defaultconf.configureName);

    /***Display***/
    conf->staticLevel = defaultconf.staticLevel;
	conf->staticInfo = defaultconf.staticInfo;
	conf->staticGray = defaultconf.staticGray;
	conf->dynamicLevel = defaultconf.dynamicLevel;
	conf->dynamicInfo = defaultconf.dynamicInfo;
	conf->dynamicGray = defaultconf.dynamicGray;
	strcpy(conf->hide,defaultconf.hide);

    /***Limitation***/
    strcpy(conf->path,defaultconf.path);
	conf->files = defaultconf.files;
	conf->limit = defaultconf.limit;
	conf->number = defaultconf.number;
	conf->cacheSize = defaultconf.cacheSize;

    /***ELS***/
	conf->elsLevel = defaultconf.elsLevel;
	conf->elsInfo = defaultconf.elsInfo;
	conf->elsGray = defaultconf.elsGray;
	strcpy(conf->elsServer,defaultconf.elsServer);
	conf->elsPort = defaultconf.elsPort;
}

inline static AdvLogConf *AdvLog_Configure_Attach(int pid) {
	if(configure == &defaultconf) {
		int sharedKey = 0x20150707 - pid;
		configure = (AdvLogConf *)mountKeyMemory(sharedKey, sizeof(AdvLogConf));
	}
	return configure;
}

int AdvLog_Configure_Init(int pid) {
	AdvLogConf *conf = AdvLog_Configure_Attach(pid);
	if(conf != NULL) {
		if(conf->pid != pid) {
			conf->pid = pid;
			AdvLog_Configure_Default(conf);
		}
		return pid;
	} else return 0;
}

static void AdvLog_Configure_Import_From_JSON(AdvLogConf *conf, AdvJSON json) {
	if(json.IsNULL()) return;
	
	string value;
	//json.Print();
	//printf("Assing: conf->configureName = %s\n", conf->configureName);
	/***Fundamental***/
	
    /***Display***/
	value = json[conf->configureName]["static"]["level"].Value();
	if(value != "NULL") {
		conf->staticLevel = atoi(value.c_str());
		if(conf->staticLevel < 0) {
			conf->staticGray = 1;
			conf->staticLevel = -conf->staticLevel;
		} else {
			conf->staticGray = 0;
		}
		//printf("conf->staticLevel = %d\n", conf->staticLevel);
	}
	
	value = json[conf->configureName]["static"]["information"].Value();
	if(value != "NULL") {
		conf->staticInfo = atoi(value.c_str());
	}
	
	
	value = json[conf->configureName]["dynamic"]["level"].Value();
	if(value != "NULL") {
		conf->dynamicLevel = atoi(value.c_str());
		if(conf->dynamicLevel < 0) {
			conf->dynamicGray = 1;
			conf->dynamicLevel = -conf->dynamicLevel;
		} else {
			conf->dynamicGray = 0;
		}
	}
	
	value = json[conf->configureName]["dynamic"]["information"].Value();
	if(value != "NULL") {
		conf->dynamicInfo = atoi(value.c_str());
	}

    value = json[conf->configureName]["dynamic"]["hide"].Value();
	if(value != "NULL") {
		strcpy(conf->hide,value.c_str());
	}
    
    /***Limitation***/
    value = json[conf->configureName]["path"].Value();
	if(value != "NULL") {
		strcpy(conf->path,value.c_str());
	}

    value = json[conf->configureName]["files"].Value();
	if(value != "NULL") {
		conf->files = atoi(value.c_str());
		if(conf->files < 30 )
			conf->files = 30;
		else if ( conf->files > 200 )
			conf->files = 200;
	}

	value = json[conf->configureName]["limit"].Value();
	if(value != "NULL") {
		conf->limit = atoi(value.c_str());
        if(conf->limit < 10240 )
            conf->limit = 10240;
    }

	
	value = json[conf->configureName]["number"].Value();
	if(value != "NULL") {
		conf->number = atoi(value.c_str());
		if(conf->number < 2 )
			conf->number = 2;
		else if ( conf->number > 20 )
			conf->number = 20;
	}

	value = json[conf->configureName]["cachesize"].Value();
	if (value != "NULL") {
		conf->cacheSize = atoi(value.c_str());
		if (conf->cacheSize < BUFSIZ)
			conf->cacheSize = BUFSIZ;
		else if (conf->cacheSize > 65536)
			conf->cacheSize = 65536;
	}
	
    /***ELS***/
	value = json[conf->configureName]["els"]["information"].Value();
	if(value != "NULL") {
		conf->elsInfo = atoi(value.c_str());
	}
	
	
	value = json[conf->configureName]["els"]["level"].Value();
	if(value != "NULL") {
		conf->elsLevel = atoi(value.c_str());
		if(conf->elsLevel < 0) {
			conf->elsGray = 1;
			conf->elsLevel = -conf->elsLevel;
		} else {
			conf->elsGray = 0;
		}
	}

	value = json[conf->configureName]["els"]["server"].String();
	if( value != "NULL" || value != "Type Error!!" )
		strcpy(conf->elsServer,value.c_str());

	value = json[conf->configureName]["els"]["port"].Value();
	if(value != "NULL") {
		conf->elsPort = atoi(value.c_str());
	if( conf->elsPort <0 || conf->elsPort > 65535 )
		conf->elsPort = 9200;
	}

	//printf("Assing: json[conf->configureName][dynamic][information].Value() = %s\n", json[conf->configureName]["dynamic"]["information"].Value().c_str());
	//printf("Assing: conf->dynamicInfo = %d\n", conf->dynamicInfo);
	
}

static void AdvLog_Configure_Export_To_File(AdvLogConf *conf, char *filename) {
	AdvJSON json("{}");
#if __cplusplus > 199711L
	AdvJSONCreator C(json);
	json.New()["default"][{"static", "dynamic", "path", "files", "limit", "number", "els"}] = {
		C[{"level","information"}]({
			(conf->staticGray == 1 ? -conf->staticLevel : conf->staticLevel),
			conf->staticInfo
			}),
		C[{"level","information", "hide"}]({
			(conf->dynamicGray == 1 ? -conf->dynamicLevel : conf->dynamicLevel),
			conf->dynamicInfo,
			conf->hide
			}),
        conf->path,
        conf->files,
		conf->limit,
		conf->number,
        C[{"level", "information", "server", "port"}]({
			(conf->elsGray == 1 ? -conf->elsLevel : conf->elsLevel),
			conf->elsInfo,
            conf->elsServer,
            conf->elsPort
			}),
	};
#else
	/***Display***/
	json.New()["default"]["static"]["level"] = (conf->staticGray == 1 ? -conf->staticLevel : conf->staticLevel);
	json.New()["default"]["static"]["information"] = conf->staticInfo;
	json.New()["default"]["dynamic"]["level"] = (conf->dynamicGray == 1 ? -conf->dynamicLevel : conf->dynamicLevel);
	json.New()["default"]["dynamic"]["information"] = conf->dynamicInfo;
	json.New()["default"]["dynamic"]["hide"] = conf->hide;
    
    /***Limitation***/
    json.New()["default"]["path"] = conf->path;
    json.New()["default"]["files"] = conf->files;
	json.New()["default"]["limit"] = conf->limit;
	json.New()["default"]["number"] = conf->number;
	json.New()["default"]["cachesize"] = conf->cacheSize;

    /***ELS***/
	json.New()["default"]["els"]["level"] = (conf->elsGray == 1 ? -conf->elsLevel : conf->elsLevel);
	json.New()["default"]["els"]["information"] = conf->elsInfo;
	json.New()["default"]["els"]["server"] = conf->elsServer;
	json.New()["default"]["els"]["port"] = conf->elsPort;
#endif
	//json.Print();

	char buffer[4096] = {0};
	json.Print(buffer, sizeof(buffer),0);

	FILE *fp = fopen(filename,"w+");
	fprintf(fp,"%s",buffer);
	fclose(fp);
}

static void AdvLog_Configure_Import_From_File(AdvLogConf *conf, char *filename) {
    char *jsonbuffer = fmalloc(conf->configureFile);
    if(jsonbuffer == NULL) {
        AdvLog_Configure_Export_To_File(conf, conf->configureFile);
        jsonbuffer = fmalloc(conf->configureFile);
    }
    AdvLog_Configure_Import_From_JSON(conf, jsonbuffer);
    if(jsonbuffer != NULL) free(jsonbuffer);
    jsonbuffer = NULL;
}

int AdvLog_Configure_OptParser(int argc, char **argv, int pid) {
	int index;
	int c;
	AdvLogConf *conf = &defaultconf;
	//printf("<%s,%d>\n",__FILE__,__LINE__);
	/*if(argc <= 1) {
		PrintHelp();
	}*/
	if(argc == 1) return 0;
	//printf("<%s,%d> pid = %d\n",__FILE__,__LINE__, pid);
	if(pid != -1) {
		conf = AdvLog_Configure_Attach(pid);
	}
	//printf("<%s,%d>\n",__FILE__,__LINE__);
	while ((c = getopt (argc, argv, "p:s:i:d:j:b:x:a:l:n:c:m:f:e:vh?")) != -1) {
		//printf("<%s,%d> c = %c\n",__FILE__,__LINE__,c);
		switch (c)
		{
			case 'p':
				pid = atoi(optarg);
				conf = AdvLog_Configure_Attach(pid);
			break;
            /***Display***/
			case 's':
				if(pid > 0) {
					conf->staticLevel = atoi(optarg);
					if(conf->staticLevel < 0) {
						conf->staticGray = 1;
					} else {
						conf->staticGray = 0;
					}
					conf->staticLevel = ABS(conf->staticLevel);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'i':
				if(pid > 0) {
					conf->staticInfo = atoi(optarg);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'd':
				if(pid > 0) {
					conf->dynamicLevel = atoi(optarg);
					if(conf->dynamicLevel < 0) {
						conf->dynamicGray = 1;
					} else {
						conf->dynamicGray = 0;
					}
					conf->dynamicLevel = ABS(conf->dynamicLevel);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'j':
				if(pid > 0) {
					conf->dynamicInfo = atoi(optarg);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'b':
				if(pid > 0) {
					strncpy(conf->hide, optarg, sizeof(conf->hide));
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
            
            /***Limitation***/
            case 'x':
				if(pid > 0) {
					strncpy(conf->path, optarg, sizeof(conf->path));
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
            case 'a':
				if(pid > 0) {
					conf->files = atoi(optarg);
					if(conf->files < 30 )
						conf->files = 30;
					else if ( conf->files > 200 )
						conf->files = 200;
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'l':

				if(pid > 0) {
					conf->limit = atoi(optarg);

				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'n':
				if(pid > 0) {
					conf->number = atoi(optarg);
                    if(conf->number < 2 )
						conf->number = 2;
					else if ( conf->number > 20 )
						conf->number = 20;
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'c':
				if (pid > 0) {
					conf->cacheSize = atoi(optarg);
					if (conf->cacheSize < BUFSIZ)
						conf->cacheSize = BUFSIZ;
					else if (conf->cacheSize > 65536)
						conf->cacheSize = 65536;
				}
				else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
            case 'm':
				{
					if(pid > 0) {
						strncpy(conf->configureName,optarg,sizeof(conf->configureName));
						AdvLog_Configure_Import_From_File(conf, conf->configureFile);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
				}
			break;
			case 'f':
				{
					if(pid > 0) {
						strncpy(conf->configureFile,optarg,sizeof(conf->configureFile));
                        AdvLog_Configure_Import_From_File(conf, conf->configureFile);
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
				}
			break;
			case 'e':
				{
					if(pid > 0) {
						AdvLog_Configure_Export_To_File(conf, optarg);
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
				}
			break;
			case 'v':
					if(pid > 0) {
					PrintConf(conf);
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
			break;
			case 'h':
			case '?':
				PrintHelp(pid);
			return 1;
			default:
				return 1;
		}
	}

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);
	return 0;
}

int AdvLog_Configure_GetPid() {
	if(configure == NULL) return 0;
	return configure->pid;
}
const char *AdvLog_Configure_Name() {
	if(configure == NULL) return NULL;
	return configure->configureName;
}

/***Display***/
int AdvLog_Configure_GetStaticLevel() {
	if(configure == NULL) return 0;
	return configure->staticLevel;
}

int AdvLog_Configure_GetStaticGray() {
	if(configure == NULL) return 0;
	return configure->staticGray;
}

int AdvLog_Configure_GetStaticInfo() {
	if(configure == NULL) return 0;
	return configure->staticInfo;
}

int AdvLog_Configure_GetDynamicLevel() {
	if(configure == NULL) return 0;
	return configure->dynamicLevel;
}

int AdvLog_Configure_GetDynamicGray() {
	if(configure == NULL) return 0;
	return configure->dynamicGray;
}

int AdvLog_Configure_GetDynamicInfo() {
	if(configure == NULL) return 0;
	return configure->dynamicInfo;
}
int AdvLog_Configure_Is_Hiden(int level) {
	if(configure == NULL) return 0;
	if(configure->hide[0] == 0) return 0;
	return strchr(configure->hide,level+48) == 0 ? 0 : 1;
}
int AdvLog_Configure_Hide_Enable() {
	if(configure == NULL) return 0;
	return configure->hide[0] == 0 ? 0 : 1;
}

int AdvLog_Configure_Determine_Status(int level) {
    if(configure == NULL) return __ADV_PRINT_STATUS_PW;
    if(level > configure->dynamicLevel && level > configure->staticLevel) return __ADV_PRINT_STATUS_NONE;
    
    int hiden = configure->hide[0] == 0 ? 0 : (strchr(configure->hide,level+48) == 0 ? 0 : 1);
    if(level <= configure->dynamicLevel && level <= configure->staticLevel && !hiden) return __ADV_PRINT_STATUS_PW;
	else if(level <= configure->dynamicLevel && level > configure->staticLevel && !hiden) return __ADV_PRINT_STATUS_P;
	else if(level <= configure->staticLevel) return __ADV_PRINT_STATUS_W;
	return __ADV_PRINT_STATUS_NONE;
}

/***Limitation***/
const char *AdvLog_Configure_GetPath() {
	if(configure == NULL) return NULL;
	return configure->path;
}

int AdvLog_Configure_GetFiles() {
	if(configure == NULL) return 0;
	return configure->files;
}
int AdvLog_Configure_GetLimit() {
	if(configure == NULL) return 0;
	return configure->limit;
}
int AdvLog_Configure_GetNumber() {
	if(configure == NULL) return 0;
	return configure->number;
}
int AdvLog_Configure_GetCacheSize() {
	if (configure == NULL) return 0;
	return configure->cacheSize;
}

/***ELS***/
int AdvLog_Configure_GetElsInfo() {
	if(configure == NULL) return 0;
	return configure->elsInfo;
}

const char *AdvLog_Configure_GetElsServer() {
	if(configure == NULL) return NULL;
	return configure->elsServer;
}
int AdvLog_Configure_GetElsPort() {
	if(configure == NULL) return 0;
	return configure->elsPort;
}

void AdvLog_Configure_Uninit() {
	if(configure != NULL) {
		unmountKeyMemory(configure);
		configure = NULL;
	}
}
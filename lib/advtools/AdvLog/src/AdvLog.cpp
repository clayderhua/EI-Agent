#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <signal.h>
#include "fplatform.h"
#include "tool.h"
#include "advstring.h"
#include "AdvLog.h"
#include "configure.h"
#include "rollingfile.h"
#include "AdvElsAPI.h"


#define TIME_STRING_FMT "[%Y/%m/%d %H:%M:%S]"

//Normal
static pthread_mutex_t *pmutex = NULL;
static pthread_mutex_t mutex;
//advstring strin(1024);
//advstring strout(1024);

char LevelTag[LOG_MAX][256] =
{
	{ "" },
	{ __COLOR_RED_BG "[CRASH]" __COLOR_NONE __COLOR_GRAY },	//1
	{ __COLOR_RED_BG "[ERROR]" __COLOR_NONE __COLOR_GRAY },	//2
	{ __COLOR_YELLOW "[WARN]" __COLOR_NONE __COLOR_GRAY },	//3
	{ __COLOR_YELLOW "[NOTICE]" __COLOR_NONE __COLOR_GRAY },	//4
	{ "[INFO]" },										//5
	{ "" },											//6
	{ __COLOR_WHITE_BG "[DEBUG]" __COLOR_NONE __COLOR_GRAY },//7
	{ __COLOR_WHITE_BG "[TRACE]" __COLOR_NONE __COLOR_GRAY },//8
	{ __COLOR_WHITE_BG "[DUMP]" __COLOR_NONE __COLOR_GRAY }	//9
};

char LevelNoColor[LOG_MAX][256] =
{
	{ "" },
	{ "[CRASH]" },	//1
	{ "[ERROR]" },	//2
	{ "[WARN]" },		//3
	{ "[NOTICE]" },	//4
	{ "[INFO]" },		//5
	{ "" },			//6
	{ "[DEBUG]" },	//7
	{ "[TRACE]" },	//8
	{ "[DUMP]" }		//9
};

char LevelHtml[LOG_MAX][256] =
{
	{ "" },
	{ __COLOR_RED_BG_HTML "[CRASH]" __COLOR_RED_BG_END_HTML },	//1
	{ __COLOR_RED_BG_HTML "[ERROR]" __COLOR_RED_BG_END_HTML },	//2
	{ __COLOR_YELLOW_HTML "[WARN]" __COLOR_YELLOW_END_HTML },	//3
	{ __COLOR_YELLOW_HTML "[NOTICE]" __COLOR_YELLOW_END_HTML },	//4
	{ "[INFO]" },										//5
	{ "" },											//6
	{ __COLOR_WHITE_BG_HTML "[DEBUG]" __COLOR_WHITE_BG_END_HTML },//7
	{ __COLOR_WHITE_BG_HTML "[TRACE]" __COLOR_WHITE_BG_END_HTML },//8
	{ __COLOR_WHITE_BG_HTML "[DUMP]" __COLOR_WHITE_BG_END_HTML }	//9
};



static void AdvLog_Exit() {
	RollFile_Close();
	AdvLog_Configure_Uninit();
}

static void AdvLog_Sig_Handler(int n) {
	exit(0);
}

static void AdvLog_Crash_Handler(int n) {
	exit(-1);
}

void ADVLOG_CALL AdvLog_Init(void) {
	if(pmutex == NULL) {
		pthread_mutex_init(&mutex, NULL);
		pmutex = &mutex;
		int pid = getpid();
		if(pid == 0) pid = getppid();
		AdvLog_Configure_Init(pid);
		RollFile_Open(AdvLog_Configure_GetPath());
		atexit(AdvLog_Exit);

		signal(SIGTERM, AdvLog_Sig_Handler);
		signal(SIGINT, AdvLog_Sig_Handler);

		// ignore SIGPIPE
#if !defined(WIN32)
		signal(SIGPIPE, SIG_IGN);

		signal(SIGBUS, AdvLog_Crash_Handler);
#endif
		signal(SIGSEGV, AdvLog_Crash_Handler);
		signal(SIGFPE, AdvLog_Crash_Handler);
		signal(SIGABRT, AdvLog_Crash_Handler);

	}
}



void ADVLOG_CALL AdvLog_Arg(int argc, char **argv) {
	//printf("<%s,%d> LOCK\n",__FILE__,__LINE__);
	pthread_mutex_lock(&mutex);
	AdvLog_Configure_OptParser(argc, argv, getpid());
	RollFile_RefreshConfigure();
	pthread_mutex_unlock(&mutex);
	//printf("<%s,%d> UNLOCK\n",__FILE__,__LINE__);
}

void ADVLOG_CALL AdvLog_Default(char *arg) {
	pthread_mutex_lock(&mutex);
	char temp[1024];
	char *pos = temp;
	char *end;
	int number;
	strcpy(temp,arg);
	std::vector<std::string> args;
	//char *saveptr;
	//printf("arg = %s\n", arg);
	args.push_back("AdvLog");

	while(1) {
		if(*pos =='\"' || *pos == '\'') {
			pos++;
			number = strcspn(pos,"\"\'");
			pos[number] = 0;
			args.push_back(pos);
			pos += number+2;
		} else {
			if((end = strchr(pos,' ')) != NULL) {
				end[0] = 0;
				args.push_back(pos);
				pos = end+1;
			} else {
				args.push_back(pos);
				break;
			}
		}
	}
	
	int argc = args.size();
	char **argv = (char **)malloc(sizeof(char *)*argc);
	for (int i = 0; i < argc; i++) {
		argv[i] = (char *)args[i].c_str();
		//printf("argv[%d] = %s\n",i,argv[i]);
	}
	//printf("<%s,%d>\n",__FILE__,__LINE__);
	AdvLog_Configure_OptParser(argc, argv, getpid());
	RollFile_RefreshConfigure();
    if(AdvLog_Configure_GetElsInfo()){
    	AdvEls_Init(AdvLog_Configure_GetElsServer(), AdvLog_Configure_GetElsPort());
    }
	free(argv);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_Control(int argc, char **argv) {
	pthread_mutex_lock(&mutex);
	AdvLog_Configure_OptParser(argc, argv, -1);
	pthread_mutex_unlock(&mutex);
}

int ADVLOG_CALL AdvLog_Status(char *ConfName, int level) {
	return AdvLog_Configure_Determine_Status(level);
}
/*
int ADVLOG_CALL AdvLog_AllNone(char *confname, int level) {
	return level > AdvLog_Configure_GetDynamicLevel() && level > AdvLog_Configure_GetStaticLevel();  
}

int ADVLOG_CALL AdvLog_Static_Is_On(char *confname, int level) {
	return level <= AdvLog_Configure_GetStaticLevel();
}
*/
inline int AdvLog_Static_Is_Gray_Mode(char *confname, int level) {
	return AdvLog_Configure_GetStaticGray();
}

inline int AdvLog_Static_InfoMode(char *confname) {
	return AdvLog_Configure_GetStaticInfo();
}
/*
int ADVLOG_CALL AdvLog_Dynamic_Is_On(char *confname, int level) {
	return level <= AdvLog_Configure_GetDynamicLevel();
}
*/
inline int AdvLog_Dynamic_Is_Gray_Mode(char *confname, int level) {
	return AdvLog_Configure_GetDynamicGray();
}

inline int AdvLog_Dynamic_InfoMode(char *confname) {
	return AdvLog_Configure_GetDynamicInfo();
}
/*
int ADVLOG_CALL AdvLog_Dynamic_Is_Hiden(char *confname, int level) {
	return AdvLog_Configure_Is_Hiden(level);
}
*/
#if defined(WIN32) //windows
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
inline short getwcolor(int color) {
	switch (color) {
	case COLOR_RED:
		return _W_COLOR_RED;
	case COLOR_GREEN:
		return _W_COLOR_GREEN;
	case COLOR_YELLOW:
		return _W_COLOR_YELLOW;
	case COLOR_BLUE:
		return _W_COLOR_BLUE;
	case COLOR_PURPLE:
		return _W_COLOR_PURPLE;
	case COLOR_CYAN:
		return _W_COLOR_CYAN;
	case COLOR_GRAY:
		return _W_COLOR_GRAY;

	case COLOR_RED_BG:
		return _W_COLOR_RED_BG;
	case COLOR_GREEN_BG:
		return _W_COLOR_GREEN_BG;
	case COLOR_YELLOW_BG:
		return _W_COLOR_YELLOW_BG;
	case COLOR_BLUE_BG:
		return _W_COLOR_BLUE_BG;
	case COLOR_PURPLE_BG:
		return _W_COLOR_PURPLE_BG;
	case COLOR_CYAN_BG:
		return _W_COLOR_CYAN_BG;
	case COLOR_WHITE_BG:
		return _W_COLOR_WHITE_BG;
	case COLOR_NONE:
	default:
		return _W_COLOR_NONE;

	}
}

inline void setlevel(int level) {
	switch (level) {

	case LOG_CRASH:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_RED_BG));
		printf("[CRASH]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_ERROR:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_RED_BG));
		printf("[ERROR]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_WARN:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_YELLOW));
		printf("[WARN]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_NOTICE:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_YELLOW));
		printf("[NOTICE]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_INFO:
		printf("[INFO]");
		break;
	case LOG_DEBUG:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		printf("[DEBUG]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_TRACE:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		printf("[TRACE]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_DUMP:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		printf("[DUMP]");
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_NONE:
	default:
		break;
	}
}

inline void __AdvLog_PrintContent(char *confname, int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	char timestr[256] = {0};

	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD saved_attributes;
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	saved_attributes = consoleInfo.wAttributes;
	va_list args;
	va_copy(args, ap);

	switch(AdvLog_Dynamic_InfoMode(confname)) {
		case 0:
		{
			if (color != NULL && !AdvLog_Dynamic_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,color,content,COLOR_NONE,NULL);
				SetConsoleTextAttribute(hConsole, getwcolor(color));
				vprintf(fmt, args);
				SetConsoleTextAttribute(hConsole, saved_attributes);
			}
			else {
				//strout.insert(1,content,NULL);
				vprintf(fmt, args);
			}
		} break;
		case 1:
		{
			if(timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
				//snprintf(timestr, sizeof(timestr), "[%s %s]",__DATE__,__TIME__);
			}
			if (AdvLog_Dynamic_Is_Gray_Mode(confname,level)) {
				vprintf(fmt, args);
				printf(" |______%s %s (%s,%s,%s)\n", timestr, LevelNoColor[level],file,func,line);
			}
			else {
				if (color != NULL) {
					//strout.insert(1,color,content,COLOR_NONE COLOR_GRAY,timestr," ",LevelTag[level]," <",file,",",func,",",line,">" COLOR_NONE "\n",NULL);
					SetConsoleTextAttribute(hConsole, getwcolor(color));
					vprintf(fmt, args);
					SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
					printf(" |______%s ",timestr);
					setlevel(level);
					printf(" (%s,%s,%s)", file, func, line);
					SetConsoleTextAttribute(hConsole, saved_attributes);
					printf("\n");
				}
				else {
					//strout.insert(1, content, COLOR_GRAY, timestr, " ", LevelTag[level], " <", file, ",", func, ",", line, ">" COLOR_NONE "\n", NULL);
					vprintf(fmt, args);
					SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
					printf(" |______%s ",timestr);
					setlevel(level);
					printf(" (%s,%s,%s)", file, func, line);
					SetConsoleTextAttribute(hConsole, saved_attributes);
					printf("\n");
				}
			}
		} break;
		case 2:
		{
			if (timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
				//snprintf(timestr, sizeof(timestr), "[%s %s]", __DATE__, __TIME__);
			}
			if (AdvLog_Dynamic_Is_Gray_Mode(confname, level)) {
				printf("%s ", timestr);
				vprintf(fmt, args);
			}
			else {
				if (color != NULL) {
					//strout.insert(1,color,content,COLOR_NONE COLOR_GRAY,timestr," ",LevelTag[level]," <",file,",",func,",",line,">" COLOR_NONE "\n",NULL);
					printf("%s", timestr);
					printf(" ");
					SetConsoleTextAttribute(hConsole, getwcolor(color));
					vprintf(fmt, args);
					SetConsoleTextAttribute(hConsole, saved_attributes);
					printf("\n");
				}
				else {
					//strout.insert(1, content, COLOR_GRAY, timestr, " ", LevelTag[level], " <", file, ",", func, ",", line, ">" COLOR_NONE "\n", NULL);
					printf("%s", timestr);
					printf(" ");
					vprintf(fmt, args);
					SetConsoleTextAttribute(hConsole, saved_attributes);
					printf("\n");
				}
			}
		} break;
		
	}
    va_end(args);
}

#elif defined(__linux)//linux

inline const char *getcolor(int color) {
        switch (color) {
        case COLOR_RED:
                return __COLOR_RED;
        case COLOR_GREEN:
                return __COLOR_GREEN;
        case COLOR_YELLOW:
                return __COLOR_YELLOW;
        case COLOR_BLUE:
                return __COLOR_BLUE;
        case COLOR_PURPLE:
                return __COLOR_PURPLE;
        case COLOR_CYAN:
                return __COLOR_CYAN;
        case COLOR_GRAY:
                return __COLOR_GRAY;

        case COLOR_RED_BG:
                return __COLOR_RED_BG;
        case COLOR_GREEN_BG:
                return __COLOR_GREEN_BG;
        case COLOR_YELLOW_BG:
                return __COLOR_YELLOW_BG;
        case COLOR_BLUE_BG:
                return __COLOR_BLUE_BG;
        case COLOR_PURPLE_BG:
                return __COLOR_PURPLE_BG;
        case COLOR_CYAN_BG:
                return __COLOR_CYAN_BG;
        case COLOR_WHITE_BG:
                return __COLOR_WHITE_BG;
        case COLOR_NONE:
        default:
                return __COLOR_NONE;
        }
}

inline void __AdvLog_PrintContent(char *confname, int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	char timestr[256] = {0};
	int remap = (fmt[strlen(fmt)-1] == '\n' ? 0 : 1);
	//char *string;
	int len = 0;
	va_list args;
	va_copy(args, ap);
	//char *string = NULL;
	//ReduceBackGroundAndDumpString(content, &string);

	switch (AdvLog_Dynamic_InfoMode(confname)) {
		case 0:
		{
			if (color != COLOR_NONE && !AdvLog_Dynamic_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,getcolor(color),(string==NULL ? content : string),__COLOR_NONE,NULL);
				if (remap == 1) {
					printf("%s", getcolor(color));
					/*string = strdup(fmt);
					string[strlen(string)-1] = 0;
					vprintf(string, args);
					free(string);*/
					vprintf(fmt, args);
					printf(__COLOR_NONE "\n" __COLOR_NONE "\n");
				}
				else {
					printf("%s", getcolor(color));
					vprintf(fmt, args);
					printf(__COLOR_NONE "\n");
				}
			}
			else {
				//strout.insert(1,(string==NULL ? content : string),NULL);
				vprintf(fmt, args);
			}
		} break;
		case 1:
		{
			if (timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
			}
			if (AdvLog_Dynamic_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,(string==NULL ? content : string),timestr," ",LevelNoColor[level]," (",file,",",func,",",line,")\n",NULL);
				vprintf(fmt, args);
				printf(__COLOR_NONE " |______%s %s (%s,%s,%s)\n" __COLOR_NONE, timestr, LevelNoColor[level], file, func, line);
			}
			else {
				if (color != COLOR_NONE) {
					//strout.insert(1,getcolor(color),(string==NULL ? content : string),__COLOR_NONE __COLOR_GRAY,timestr,__COLOR_NONE " ",LevelTag[level]," (",file,",",func,",",line,")" __COLOR_NONE "\n",NULL);
					if (remap == 1) {
						printf("%s", getcolor(color));
						vprintf(fmt, args);
						printf(__COLOR_NONE "\n" __COLOR_NONE __COLOR_GRAY " |______%s" __COLOR_NONE " %s (%s,%s,%s)" __COLOR_NONE "\n", timestr, LevelTag[level], file, func, line);
					}
					else {
						printf("%s", getcolor(color));
						vprintf(fmt, args);
						printf(__COLOR_NONE __COLOR_GRAY " |______%s" __COLOR_NONE " %s (%s,%s,%s)" __COLOR_NONE "\n", timestr, LevelTag[level], file, func, line);
					}

				}
				else {
					vprintf(fmt, args);
					printf(__COLOR_NONE " |______%s" __COLOR_NONE " %s (%s,%s,%s)" __COLOR_NONE "\n", timestr, LevelTag[level], file, func, line);
				}
			}
		} break;
		case 2:
		{
			if (timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
			}
			if (AdvLog_Dynamic_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,(string==NULL ? content : string),timestr," ",LevelNoColor[level]," (",file,",",func,",",line,")\n",NULL);
				printf(__COLOR_NONE "%s" __COLOR_NONE, timestr);
				vprintf(fmt, args);
				printf(__COLOR_NONE);
			}
			else {
				if (color != COLOR_NONE) {
					//strout.insert(1,getcolor(color),(string==NULL ? content : string),__COLOR_NONE __COLOR_GRAY,timestr,__COLOR_NONE " ",LevelTag[level]," (",file,",",func,",",line,")" __COLOR_NONE "\n",NULL);
					if (remap == 1) {
						printf(__COLOR_GRAY "%s" __COLOR_NONE " " __COLOR_NONE "%s", timestr, getcolor(color));
						vprintf(fmt, args);
						printf(__COLOR_NONE "\n" __COLOR_NONE);
					}
					else {
						printf(__COLOR_NONE __COLOR_GRAY "%s" __COLOR_NONE " " __COLOR_NONE "%s", timestr, getcolor(color));
						vprintf(fmt, args);
						printf(__COLOR_NONE);
					}

				}
				else {
					printf(__COLOR_NONE "%s" __COLOR_NONE " " __COLOR_NONE, timestr);
					vprintf(fmt, args);
					printf(__COLOR_NONE);
				}
			}
		} break;
	}
	//std::cout << strout.str();
	//fwrite(strout.str(),strout.size(),1,stdout);

	//if(string != NULL) free(string);
    va_end(args);
}
#endif

inline void __AdvLog_WriteContent(char *confname, int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	char timestr[256] = {0};
	FILE *outFile = RollFile_Check();
	int len = 0;
	va_list args;
	va_copy(args, ap);
		
	switch (AdvLog_Static_InfoMode(confname)) {
		case 0:
		{
			if (color != NULL && !AdvLog_Static_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,color,content,colorend,NULL);
				len += fprintf(outFile, __LOG_HEAD_HTML "%s" __NODECODE_HTML, color);
				len += vfprintf(outFile, fmt, args);
				len += fprintf(outFile, __NODECODE_END_HTML "%s" __LOG_END_HTML, colorend);
			}
			else {
				//strout.insert(1,content);
				len += vfprintf(outFile, fmt, args);
			}
			//RollFile_StreamIn(strout.str(), strout.size());
		} break;
		case 1:
		{
			if (timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
			}

			if (AdvLog_Static_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,content,timestr," ",LevelNoColor[level]," (",file,",",func,",",line,")\n",NULL);
				len += vfprintf(outFile, fmt, args);
				len += fprintf(outFile, " |______%s %s (%s,%s,%s)\n", timestr, LevelNoColor[level], file, func, line);
			}
			else {
				if (color != NULL) {
					//strout.insert(1,color,content,colorend,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
					len += fprintf(outFile, __LOG_HEAD_HTML "%s" __NODECODE_HTML, color);
					len += vfprintf(outFile, fmt, args);
					len += fprintf(outFile, __NODECODE_END_HTML "%s" __COLOR_GRAY_HTML " |______%s %s (%s,%s,%s)" __COLOR_GRAY_END_HTML "\n" __LOG_END_HTML, colorend, timestr, LevelHtml[level], file, func, line);
				}
				else {
					//strout.insert(1,content,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
					len += fprintf(outFile, "%s", __LOG_HEAD_HTML __NODECODE_HTML);
					len += vfprintf(outFile, fmt, args);
					len += fprintf(outFile, __NODECODE_END_HTML __COLOR_GRAY_HTML " |______%s %s (%s,%s,%s)" __COLOR_GRAY_END_HTML "\n" __LOG_END_HTML, timestr, LevelHtml[level], file, func, line);
				}
			}
			//RollFile_StreamIn(strout.str(), strout.size());
		} break;
		case 2:
		{
			if (timestr[0] == 0) {
				GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
			}

			if (AdvLog_Static_Is_Gray_Mode(confname, level)) {
				//strout.insert(1,content,timestr," ",LevelNoColor[level]," (",file,",",func,",",line,")\n",NULL);
				len += fprintf(outFile, "%s ", timestr);
				len += vfprintf(outFile, fmt, args);
			}
			else {
				if (color != NULL) {
					//strout.insert(1,color,content,colorend,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
					len += fprintf(outFile, __LOG_HEAD_HTML __COLOR_GRAY_HTML "%s" __COLOR_GRAY_END_HTML "%s" __NODECODE_HTML, timestr, color);
					len += vfprintf(outFile, fmt, args);
					len += fprintf(outFile, __NODECODE_END_HTML "%s\n" __LOG_END_HTML, colorend);
				}
				else {
					//strout.insert(1,content,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
					len += fprintf(outFile, __LOG_HEAD_HTML __COLOR_GRAY_HTML "%s" __COLOR_GRAY_END_HTML __NODECODE_HTML, timestr);
					len += vfprintf(outFile, fmt, args);
					len += fprintf(outFile, __NODECODE_END_HTML "\n" __LOG_END_HTML);
				}
			}
			//RollFile_StreamIn(strout.str(), strout.size());
		} break;
	}
	RollFile_Flush(len);
    va_end(args);
}

inline void __AdvLog_ElsWriteContent( const char* DataPath,  const char *data, int len ) {
	char result[1024] = {0};
	AdvEls_Write(DataPath, data, result, 1024 );
}

inline void __AdvLog_Print(char *confname, int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	__AdvLog_PrintContent(confname, level, color, file, func, line, levels,fmt, ap);
}

inline void __AdvLog_Write(char *confname, int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	__AdvLog_WriteContent(confname, level, color, colorend, file, func, line, levels,fmt, ap);
}

/*void ADVLOG_CALL AdvLog_AssignContent(const char *fmt, ...) {
	pthread_mutex_lock(&mutex);
	va_list ap;
	va_start(ap, fmt);
	strin.assign(fmt,ap);
	va_end(ap);
	pthread_mutex_unlock(&mutex);
}*/

void ADVLOG_CALL AdvLog_Print(char *confname, int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *fmt, ...) {
	pthread_mutex_lock(&mutex);
	va_list ap;
	va_start(ap, fmt);
	__AdvLog_Print(confname, level, color, file, func, line, levels, fmt, ap);
	va_end(ap);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_Write(char *confname, int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, ...) {
	pthread_mutex_lock(&mutex);
	va_list ap;
	va_start(ap, fmt);
	__AdvLog_Write(confname, level, color, colorend, file, func, line, levels, fmt, ap);
	va_end(ap);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_PrintAndWrite(char *confname, int level, int color, const char *colorstr, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, ...) {
	pthread_mutex_lock(&mutex);
	va_list ap;
	va_start(ap, fmt);
	__AdvLog_Write(confname, level, colorstr, colorend, file, func, line, levels, fmt, ap);
	__AdvLog_Print(confname, level, color, file, func, line, levels, fmt, ap);
	va_end(ap);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_VPrint(char *confname, int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	pthread_mutex_lock(&mutex);
	__AdvLog_Print(confname, level, color, file, func, line, levels, fmt, ap);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_VWrite(char *confname, int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	pthread_mutex_lock(&mutex);
	__AdvLog_Write(confname, level, color, colorend, file, func, line, levels, fmt, ap);
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_VPrintAndWrite(char *confname, int level, int color, const char *colorstr, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *fmt, va_list ap) {
	pthread_mutex_lock(&mutex);
	__AdvLog_Write(confname, level, colorstr, colorend, file, func, line, levels, fmt, ap);
	__AdvLog_Print(confname, level, color, file, func, line, levels, fmt, ap);
	pthread_mutex_unlock(&mutex);
}

time_t ADVLOG_CALL AdvLog_TimesUp(time_t check, time_t interval) {
	if (check <= 0) return TimeEventWithSecond(-1, interval);
	return TimeEventWithSecond(check, interval);
}

void ADVLOG_CALL AdvLog_ElsWrite( const char* DataPath,  const char *data, int len ) {
	pthread_mutex_lock(&mutex);
	__AdvLog_ElsWriteContent( DataPath, data, len );
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_Uninit(void) {
	AdvEls_Uninit();
	AdvLog_Exit();
}

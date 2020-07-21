#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <algorithm>
#include <deque>
//#include <queue>
#include "fplatform.h"
#include "tool.h"
#include "configure.h"
#include "rollingfile.h"
#include "tinydir.h"


//size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
static char pathname[256] = "logs";
static char lastfile[256];
static char filename[256];
static char fullname[256];
static FILE *logfile = NULL;
static int limit = 102400;
static int cacheSize = BUFSIZ;
static int currentcount;
static int staticGray = 0;
using namespace std;
static deque<string> rollinglist;

bool isDirExist(const std::string& path)
{
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else 
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

bool makePath(const std::string& target, int mode)
{
	std::string path = ReplaceAll(target, "\\", "/");
    int ret = mkdir(path.c_str(), mode);
    if (ret == 0)
        return true;

    switch (errno)
    {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
                return false;
            if (!makePath( path.substr(0, pos) , mode))
                return false;
        }
        // now, try to create again

        return 0 == mkdir(path.c_str(), mode);

    case EEXIST:
        // done!
        return isDirExist(path);

    default:
        return false;
    }
}

static char *GetAbsolutePath(char *path)
{
    char *sp = path;
    char workingdir[512]={0};
    static char temp[512]={0};
    char *p = strrchr(path,'/');

    memset(temp, 0, sizeof(temp));

    if( p ==  NULL )
        p = strrchr(path,'\\');

    if( p != NULL && ( ( p - sp ) >=2 || p == sp )) /* z:\,   / ,    */
        return path;
    else
    {		
        if( p == NULL )
        p = sp;   //  logs
        else if ( *(p+1) != '0' )
        ++p;  //  p=p+1  => /logs

        sp = getcwd(workingdir,512);
#if defined(_WIN32)
        snprintf(temp,sizeof(temp),"%s\\%s\\",sp,p);
#else
        snprintf(temp,sizeof(temp),"%s/%s/",sp,p);
#endif
    }
    return temp;
}

static bool filscmp (string l,string r) { 
    if(l.c_str()[0] == '_') {
        if(r.c_str()[0] == '_') {
            return (strcmp(l.c_str()+2,r.c_str()+2) < 0);
        } else {
            return (strcmp(l.c_str()+2,r.c_str()) < 0);
        }
    } else {
        if(r.c_str()[0] == '_') {
            return (strcmp(l.c_str(),r.c_str()+2) < 0);
        }
    }
    return (strcmp(l.c_str(),r.c_str()) < 0);
}

static void *RemoveOverFiles(void *data) {
	char *path = (char *)data;
    pthread_detach(pthread_self());
    deque<string> files;
    tinydir_dir dir;
    string abpath = GetAbsolutePath(path);
    string target;
    int i, count;
    tinydir_open(&dir, abpath.c_str());
    while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		
		if (!file.is_dir) {
            files.push_back(file.name);
		}

		tinydir_next(&dir);
	}
    tinydir_close(&dir);
    
    sort(files.begin(), files.end(), filscmp);
	i = files.size();
    count = i - AdvLog_Configure_GetFiles() + 1;
	count = count > i ? i : count;
    if(count > 0) {
        for( i = 0; i < count ; i ++ ) {
            target = abpath+files[i];
            remove(target.c_str());
        }
    }
    return (void *)0;
}

void RollFile_RemoveOverFiles(char *path)
{
    pthread_t pid;
    pthread_create(&pid, NULL, RemoveOverFiles, path);
}

inline static void RollFile_New() {
	int pid = AdvLog_Configure_GetPid();
	
	char timestr[256]= {0};
	//char checkfilename[256] = {0};
	time_t oldt = 0;
	time_t t = time(NULL);
	struct tm timetm;
	localtime_r(&t, &timetm);
	strftime(timestr, sizeof(timestr), "%Y-%m-%d_%H-%M-%S", &timetm);
    
	makePath(pathname,0777);
    RollFile_RemoveOverFiles(pathname);

	staticGray = AdvLog_Configure_GetStaticGray();

	do {
		if(oldt == t) sleep(1);
		t = time(NULL);
		localtime_r(&t, &timetm);
		strftime(timestr, sizeof(timestr), "%Y-%m-%d_%H-%M-%S", &timetm);
		snprintf(filename, sizeof(filename), "%s@%d", timestr, pid < 0 ? 0 : pid);
		if(staticGray == 0) {
			snprintf(fullname, sizeof(fullname), "%s/%s.html", pathname, filename);
		} else {
			snprintf(fullname, sizeof(fullname), "%s/%s.log", pathname, filename);
		}
		oldt = t;
	} while(access(fullname,F_OK) == 0);
	
	logfile = fopen(fullname,"w");
	if (logfile == NULL) {
		printf("***Can't create file [%s]***\n", fullname);
		logfile = stderr;
	} else chmod(fullname, 0777);

	cacheSize = AdvLog_Configure_GetCacheSize();
	if (cacheSize != BUFSIZ) setvbuf(logfile, NULL, _IOFBF, cacheSize);

	if(staticGray == 0 && logfile != NULL) { 
		fprintf(logfile,"<!DOCTYPE html><html><head>" ADV_HTML_CSS_STYLE "</head><body><pre>");
	}
}

void RollFile_Open(const char *path) {
	strncpy(pathname,path,sizeof(pathname));
}

void RollFile_RefreshConfigure() {
	limit = AdvLog_Configure_GetLimit();
	strncpy(pathname,AdvLog_Configure_GetPath(),sizeof(pathname));
}

inline static void RollFile_Rolling() {

	if(staticGray == 0 && logfile != NULL) { 
		fprintf(logfile,"</pre></body></html>");
	}
	
	if(logfile != NULL) {
		fclose(logfile);
		logfile = NULL;
	}
	
	if(staticGray == 0) {
		sprintf(lastfile,"%s/__%s%s",pathname,filename,".html");
	} else {
		sprintf(lastfile,"%s/__%s%s",pathname,filename,".log");
	}
	rename(fullname,lastfile);
	
    unsigned int number = AdvLog_Configure_GetNumber();
    //rolling by number
    //printf("@@@@@@@@@@@@@ rollinglist.size() = %d, number = %d\n", rollinglist.size(), number);
    if(rollinglist.size() < number) {
        rollinglist.push_back(lastfile);
    } else {
        rollinglist.push_back(lastfile);
        const char *garbage = rollinglist.front().c_str();
        if(strlen(garbage) != 0) { 
            unlink(garbage);
        }
        rollinglist.pop_front();
    }
    
    if(rollinglist.size() > number) {
        do {
            /*const char *garbage = rollinglist.front().c_str();
            if(strlen(garbage) != 0) { 
                unlink(garbage);
            }*/
            rollinglist.pop_front();
        } while(rollinglist.size() > number);
    }
    
	RollFile_RefreshConfigure();
	RollFile_New();
}

FILE *RollFile_Check() {
	if(logfile == NULL) {
		RollFile_New();
	}
	
	if(staticGray != AdvLog_Configure_GetStaticGray()) {
		RollFile_Rolling();
		currentcount = 0;
	}
	return logfile;
}

/*
void RollFile_StreamIn(const char *stream, int length) {
	if(logfile == NULL) {
		RollFile_New();
	}
	
	if(staticGray != AdvLog_Configure_GetStaticGray()) {
		RollFile_Rolling();
		currentcount = 0;
	}
	
	if(logfile != NULL) {
		currentcount += length;
		fwrite(stream, length, 1, logfile);
		fflush(logfile);
	}
	
	if(currentcount >= limit) {
		RollFile_Rolling();
		currentcount = 0;
	}
}
*/

void RollFile_Flush(int length) {
	static time_t lastupdate;
	time_t vaild = 0;
	if(logfile != NULL) {
		currentcount += length;
		if(currentcount > cacheSize) fflush(logfile);
		vaild = TimeEventWithSecond(lastupdate,10);
		if (vaild != 0) {
			lastupdate = vaild;
			fflush(logfile);
		}
	}
    
	if(currentcount >= limit) {
		fflush(logfile);
		RollFile_Rolling();
		currentcount = 0;
	}
}

void RollFile_Close() {
	if(logfile != NULL) {
		fclose(logfile);
		logfile = NULL;
	}
}

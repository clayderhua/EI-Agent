// mcafeelib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "stack_objinfo.h"
#include "mcafeelib.h"
/*--------------------------------
//
//	Parameters
//
//-------------------------------*/
/* Global variables */
int g_EngineStatus = STATE_STOPPED;
AV_INITRESULT g_initResult;
ScanResult g_scanResult;

/* Functions prototypes for this example */
AV_ERROR InitialiseEngine(char* filepath, AV_INITRESULT *init_result);
void BuildParameterList(AV_PARAMETERS *parameters);
int DisplayDATVersion(AV_INITRESULT *init_result);
void AddDatFile(const char * name , AV_DATSETFILES * pSet);
bool GetIniFileString( const char * file_name, const char * section, const char * key, char value[MAXPATH]);
bool BuildDeltaNames(char * ini_name, char* update_dir, AV_UAPPLY * dats, int current_dat);
int GetUpdateFileNames(char* ini_name, char* update_dir, int current_dat, char namelist[][MAXLENGTH], int *numUpdateFile);
bool BuildDatSet(const char * directory, AV_DATSETFILES ** pDat_set);
void FreeDatSet(AV_DATSETFILES * pset);
void ShowErrorBox(char* msg);
void RecordError(char*msg);
void WriteToFile(char* msg);

/* Callback function */
AV_LIBRET LIBFUNC API_Callback(HSCANNER, AV_MESSAGE, AV_PARAM1, AV_PARAM2);
MessageCallback g_MessageCallback = NULL;

/* The binaries provided by McAfee */
#define ENGINE_DIRECTORY ""
#define AV_NAMES "dat\\avvnames.dat"
#define AV_SCAN "dat\\avvscan.dat"
#define AV_CLEAN "dat\\avvclean.dat"

#define AV_MAX_INIT_PARAMS		10		/* The maximum number of parameters we will use to initialize the engine */
#define AV_MAX_SCAN_PARAMS		20		/* The maximum number of parameters we will use during scanning */

char datfilepath[3][MAX_PATH] = { 0 };
char enginepath[MAX_PATH] = { 0 };
/*Dyncmic load mcafee engine*/
typedef AV_ERROR (WINAPI *PAVInitialise)(AV_PARAMETERS* parameters, AV_INITRESULT* init_result);
typedef AV_ERROR (WINAPI *PAVClose)(HENGINE hengine);
typedef AV_ERROR (WINAPI *PAVScanObject)(HENGINE hengine, AV_PARAMETERS* parameters, AV_SCANRESULT* result);
typedef AV_ERROR (WINAPI *PAVUpdate)(AV_UAPPLY* update, AV_PARAMETERS* parameters);
PAVInitialise pAVInitialise = NULL;
PAVClose pAVClose = NULL;
PAVScanObject pAVScanObject = NULL;
PAVUpdate pAVUpdate = NULL;

HMODULE g_hMcAfeeDll = NULL;
void GetMcAfeeEngineFunction(void* hMcAfeeDll)
{
    if (hMcAfeeDll != NULL)
    {
        pAVInitialise = (PAVInitialise)GetProcAddress((HMODULE)hMcAfeeDll, "AVInitialise");
        pAVClose = (PAVClose)GetProcAddress((HMODULE)hMcAfeeDll, "AVClose");
        pAVScanObject = (PAVScanObject)GetProcAddress((HMODULE)hMcAfeeDll, "AVScanObject");
        pAVUpdate = (PAVUpdate)GetProcAddress((HMODULE)hMcAfeeDll, "AVUpdate");
    }
}

bool InitMcAfeeEngineLib(char* filepath)
{
    bool bRet = false;
    HMODULE hMcAfeeDll = NULL;
    WCHAR wsz[MAX_PATH] = { 0 };
    swprintf(wsz, L"%S\\mcscan32.dll", filepath);
    hMcAfeeDll = LoadLibrary(wsz);
    if (hMcAfeeDll != NULL)
    {
        g_hMcAfeeDll = hMcAfeeDll;
        GetMcAfeeEngineFunction(hMcAfeeDll);
        bRet = true;
    }
    return bRet;
}

bool CleanupMcAfeeEngineLib()
{
    bool bRet = true;

    if (g_hMcAfeeDll != NULL)
    {
        FreeLibrary(g_hMcAfeeDll);
        g_hMcAfeeDll = NULL;
        pAVInitialise = NULL;
        pAVClose = NULL;
        pAVScanObject = NULL;
        pAVUpdate = NULL;
    }

    return bRet;
}

/*--------------------------------
//
//	APIs
//
//-------------------------------*/
MCAFEELIB_API Status_t WINAPI InitializeMcAfeeEngine(char* filepath)
{
	AV_ERROR error = AVE_CRITICALERROR;
	char error_str[100];

	if (g_EngineStatus != STATE_STOPPED)
		return ADV_STATUS_ENGINE_INITIALIZED;

    if(g_hMcAfeeDll == NULL)
        InitMcAfeeEngineLib(filepath);

	memset(&g_initResult, 0, sizeof(AV_INITRESULT));
    g_initResult.structure_size = sizeof(AV_INITRESULT);

	error = InitialiseEngine(filepath, &g_initResult);
    if (error != AVE_SUCCESS)
    {
		sprintf(error_str, "Engine failed to initialize (error: %d)\n", (int)error);
		RecordError(error_str);
        return ADV_STATUS_ERROR;
    }

	g_EngineStatus = STATE_STATRED;
	return ADV_STATUS_SUCCESS;
}

MCAFEELIB_API Status_t WINAPI UnitializeMcAfeeEngine(bool bForce)
{
    AV_ERROR error;
	if (g_EngineStatus != STATE_STATRED)
		return ADV_STATUS_ENGINE_NOT_INITIALIZED;

	//AVClose(g_initResult.engine_handle);
    if(pAVClose)
        error = pAVClose(g_initResult.engine_handle);

    if (error != AVE_SUCCESS)
    {
        char error_str[100] = { 0 };
        sprintf(error_str, "Engine failed to uninitialize (error: %d)\n", (int)error);
        RecordError(error_str);
        return ADV_STATUS_ERROR;
    }

    if(bForce)
        CleanupMcAfeeEngineLib();
	g_EngineStatus = STATE_STOPPED;
	return ADV_STATUS_SUCCESS;
}

MCAFEELIB_API Status_t WINAPI ScanDrives(char* driverletters, int amount, ScanResult &result)
{
	if (g_EngineStatus != STATE_STATRED)
		return ADV_STATUS_ENGINE_NOT_INITIALIZED;

	if (driverletters == NULL)
		return ADV_STATUS_INVALID_PARAMETER;

	if (g_MessageCallback == NULL)
		return ADV_STATUS_ERROR;

	/* Structures used for scanning */
	AV_PARAMETERS scan_parameters;
	AV_SINGLEPARAMETER parameters[AV_MAX_SCAN_PARAMS];
	AV_OBJECT object;
	AV_ERROR error;
	char error_str[100];
	char drive [10];
	int i;

	/* Structures used for record scan result */
	g_scanResult.total_count = 0;
	g_scanResult.infected_count = 0;

	/* Initialize scanning structures */
    memset(&scan_parameters, 0, sizeof(AV_PARAMETERS));
    scan_parameters.structure_size = sizeof(AV_PARAMETERS);
    scan_parameters.parameters = parameters;
    
    /* Add any parameters needed for scanning. */
    BuildParameterList(&scan_parameters);

    memset(&object, 0, sizeof(AV_OBJECT));
    object.structure_size = sizeof(AV_OBJECT);
    object.pcontext = NULL;

    /* This parameter holds information about scanned object */
    /* It needs to be added to the parameters list only once */
    /*  (we will update the structure pointed to by the parameter instead) */
    AVAddParameter( scan_parameters.parameters,
                    scan_parameters.nparameters,
                    AVP_OBJECT,
                    (void*)&object,
                    sizeof(AV_OBJECT)
                    );

	for(i = 0; i < amount; i++)
    {
		if (driverletters[i] == '\0')
			break;

		sprintf(drive, "%c:\\*.*", driverletters[i]);

		/* Set up the current object to scan */
		object.type = AVOT_DIRECTORY;
		/* The path to the file will be provided to the engine */
		object.subtype = AVOS_DOSPATH;
		/* Full path of the file to scan */
		object.pAttribute = (void *)drive;
		object.size = (DWORD)strlen(drive) + 1;

        if (pAVScanObject)
        {
            initstack_obj();
            //error = AVScanObject(g_initResult.engine_handle, &scan_parameters, NULL);
            error = pAVScanObject(g_initResult.engine_handle, &scan_parameters, NULL);
            unitstack_obj();
        }
		

		if (error != AVE_SUCCESS)
		{
			sprintf(error_str, "Engine failed to scan system (error: %d)\n", (int)error);
			RecordError(error_str);
			return ADV_STATUS_ERROR;
		}
	}
	result.total_count = g_scanResult.total_count;
	result.infected_count = g_scanResult.infected_count;

	return ADV_STATUS_SUCCESS;

}

MCAFEELIB_API Status_t WINAPI GetCurrentVirusDefVersion(int &version)
{
	if (g_EngineStatus != STATE_STATRED)
		return ADV_STATUS_ENGINE_NOT_INITIALIZED;

	version = DisplayDATVersion(&g_initResult);
	return ADV_STATUS_SUCCESS;
}

MCAFEELIB_API Status_t WINAPI UpdateAutoInfo(char* updateDirectory, int currentVersion, UpdateInfo &info)
{
	int canUpdate;
	char updateFileList[UPDATEMAXNUM][MAXLENGTH];
	int numUpdateFile = 0;
	char* infoFileName = "gdeltaavv.ini";

	canUpdate = GetUpdateFileNames(infoFileName, updateDirectory, currentVersion, updateFileList, &numUpdateFile);

	info.canUpdate = canUpdate;
	info.numUpdateFile = numUpdateFile;
	for (int i = 0; i < numUpdateFile; i++)
		strcpy(info.updateFileList[i], updateFileList[i]);

	return ADV_STATUS_SUCCESS;
}

MCAFEELIB_API Status_t WINAPI UpdateAutoVirusDef(char* updateDirectory, int currentVersion)
{
	/* Structure to contain update details */
    AV_UAPPLY     dats;
    AV_ERROR error = 0;
    char error_str[100] = { 0 };
    char datpath[MAXPATH] = { 0 };
	char* infoFileName = "gdeltaavv.ini";
    bool can_be_updated = false; // the current version is up-to-date, and can be updated

	dats.structure_size = sizeof(dats);
    dats.dat_set = NULL;
    dats.updated_set = NULL;
    dats.updates = NULL;

	can_be_updated = BuildDeltaNames(infoFileName, updateDirectory, &dats, currentVersion);
	
    sprintf(datpath, "%sdat", enginepath);
	BuildDatSet(datpath, &dats.dat_set);
    BuildDatSet(updateDirectory, &dats.updated_set);

	if(can_be_updated && pAVUpdate)
    {
        AV_PARAMETERS update_params;
        AV_SINGLEPARAMETER parameters[AV_MAX_INIT_PARAMS];

        memset(&update_params, 0, sizeof(AV_PARAMETERS));
        update_params.structure_size = sizeof(AV_PARAMETERS);

        update_params.parameters = parameters;

        /* Add a callback function */
        AVAddParameter( update_params.parameters,
                        update_params.nparameters,
                        AVP_CALLBACKFN,
                        (void *)API_Callback,
                        sizeof(void *)
            );

        /* Tell the engine the location of the engine binaries */
        AVAddParameter( update_params.parameters,
                        update_params.nparameters,
                        AVP_ENGINELOCATION,
                        (void *)enginepath,
                        (strlen(enginepath) + 1)
            );

        AVAddParameter(update_params.parameters,
                    update_params.nparameters,
                    AVP_APIVER,
                    (void *)AV_APIVER,
                    sizeof(void *)
                    );

        //error = AVUpdate(&dats, &update_params);
        error = pAVUpdate(&dats, &update_params);

        /* Free memory allocated from above */
        FreeDatSet(dats.dat_set);
        FreeDatSet(dats.updates);
        FreeDatSet(dats.updated_set);

    }
    

	if (can_be_updated)
		if (error == 0)
			return ADV_STATUS_SUCCESS;

	sprintf(error_str, "Engine failed to update (error: %d)\n", (int)error);
	RecordError(error_str);
	return ADV_STATUS_ERROR;
}

MCAFEELIB_API Status_t WINAPI SetCallback(MessageCallback messageCallback)
{
	if (messageCallback)
		g_MessageCallback = messageCallback;
	else
		return ADV_STATUS_ERROR;

	return ADV_STATUS_SUCCESS;
}


/*--------------------------------
//
//	Local functions
//
//-------------------------------*/
/* Function to build engine initialization parameters
 * and initialize the engine. This assumes init_result
 * has been allocated and initialised.
 */
AV_ERROR InitialiseEngine(char* filepath, AV_INITRESULT *init_result)
{
    AV_PARAMETERS init_params;
    AV_SINGLEPARAMETER parameters[AV_MAX_INIT_PARAMS];
    AV_ERROR error;

    AV_DATSETFILES av_dat_set;
    char datpath[MAX_PATH] = { 0 };

    const char *av_dat_names[3];
    
    /*av_dat_names[0] = AV_NAMES;
    av_dat_names[1] = AV_SCAN;
    av_dat_names[2] = AV_CLEAN;*/
    sprintf(datfilepath[0], "%s%s", filepath, AV_NAMES);
    sprintf(datfilepath[1], "%s%s", filepath, AV_SCAN);
    sprintf(datfilepath[2], "%s%s", filepath, AV_CLEAN);
    av_dat_names[0] = datfilepath[0];
    av_dat_names[1] = datfilepath[1];
    av_dat_names[2] = datfilepath[2];

    memset(&av_dat_set, 0, sizeof(AV_DATSETFILES));
    av_dat_set.structure_size = sizeof(AV_DATSETFILES);
    av_dat_set.read_type = AV_READTYPE_DIRECT;
    av_dat_set.datfile_count = 3;
    av_dat_set.datfiles.datfile_names = av_dat_names;

    /* Initialize all structures */
    memset(&init_params, 0, sizeof(AV_PARAMETERS));
    init_params.structure_size = sizeof(AV_PARAMETERS);
    
    init_params.parameters = parameters;
    
    /* Add a callback function */
    AVAddParameter( init_params.parameters,
                    init_params.nparameters,
                    AVP_CALLBACKFN,
                    (void *)API_Callback,
                    sizeof(void *)
                    );
       
    AVAddParameter( init_params.parameters,
                    init_params.nparameters,
                    AVP_VIRUSDATSET,
                    (void *)&av_dat_set,
                    sizeof(AV_DATSETFILES));

    /* Tell the engine the location of the engine binaries */
    AVAddParameter( init_params.parameters,
                    init_params.nparameters,
                    AVP_ENGINELOCATION,
                    (void *)filepath,
                    ( strlen(filepath) + 1 )
                    );

    AVAddParameter( init_params.parameters,
                    init_params.nparameters,
                    AVP_APIVER,
                    (void *)AV_APIVER,
                    sizeof(void *)
                    );

    /* Then initialize */
    //error = AVInitialise(&init_params,init_result);
    if(pAVInitialise)
        error = pAVInitialise(&init_params, init_result);
    strcpy(enginepath, filepath);
    return error;
}


/* Function to add any parameters that the scan needs.
 * It does not add a target to scan.
 */
void BuildParameterList(AV_PARAMETERS *parameters)
{
    /* The callback function */
    AVAddParameter( parameters->parameters,
                    parameters->nparameters,
                    AVP_CALLBACKFN,
                    (void *)API_Callback,
                    sizeof(void *)
                    );

    /* Scan for everything */
    AVAddBoolParameter( parameters->parameters,
                        parameters->nparameters,
                        AVP_SCANALLFILES
                        );

	    /* Scan inside archives */
    AVAddBoolParameter( parameters->parameters,
                        parameters->nparameters,
                        AVP_DECOMPARCHIVES
                        );

    /* Scan inside packed files */
    AVAddBoolParameter( parameters->parameters,
                        parameters->nparameters,
                        AVP_DECOMPEXES
                        );

	/* Scan all sub-directories in the specified directory */
    AVAddBoolParameter( parameters->parameters,
                        parameters->nparameters,
                        AVP_RECURSESUBDIRS
                        );

	/* Clean any infections */
    AVAddBoolParameter( parameters->parameters,
                        parameters->nparameters,
                        AVP_REPAIR
                        );
}

int DisplayDATVersion(AV_INITRESULT *init_result)
{
    if (init_result->datset_count > 0)
    {
        const AV_DATSETSTATUS *dat_status = init_result->datset_info[0];
        printf("Engine Initialised using DAT version: %d.%d\n", (int)dat_status->major_version, (int)dat_status->minor_version);

		return (int)dat_status->major_version;
    }
	return 0;
}

/* AddDatFile : adds name to the current dat set.
 */
void AddDatFile(const char * name , AV_DATSETFILES * pSet)
{
    int i = 0;
    const char ** new_set = NULL;
    const char ** old_set = NULL;
    int old_size = 0;
    int new_size = 0;
    old_size = pSet->datfile_count;
    new_size = old_size + 1;
    new_set = (const char **)malloc(new_size * sizeof(char *));
    old_set = pSet->datfiles.datfile_names;

    /* Copy the old array into the new array */
    if(new_set)
    {

        for(i = 0;i < old_size;i++)
        {
            new_set[i] = old_set[i];
        }
        /* create a memory copy of the new name */
        new_set[old_size] = strdup(name);

        pSet->datfiles.datfile_names = new_set;
        pSet->datfile_count = new_size;
    } else {
        /* Free the old data */
        pSet->datfiles.datfile_names = NULL;
        pSet->datfile_count = 0;
    }
    if(old_set)
    {
        free(old_set);
    }
}

/* GetIniFileString. Read a standard ini file (a section with a number of key/value pairs
 */
bool GetIniFileString( const char * file_name, const char * section, const char * key, char value[MAXPATH])
{
    char buffer2[MAXPATH];
    char buffer[MAXPATH];
    bool rightSection = false;
    char SectionCheck[MAXPATH];
    char ValueCheck[MAXPATH];

    char test_ch;

    FILE * f = fopen (file_name ,"rt");
    if(f == NULL) 
	{
		printf("Error: %s \n", strerror(errno));
		return false;
	}


    sprintf(SectionCheck, "[%s]", section);
    sprintf(ValueCheck, "%s=%%[^\n]", key);


    while(fgets(buffer2, sizeof(buffer2),f) != NULL)
    {
        /* Remove \r and \n from end of buffer */
        sscanf(buffer2, "%[^\r\n]", buffer);

        if(strcmp(SectionCheck, buffer)  == 0)
        {
            rightSection = true;
        } else
            /* test for [ - means leaving current section */
            if (sscanf(buffer, "[%c", &test_ch) == 1)
        {
            rightSection = false;

        } else {
            /* Are we in the correct section, and is the key the correct key */
            if(rightSection && sscanf(buffer, ValueCheck, value ) == 1)
            {
                fclose(f);
                return true;
            }
        }
    }
    fclose(f);

    return false;
}

/* Fill out the AV_UAPPLY with the information needed to
 * update from the current dat version to the version specified by the gdelatavv.ini
 */
bool BuildDeltaNames(char * ini_name, char* update_dir, AV_UAPPLY * dats, int current_dat)
{
    char Buffer[MAXPATH] = { 0 };
    char current_dat_str[20] = { 0 };
	char ini_path[MAXPATH] = { 0 };
	char gem_path[MAXPATH] = { 0 };
    int lowIncrement = 0;
    int lastIncrement = 0;

    AV_DATSETFILES * pUpdates = NULL;

    /* Build out string value to retrieve from ini file */
	sprintf(ini_path, "%s\\%s", update_dir, ini_name);
    sprintf(current_dat_str, "%d", current_dat);

    /* allocate a new AV_DATSETFILES structure. */
    pUpdates = (AV_DATSETFILES *)malloc(sizeof(AV_DATSETFILES));
    if(pUpdates)
    {
        pUpdates->structure_size = sizeof(AV_DATSETFILES);
        pUpdates->read_type  = AV_READTYPE_DIRECT;
        pUpdates->datfile_count = 0;
        pUpdates->datfiles.datfile_names = NULL;
    }

    if(GetIniFileString(ini_path, "Contents", "LastIncremental", Buffer))
    {
        lastIncrement = atoi(Buffer);
    } else {
        free(pUpdates);
        return false;
    }

    if(GetIniFileString(ini_path, "Multiple Patch Table", current_dat_str, Buffer))
    {
        lowIncrement = atoi(Buffer);
    } else {
        /* Unable to apply incrementals on this build */
        free(pUpdates);
        return false;
    }

    /* Is the gdeltaavv.ini correct for this version of the dats? */
    if(lowIncrement != 0 && lastIncrement != 0)
    {
        int i;

        for(i = lowIncrement; i <= lastIncrement; i++)
        {
            char id[MAXPATH];
            sprintf(id, "%d", i);

            if(GetIniFileString(ini_path, "Incremental Resolver", id, Buffer))
            {
                sprintf(gem_path, "%s\\%s", update_dir, Buffer);
				printf("adding : %s\n", gem_path);
                AddDatFile(gem_path, pUpdates);
            } else {
                fprintf(stderr, "Failed to find lookup at %d\n", i);
                return false;
            }
        }
    }

    dats->updates = pUpdates;
    return true;
}

int GetUpdateFileNames(char* ini_name, char* update_dir, int current_dat, char namelist[][MAXLENGTH], int *numUpdateFile)
{
	// 0: fail, 1: success, 2: already newest
    char Buffer[MAXPATH];
    char current_dat_str[20];
	char ini_path[MAXPATH];
	char gem_file[MAXPATH] = {0};
    int lowIncrement;
    int lastIncrement;
	int newestVersion = 0;
	int num = 0;

	enum update_status
	{
		UPDATE_SUCCESS,		// 0
		UPDATE_FAIL,		// 1
		UPDATE_NEWALREADY,	// 2
	};

    /* Build out string value to retrieve from ini file */
	sprintf(ini_path, "%s\\%s", update_dir, ini_name);
    sprintf(current_dat_str, "%d", current_dat);

	if(GetIniFileString(ini_path, "Contents", "CurrentVersion", Buffer))
    {
        newestVersion = atoi(Buffer);
		if (current_dat == newestVersion)
			return UPDATE_NEWALREADY;
    } else {
        newestVersion = 0;
        return UPDATE_FAIL;
    }

    if(GetIniFileString(ini_path, "Contents", "LastIncremental", Buffer))
    {
        lastIncrement = atoi(Buffer);
    } else {
        lastIncrement = 0;
        return UPDATE_FAIL;
    }

    if(GetIniFileString(ini_path, "Multiple Patch Table", current_dat_str, Buffer))
    {
        lowIncrement = atoi(Buffer);
    } else {
        /* Unable to apply incrementals on this build */
        lowIncrement = 0;
        return UPDATE_FAIL;
    }
	int length = 0;
    /* Is the gdeltaavv.ini correct for this version of the dats? */
    if(lowIncrement != 0 && lastIncrement != 0)
    {
        int i;
		
        for(i = lowIncrement; i <= lastIncrement; i++)
        {
            char id[MAXPATH];
            sprintf(id, "%d", i);

            if(GetIniFileString(ini_path, "Incremental Resolver", id, Buffer))
            {
				sprintf(gem_file, "%s", Buffer);
				//strcpy(namelist[num], gem_file);

				
				for(length = 0; length < MAXLENGTH; length ++)
					namelist[num][length] = gem_file[length];

				num++;
            } else {
                return UPDATE_FAIL;
            }
        }
    }
	*numUpdateFile = num;

    return UPDATE_SUCCESS;
}

/* Given a directory, build a AV_DATSETFILES stucture to define the V2 dats for direct IO.
 */ 
bool BuildDatSet(const char * directory, AV_DATSETFILES ** pDat_set)
{
    AV_DATSETFILES * pDats = NULL;
    bool result = false;
    char dat_name[MAXPATH];

    pDats = (AV_DATSETFILES *) malloc(sizeof(AV_DATSETFILES));
    if(pDats)
    {
        const char ** names = NULL;
        names = (const char **) malloc(3*sizeof(char *));
        if(names != NULL)
        {
            pDats->structure_size = sizeof(AV_DATSETFILES);
            pDats->read_type = AV_READTYPE_DIRECT;
            pDats->datfile_count = 3;
            pDats->datfiles.datfile_names = names;
            sprintf(dat_name, "%s\\avvscan.dat", directory);
            names[0] = strdup(dat_name );

            sprintf(dat_name, "%s\\avvnames.dat", directory);
            names[1] = strdup(dat_name );

            sprintf(dat_name, "%s\\avvclean.dat", directory);
            names[2] = strdup(dat_name);
            result = true;
        } else {
            free(pDats);
        }
    }

    if(result == true)
        *pDat_set = pDats;

    return result;
}

/* FreeDatSet, Given a dat set, free the allocated memory.
 */
void FreeDatSet(AV_DATSETFILES * pset)
{
    int i;

    for(i =0; i < pset->datfile_count; i++)
    {
        free((char *)pset->datfiles.datfile_names[i]);
    }

    free(pset->datfiles.datfile_names);
    free(pset);
}

static unsigned int getcharlength(char *s)
{
	unsigned int i = 0;
    while (s[i] != '\0')
        ++i;

    return i;
}

void GetObjectInfo()
{
	object_info obj;
	ScanLog log;
	int len = 0;
	log.filename = NULL;
	log.scan_status = NULL;
	log.repair_status = NULL;

	g_scanResult.total_count++;

	do
	{
		pop_obj(obj);

		switch (obj.obj_type)
		{
		case filename:
			log.filename = (unsigned char*)malloc(strlen(obj.value)+1); 
			memcpy(log.filename, obj.value, strlen(obj.value)+1);
			break;
		case scan_status:
			log.scan_status = (char*)malloc(getcharlength(obj.value)+1); 
			strcpy(log.scan_status, obj.value);
			
			break;
		case repair_status:
			log.repair_status = (char*)malloc(getcharlength(obj.value)+1); 
			strcpy(log.repair_status, obj.value);
			
			if (strcmp(obj.value, "norep") == 0) 
				g_scanResult.infected_count++;
			break;
		default:
			printf("GetObjectInfo() error!\n");
			break;
		}
	}while (obj.obj_type != filename);

	
	try
	{
		printf("%s\t%s\t%s\n", log.filename, (log.scan_status)?log.scan_status:"", (log.repair_status)?log.repair_status:"");
		g_MessageCallback(log);
	} 
	catch (char* message)
	{
		printf("[C Exception] %s", message);
	}

	if(log.filename != NULL)
	{
		free(log.filename);
		log.filename = NULL;
	}

	if(log.scan_status != NULL)
	{
		free(log.scan_status);
		log.scan_status = NULL;
	}

	if(log.repair_status != NULL)
	{
		free(log.repair_status);
		log.repair_status = NULL;
	}

	if(obj.value != NULL)
	{
		free(obj.value);
		obj.value = NULL;
	}

}

/* Callback function gets messages from the engine and acts upon them ... */
AV_LIBRET LIBFUNC API_Callback(HSCANNER hengine, AV_MESSAGE _message, AV_PARAM1 _p1, AV_PARAM2 _p2)
{
    /* Default return value */
    AV_LIBRET returnvalue = 0;
	object_info obj;

    switch(_message)
    {
        /* Miscellaneous messages */
		case AVM_DAT_VERSION:
            printf( "About to update from %ld\n", (long)(_p2.dwValue) );
            break;
        case AVM_UPDATE_OK:
            printf( "dats updated OK to %ld.\n", (long)(_p2.dwValue) );
            break;
        case AVM_QUERYUPDATEDENY:
            printf( "Query deny update\n" );
            break;
        case AVM_UPDATE_ERROR:
            printf( "Update error!(%ld)\n" , (long)(_p2.dwValue));
            break;

        /* Status messages */

        case AVM_OBJECTNAME:
            //printf("%s ... ",(const char*)(_p2.pValue));
			obj = setobj(filename, (char*)(_p2.pValue));
			push_obj(obj);

            break;

        case AVM_OBJECTSIZE:
            //printf("[%llu bytes] ", _p2.qwValue);
            break;

        case AVM_OBJECTREPAIRED:
            //printf("repaired\n");
			obj = setobj(repair_status, "rep");
			push_obj(obj);
            break;

        case AVM_OBJECTNOTSCANNED:
            //printf("*not* scanned (error: %lu)\n",(unsigned long)(_p2.dwValue));
            break;

		case AVM_OBJECTNOTREPAIRED:
			//printf("not repaired\n");
			obj = setobj(repair_status, "norep");
			push_obj(obj);
            break;

        case AVM_OBJECTSUMMARY:
            if (_p2.dwValue == AV_SUMMARY_OBJECTOK)
			{
                //printf("is OK\n");
				obj = setobj(scan_status, "ok");
				push_obj(obj);
			}
			GetObjectInfo();
            break;

        case AVM_OBJECTINFECTED:
            //printf("is infected with %s\n",((AV_INFECTION*)_p2.pValue)->virus_name);
            obj = setobj(scan_status, ((AV_INFECTION*)_p2.pValue)->virus_name);
			push_obj(obj);
			break;

        case AVM_OBJECTCLOSED:
        case AVM_OBJECTSTART:
            break;

		case AVM_CONTAINERSTART:
			if ((unsigned)_p1 == 4 && (unsigned long)(_p2.dwValue) == 0)
				clearstack_obj();
			break;

        /* Query messages ... */

        case AVM_QUERYTERMINATE:
        case AVM_QUERYDENYSCAN:
        case AVM_QUERYDENYREPAIR:
        case AVM_QUERYQUITSCANNING:
            /* Let them drop through */
            break;

        /* Fall-through case */

        default:
            //printf("Received message 0x%x p1:0x%x p2:0x%lx\n\r",(unsigned)_message,(unsigned)_p1,(unsigned long)(_p2.dwValue));
            break;
        
    }

    return returnvalue;
}

void RecordError(char*msg)
{
#ifdef _DEBUG
	WriteToFile(msg);
	ShowErrorBox(msg);
#endif
}

void ShowErrorBox(char* msg)
{
	wchar_t wmsg[100];
	LPWSTR ptr;

	mbstowcs(wmsg, msg, strlen(msg)+1);//Plus null
	ptr = wmsg;

	MessageBox(NULL, ptr,(LPCWSTR)L"McAfee library errors", MB_OK | MB_ICONERROR);
}

void WriteToFile(char* msg)
{
	FILE *fptr;
	fptr = fopen("tool_log.txt", "w");
	if(fptr == NULL)
		return;

	fprintf(fptr, "%s\n", msg);
	fclose(fptr);
}

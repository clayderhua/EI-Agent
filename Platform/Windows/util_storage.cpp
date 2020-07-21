#include "util_storage.h"
#include <windows.h>

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
#include <regex>

#pragma comment(lib, "wbemuuid.lib")

WISEPLATFORM_API DiskItemList* util_storage_getdisklist()
{
	DiskItemList* disklist = NULL;
	DiskItemList* last = NULL;
	char drive[] = "A:\\";
	int diskIndex = 0;

	for (; diskIndex < MAX_ITEM_COUNT; diskIndex++)
	{
		drive[0] = 'A' + diskIndex;
		if ((GetDriveType(drive) == DRIVE_FIXED) || (GetDriveType(drive) == DRIVE_REMOVABLE))
		{
			DiskItemList* disk = (DiskItemList*)malloc(sizeof(DiskItemList));
			memset(disk, 0, sizeof(DiskItemList));
			disk->disk = (DiskInfoItem*)malloc(sizeof(DiskInfoItem));
			memset(disk->disk, 0, sizeof(DiskInfoItem));
			strncpy(disk->disk->name, drive, 2);

			if(disklist == NULL)
				last = disklist = disk;
			else{
				while(last)
				{
					if(last->next)
						last = last->next;
					else
						break;
				}

				last->next = disk;
			}

		}
	}
	return disklist;
}

WISEPLATFORM_API void util_storage_freedisklist(DiskItemList* list)
{
	while(list)
	{
		DiskItemList* current = list;
		list = list->next;
		if(current->disk)
			free(current->disk);
		free(current);
	}
}

WISEPLATFORM_API unsigned long long util_storage_gettotaldiskspace(char* drive)
{
	unsigned long long value = 0;
	char newdrive[4] = {drive[0], ':', '\\', '\0'};
	// The GetDiskFreeSpaceEx function returns zero (0) for lpTotalNumberOfFreeBytes and 
	// lpFreeBytesAvailable for all CD requests unless the disk is an unwritten CD in a CD-RW drive.
	if(GetDiskFreeSpaceEx(newdrive, NULL, (PULARGE_INTEGER)&value, NULL)==FALSE)
		value = 0;
	return value;
}

WISEPLATFORM_API unsigned long long util_storage_getfreediskspace(char* drive)
{
	unsigned long long value = 0;
	char newdrive[4] = {drive[0], ':', '\\', '\0'};
	// The GetDiskFreeSpaceEx function returns zero (0) for lpTotalNumberOfFreeBytes and 
	// lpFreeBytesAvailable for all CD requests unless the disk is an unwritten CD in a CD-RW drive.
	if(GetDiskFreeSpaceEx(newdrive, (PULARGE_INTEGER)&value, NULL, NULL)==FALSE)
		value = 0;
	return value;
}

WISEPLATFORM_API int util_storage_getdiskspeed(DiskItemList* list)
{
    HRESULT hres;

    if (list == NULL)
        return 0;



    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    //hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    //if (FAILED(hres))
    //{
    //    cout << "Failed to initialize COM library. Error code = 0x"
    //        << hex << hres << endl;
    //    return 1;                  // Program has failed.
    //}

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    //hres = CoInitializeSecurity(
    //    NULL,
    //    -1,                          // COM authentication
    //    NULL,                        // Authentication services
    //    NULL,                        // Reserved
    //    RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
    //    RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
    //    NULL,                        // Authentication info
    //    EOAC_NONE,                   // Additional capabilities 
    //    NULL                         // Reserved
    //);

    //Ignore error by Scott
    //if (FAILED(hres))
    //{
    //    cout << "Failed to initialize security. Error code = 0x"
    //        << hex << hres << endl;
    //    CoUninitialize();
    //    return 1;                    // Program has failed.
    //}

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << hres << endl;
        //CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x"
            << hex << hres << endl;
        pLoc->Release();
        //CoUninitialize();
        return false;                // Program has failed.
    }

    //cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x"
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        //CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_PerfFormattedData_PerfDisk_PhysicalDisk"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        cout << "Query for operating system name failed."
            << " Error code = 0x"
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        //CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    BOOL bSkip = FALSE;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;
        CIMTYPE type;
        hr = pclsObj->Get(L"DiskReadBytesPersec", 0, &vtProp, &type, 0);
        long long readspeed = _wtoll(vtProp.bstrVal);
        //cout << " OS DiskReadBytesPersec : " << readspeed << endl;
        VariantClear(&vtProp);

        hr = pclsObj->Get(L"DiskWriteBytesPersec", 0, &vtProp, &type, 0);
        long long writespeed = _wtoll(vtProp.bstrVal);
        //cout << " OS DiskWriteBytesPersec : " << writespeed << endl;
        VariantClear(&vtProp);

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        string text = (_bstr_t)vtProp.bstrVal;
        //cout << " OS Name : " << text << endl;
        VariantClear(&vtProp);

        std::regex ws_re("\\s+"); // whitespace
        std::vector<std::string> v(std::sregex_token_iterator(text.begin(), text.end(), ws_re, -1),
            std::sregex_token_iterator());
        int diskid = -1;
        for (int i = 0; i < v.size(); i++)
        {
            if (i == 0)
            {
                if (v[0] == "_Total")
                {
                    bSkip = TRUE;
                    break;
                }

                diskid = std::stoi(v[0]);
                //std::cout << "diskid" << diskid << "\n";
            }
            else
            {
                DiskItemList* current = list;
                while (current)
                {
                    if (strcmp(current->disk->name, v[i].c_str()) == 0)
                    {
                        current->disk->diskid = diskid;
                        current->disk->readpersec = readspeed;
                        current->disk->writepersec = writespeed;
                    }
                    current = current->next;
                }
            }
        } 
        pclsObj->Release();
    }

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    //CoUninitialize();

    return 0;   // Program successfully completed.
}
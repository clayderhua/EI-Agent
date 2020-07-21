#include "WMIHelper.h"

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
#include <regex>

#pragma comment(lib, "wbemuuid.lib")

#define INVALID_DEVMON_VALUE  (-999)

ram_node_list wmi_GetMemorySpeed()
{
	HRESULT hres;

	ram_node_list list = NULL;

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
		return list;                 // Program has failed.
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
		return list;                // Program has failed.
	}

	cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


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
		return list;               // Program has failed.
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM  Win32_PhysicalMemory"),
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
		return list;               // Program has failed.
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;
	BOOL bSkip = FALSE;
	ram_node_t* prev = list;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		hr = pclsObj->Get(L"Speed", 0, &vtProp, NULL, 0);
		unsigned long speed = vtProp.iVal;
		std::cout << " Memory Speed : " << speed << endl;
		VariantClear(&vtProp);

		// Get the value of the Name property
		hr = pclsObj->Get(L"Tag", 0, &vtProp, 0, 0);
		string text = (_bstr_t)vtProp.bstrVal;
		std::cout << " Memory Tag : " << text << endl;
		VariantClear(&vtProp);

		std::regex ws_re("\\s+"); // whitespace
		std::vector<std::string> v(std::sregex_token_iterator(text.begin(), text.end(), ws_re, -1),
			std::sregex_token_iterator());
		int ramid = -1;
		for (int i = 0; i < v.size(); i++)
		{
			if (v[i] == "Physical")
			{
				continue;
			}
			else if (v[i] == "Memory")
			{
				continue;
			}
			else
			{
				int index = std::stoi(v[i]);
				ram_node_t* current = (ram_node_t*)calloc(1, sizeof(ram_node_t));
				current->Index = index;
				current->Speed = speed;

				if (prev == NULL)
				{
					list = current;
				}
				else
				{
					prev->next = current;
				}
				prev = current;
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

	return list;   // Program successfully completed.
}

unsigned int wmi_FindRAMSpeed(ram_node_list list, int index)
{
	unsigned int speed = INVALID_DEVMON_VALUE;
	ram_node_t* current = list;
	while (current)
	{
		if (current->Index == index)
		{
			speed = current->Speed;
			break;
		}
		current = current->next;
	}
	return speed;
}

void wmi_ReleaseRAMList(ram_node_list list)
{
	ram_node_t* prevone = list;
	while (prevone)
	{
		ram_node_t* tmpnode = prevone;
		prevone = tmpnode->next;
		free(tmpnode);
		tmpnode = NULL;
	}
}
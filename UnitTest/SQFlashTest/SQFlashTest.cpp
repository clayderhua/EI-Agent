// SQFlashTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdbool.h>
#include "sqflashhelper.h"
#include "McAfeeHelper.h"

int main()
{
    std::cout << "Hello World!\n";
    /*Not Ready
    if (hdd_StartupSQFlashLib())
    {
        hdd_SelfTest(0);
        hdd_CleanupSQFlashLib();
    }
    */
    int version = 0;
    if (mc_Initialize((char*)"C:\\WISEAgent\\mcafee\\", &version))
    {
        std::cout << "McAfee Version: " << version << std::endl;

        bool bUpdate = mc_CheckUpdate();
        int total = 0;
        int infected = 0;
        mc_StartScan((char*)"F", 1, &total, &infected);

        mc_Uninitialize();
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

#pragma once
#ifndef _WMI_HELPER_H_
#define _WMI_HELPER_H_

typedef struct ram_node_t {
	int Index;
	unsigned int Speed;
	struct ram_node_t* next;
}ram_node_t;

typedef ram_node_t* ram_node_list;

ram_node_list wmi_GetMemorySpeed();
unsigned int wmi_FindRAMSpeed(ram_node_list list, int index);
void wmi_ReleaseRAMList(ram_node_list list);

int wmi_GetHDDInstanceName(int index, char* instancename);
int wmi_HDDSelfCheck(char* instancename);

#endif
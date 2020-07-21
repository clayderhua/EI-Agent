#ifndef _ACT_MESSAGE_GENERATE_H_
#define _ACT_MESSAGE_GENERATE_H_
#include "MsgGenerator.h"

#ifdef __cplusplus
extern "C" {
#endif

	MSG_CLASSIFY_T* Act_CreateRoot(char* handlerName);
	//void Act_SetNonSensorDataFlag(MSG_CLASSIFY_T* pNode, bool bNoSensor);
	MSG_CLASSIFY_T* Act_AddAction(MSG_CLASSIFY_T* pNode, char* actName, char* descript, int actCode);
	MSG_ATTRIBUTE_T* Act_AddParamDouble(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamFloat(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamBool(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamLong(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamInt(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamString(MSG_CLASSIFY_T* pNode, char* paramName);
	MSG_ATTRIBUTE_T* Act_AddParamSerial(MSG_CLASSIFY_T* pNode, char* paramName);
	
	MSG_CLASSIFY_T* Act_FindAction(MSG_CLASSIFY_T* pNode, char* actName);
	MSG_ATTRIBUTE_T* Act_FindParam(MSG_CLASSIFY_T* pNode, char* paramName);
	
	bool Act_DelAction(MSG_CLASSIFY_T* pNode, char* actName);
	bool Act_DelParam(MSG_CLASSIFY_T* pNode, char* paramName);
	void Act_ReleaseAll(MSG_CLASSIFY_T* pRoot);

	char *Act_PrintCapability(MSG_CLASSIFY_T* pRoot);

#ifdef __cplusplus
}
#endif
#endif
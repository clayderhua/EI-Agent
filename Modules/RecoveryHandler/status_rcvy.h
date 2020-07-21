#ifndef __STATUS_RCVY_H__
#define __STATUS_RCVY_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL IsAcrocmdRunning();
void DeactivateARSM();
void GetRcvyCurStatus();
BOOL GetRcvyStatus(recovery_status_t * rcvyStatus);
void RunASRM(BOOL isNeedHotkey);
void SendAgentStartMessage(BOOL isCallHotkey);
BOOL isAszExist();
#ifndef _is_linux
int is_acronis_12_5();
#endif

#endif /* __STATUS_RCVY_H__ */
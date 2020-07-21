#ifndef PMQALERT_H
#define PMQALERT_H

#include <stdint.h>

#define PmqStatus_t uint32_t
#define DATA_LENGTH_FOR_ALERT_7_8	72

#define FLASE	0
#define	TRUE	1
#define ALERT(PRE_SMART_VALUE, CUR_SMART_VALUE, TH) (((CUR_SMART_VALUE - PRE_SMART_VALUE) >= TH) ? TRUE : FLASE)	// Check alert

#define PMQ_STATUS_SUCCESS						((PmqStatus_t)0)
#define PMQ_STATUS_ERROR						((PmqStatus_t)0xFFFFF0FF)
#define PMQ_STATUS_HDD_SMART_BASE				((PmqStatus_t)0xF1000000)
#define PMQ_STATUS_HDD_SMART_SPACE_INSUFFIENT	((PmqStatus_t)(PMQ_STATUS_HDD_SMART_BASE + 1))
#define	PMQ_STATUS_HDD_SMART_SPACE_REDUNDANCY	((PmqStatus_t)(PMQ_STATUS_HDD_SMART_BASE + 2))
#define PMQ_STATUS_HDD_SMART_TYPE_NOTFOUND		((PmqStatus_t)(PMQ_STATUS_HDD_SMART_BASE + 3))

enum HDDCheckAlertType
{
	Alert_1 = 0x00,
	Alert_2,
	Alert_3,
	Alert_4,
	Alert_5,
	Alert_6,
	Alert_7,
	Alert_8,
};

PmqStatus_t PMQ_CheckHDDSmartAlert(uint32_t type, int32_t* smartVal, uint8_t len, uint8_t* isAlertTriggered);

#endif // !PMQALERT_H


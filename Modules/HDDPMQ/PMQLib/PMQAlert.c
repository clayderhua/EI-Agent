///************************************************************************///
///**********************   Alert code   **********************************///
///************************************************************************///
#include "PMQAlert.h"
#include <stdlib.h>

uint8_t CheckHDDTemperature(int32_t* data, uint8_t len)
{
	int offset, abnormal = 0;

	for (offset = 0; offset < len; offset++)
	{
		if ((*(data + offset) > 65) || (*(data + offset) < -10))
			abnormal++;
	}

	if (abnormal >= len)
		return TRUE;

	return FLASE;
}

PmqStatus_t PMQ_CheckHDDSmartAlert(uint32_t type, int32_t* smartVal, uint8_t len, uint8_t* isAlertTriggered)
{
	if ((smartVal == NULL) || (isAlertTriggered == NULL))
		return PMQ_STATUS_ERROR;

	if ((type >= Alert_1 && type <= Alert_6) && len < 2)
		return PMQ_STATUS_HDD_SMART_SPACE_INSUFFIENT;
	
	if (((type == Alert_7) || (type == Alert_8)) && len < 72)
		return PMQ_STATUS_HDD_SMART_SPACE_INSUFFIENT;

	switch (type)
	{
	case Alert_1:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 20);
		break;

	case Alert_2:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 10);
		break;

	case Alert_3:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 10);
		break;

	case Alert_4:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 10);
		break;

	case Alert_5:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 10);
		break;

	case Alert_6:
		*(isAlertTriggered) = ALERT(*(smartVal + 0), *(smartVal + 1), 30);
		break;

	case Alert_7:
		*(isAlertTriggered) = CheckHDDTemperature(smartVal, DATA_LENGTH_FOR_ALERT_7_8);
		break;

	case Alert_8:
		*(isAlertTriggered) = CheckHDDTemperature(smartVal, DATA_LENGTH_FOR_ALERT_7_8);
		break;

	default:
		return PMQ_STATUS_HDD_SMART_TYPE_NOTFOUND;
	}

	return PMQ_STATUS_SUCCESS;
}





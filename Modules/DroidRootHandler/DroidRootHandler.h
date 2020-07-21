#ifndef   _DROID_ROOT_HANDLER_H_
#define   _DROID_ROOT_HANDLER_H_

#define cagent_request_droid_root  13
#define cagent_reply_droid_root    134

typedef enum
{
	unknown_cmd = 0,
	droidroot_error_rep = 100,
	droidroot_get_capability_req = 521,
    droidroot_get_capability_rep = 522,
    droidroot_get_sensors_data_req = 523,
    droidroot_get_sensors_data_rep = 524,
    droidroot_set_sensors_data_req = 525,
    droidroot_set_sensors_data_rep = 526,

}susi_comm_cmd_t;

//set/get result code
#define IOT_SGRC_NOT_FOUND     404
#define IOT_SGRC_SUCCESS       200
#define IOT_SGRC_FAIL          500
#define IOT_SGRC_READ_ONLY     405 
#define IOT_SGRC_WRIT_ONLY     405 
#define IOT_SGRC_FORMAT_ERROR  415 
#define IOT_SGRC_SYS_BUSY      503
#define IOT_SGRC_OUT_RANGE     416

#define IOT_SGRC_STR_NOT_FOUND       "Not Found"
#define IOT_SGRC_STR_SUCCESS         "Success"
#define IOT_SGRC_STR_FAIL            "Fail"
#define IOT_SGRC_STR_READ_ONLY       "Read Only" 
#define IOT_SGRC_STR_WRIT_ONLY       "Writ Only" 
#define IOT_SGRC_STR_FORMAT_ERROR    "Format Error" 
#define IOT_SGRC_STR_SYS_BUSY        "Sys Busy"
#define IOT_SGRC_STR_OUT_RANGE       "Out of Range"

typedef enum{
	SSR_SUCCESS = 0,
	SSR_FAIL=1,
	SSR_READ_ONLY = 2,
	SSR_WRITE_ONLY = 3,
	SSR_OVER_RANGE = 4,
	SSR_SYS_BUSY = 5,
	SSR_WRONG_FORMAT = 6,
	SSR_NOT_FOUND = 7,
}sensor_set_ret;

typedef struct sensor_info_t{
	int id;
	char * pathStr;
	char * jsonStr;
	sensor_set_ret setRet;
}sensor_info_t;
typedef struct sensor_info_node_t{
	sensor_info_t sensorInfo;
	struct sensor_info_node_t * next;
}sensor_info_node_t;
typedef sensor_info_node_t * sensor_info_list;

#endif

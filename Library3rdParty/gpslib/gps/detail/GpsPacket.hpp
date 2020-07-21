#ifndef _GPS_PACKET_HPP_
#define _GPS_PACKET_HPP_

// vector 不能被外部存取
#include <vector>

typedef struct _GSVSatData {
    int	nSatPRN;
    int	nElevation;
    int	nAzimuth;
    int	nSNR;
} GSVSatData;

typedef struct _GGAData {

	double	dbGGAUTC;

	double	dbGGALatitude;
	char	cGGALatitudeUnit;
	
	double	dbGGALongitude;
	char	cGGALongitudeUnit;

	int		nGGAQuality;
	int		nGGANumOfSatsInUse;
	double	dbGGAHDOP;
	double	dbGGAAltitude;
	char	cGGAAltUnit;

} GGAData, *PGGAData;


typedef struct _GSVData {

	int	nGSVNumOfMessage;
	int	nGSVMessageNum;
	int	nGSVNumOfSatInView;

	std::vector<GSVSatData> GSVSatInfo;
} GSVData, *PGSVData;

typedef struct _GLLData {
	double dbGLLLatitude;
	char   cGLLLatitudeUnit;
	
	double dbGLLLongitude;
	char   cGLLLongitudeUnit;

	double dbGLLUTC;
	char   cGLLValidStatus;
} GLLData, *PGLLData;

typedef struct _VTGData {
	double	dbTrackModeGood;
	char	cDegreeTure;
	double	dbMagneticModeGood;
	char	cDegreeMagnetic;
	double	dbGroundSpeedKnot;
	char	cGSKnot;
	double	dbGroundSpeedKilo;
	char	cGSKilo;
} VTGData, *PVTGData;

typedef struct _GSAData {

	char	cGSAAutoControl;
	int		nGSAUsedMode;

	std::vector<int> nGSASateInUsed;

	double	dbGSAPDOP;
	double	dbGSAHDOP;
	double	dbGSAVDOP;

} GSAData, *PGSAData;

typedef struct _RMCData {

	double	dbRMCUTC;
	char	cRMCSatus;
	double	dbRMCLatitude;
	char	cRMCLatitudeUnit;
	double	dbRMCLongitude;
	char	cRMCLongitudeUnit;
	double	dbRMCSpeedOverGround;
	double	dbRMCCourseOverGround;
	int		nRMCDate;
	double	dbRMCMSLAltitude;
	char	cRMCMagneticVarUnit;

} RMCData, *PRMCData;

#endif

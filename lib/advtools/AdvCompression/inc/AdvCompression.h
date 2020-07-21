#ifndef __ADVANTECH_COMPRESSION_H__
#define __ADVANTECH_COMPRESSION_H__

#if defined(WIN32)
	#pragma once
	#ifdef ADVCOMPRESSION_EXPORTS
		#define ADVCOMPRESSION_CALL __stdcall
		#define ADVCOMPRESSION_EXPORT __declspec(dllexport)
	#else
		#define ADVCOMPRESSION_CALL __stdcall
		#define ADVCOMPRESSION_EXPORT
	#endif

#else
	#define ADVCOMPRESSION_CALL
	#define ADVCOMPRESSION_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif
ADVCOMPRESSION_EXPORT int ADVCOMPRESSION_CALL AdvZ_GetMaxCompressedLen(const char *method, const unsigned char* raw, int rawtLen);
ADVCOMPRESSION_EXPORT int ADVCOMPRESSION_CALL AdvZ_GetMaxUncompressedLen(const char *method, const unsigned char* compressed, int compressedLen);
ADVCOMPRESSION_EXPORT int ADVCOMPRESSION_CALL AdvZ_CompressData(const char *method, const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst);
ADVCOMPRESSION_EXPORT int ADVCOMPRESSION_CALL AdvZ_UncompressData(const char *method, const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst);
#ifdef ANDROID
#define AdvZ_GetMaxCompressedLen(method,raw,rawtLen) ({int p = 0; p;})
#define AdvZ_CompressData(method,abSrc,nLenSrc,abDst,nLenDst) ({int p = 0; p;})
#endif
#ifdef __cplusplus
}
#endif
#endif //__ADVANTECH_COMPRESSION_H__
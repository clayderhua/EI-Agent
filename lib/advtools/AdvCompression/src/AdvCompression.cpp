#include "zlib.h"
#include "AdvCompression.h"


int ADVCOMPRESSION_CALL AdvZ_GetMaxCompressedLen(const char *method, const unsigned char* raw, int rawtLen)
{
	int n16kBlocks = (rawtLen + 16383) / 16384; // round up any fraction of a block
	return (rawtLen + 6 + (n16kBlocks * 5));
}

int ADVCOMPRESSION_CALL AdvZ_GetMaxUncompressedLen(const char *method, const unsigned char* compressed, int compressedLen) {
	return 0;
}

int ADVCOMPRESSION_CALL AdvZ_CompressData(const char *method, const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst)
{
	int nErr = -1;
	int nRet = -1;
	z_stream zInfo = { 0 };
	zInfo.total_in = zInfo.avail_in = nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (unsigned char*)abSrc;
	zInfo.next_out = abDst;

	nErr = deflateInit(&zInfo, Z_DEFAULT_COMPRESSION); // zlib function
	if (nErr == Z_OK) {
		nErr = deflate(&zInfo, Z_FINISH);              // zlib function
		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	deflateEnd(&zInfo);    // zlib function
	return(nRet);
}

int ADVCOMPRESSION_CALL AdvZ_UncompressData(const char *method, const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst)
{
	int nErr = -1;
	int nRet = -1;

	z_stream zInfo = { 0 };
	zInfo.total_in = zInfo.avail_in = nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (unsigned char*)abSrc;
	zInfo.next_out = abDst;

	nErr = inflateInit(&zInfo);               // zlib function
	if (nErr == Z_OK) {
		nErr = inflate(&zInfo, Z_FINISH);     // zlib function
		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	inflateEnd(&zInfo);   // zlib function
	return(nRet); // -1 or len of output
}
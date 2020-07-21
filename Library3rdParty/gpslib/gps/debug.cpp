#include "stdafx.h"
#include <stdio.h>

void __cdecl odprintf(const CHAR *format, ...)
{
	CHAR buf[512], *p=buf;
	va_list args;
	int n;

		va_start(args, format);
		n = _vsnprintf_s(p,				// Storage location for output
						sizeof(buf),	// The size of the buffer for output.
						sizeof(buf)-3,	// Maximum number of characters to write
										//  (not including the terminating null),
										//  buf-3 is room for CR/LF/NUL
						format,			// Format specification
						args);			// Pointer to list of arguments.

		va_end(args);
	
		p += (n < 0) ? (sizeof(buf)-3) : n;
		while( p>buf && isspace(p[-1]) )
		{
			*--p = '\0';
		}
 
		*p++ = '\r';
		*p++ = '\n';
		*p++ = '\0';
 
		OutputDebugStringA(buf);
}
#ifndef __UTILS_COLLECTION__STRING_BUILDER_EXTENSION_H__
#define __UTILS_COLLECTION__STRING_BUILDER_EXTENSION_H__

#include "StringBuilder.h"

inline void operator +=(StringBuilder& oBuilder, int iValue)
{
	char pBuffer[33];
	sprintf(pBuffer, "%d", iValue);
	oBuilder.Append(pBuffer);
}

inline void operator +=(StringBuilder& oBuilder, unsigned int iValue)
{
	char pBuffer[33];
	sprintf(pBuffer, "%u", iValue);
	oBuilder.Append(pBuffer);
}

inline void operator +=(StringBuilder& oBuilder, long iValue)
{
	char pBuffer[65];
	sprintf(pBuffer, "%ld", iValue);
	oBuilder.Append(pBuffer);
}

inline void operator +=(StringBuilder& oBuilder, unsigned long iValue)
{
	char pBuffer[65];
	sprintf(pBuffer, "%lu", iValue);
	oBuilder.Append(pBuffer);
}

inline void operator +=(StringBuilder& oBuilder, float fValue)
{
	char pBuffer[65];
	sprintf(pBuffer, "%f", fValue);
	oBuilder.Append(pBuffer);
}

inline void operator +=(StringBuilder& oBuilder, double fValue)
{
	char pBuffer[65];
	sprintf(pBuffer, "%f", fValue);
	oBuilder.Append(pBuffer);
}

#endif //__UTILS_COLLECTION__STRING_BUILDER_EXTENSION_H__
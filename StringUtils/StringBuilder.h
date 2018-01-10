#ifndef __UTILS_COLLECTION__STRING_BUILDER_H__
#define __UTILS_COLLECTION__STRING_BUILDER_H__

#include <stdlib.h> // for malloc/free

//#define __STRINGBUILDER_USE_STD_STRING__
#ifdef __STRINGBUILDER_USE_STD_STRING__
#include <string>
#endif // __STRINGBUILDER_USE_STD_STRING__

class StringBuilder
{
	typedef unsigned int uint32;

	static const uint32 c_iPoolSize = 2048;

	struct Pool
	{
		char			m_oData[c_iPoolSize];
		Pool*			m_pNext;
	};
public:
	typedef void*(*MallocFuncPtr)(size_t iSize);
	typedef void(*FreeFuncPtr)(void*);

	StringBuilder(MallocFuncPtr pMallocFunc = malloc, FreeFuncPtr pFreeFunc = free);
	~StringBuilder();

	void				Clear(bool bFreePools = false);
	int					Size() const { return m_iPosition; }

	void				Append(char cChar);
	void				Append(const char* pString);
	void				CopyTo(char* pDest);
	char*				Export(); //Using malloc passed to constructor

	void				operator +=(char cChar);
	void				operator +=(const char* pString);

#ifdef __STRINGBUILDER_USE_STD_STRING__
	void				Append(const std::string& oString);
	void				CopyTo(std::string& sOut);

	void				operator +=(const std::string& oString);
#endif //__STRINGBUILDER_USE_STD_STRING__

protected:
	void				AppendInternal(const char* pString, uint32 iLen);

	MallocFuncPtr		m_pMallocFunc;
	FreeFuncPtr			m_pFreeFunc;

	uint32				m_iPosition;
	Pool				m_oFirstPool;
	Pool*				m_pCurrentPool;
	int					m_iCurrentPoolIndex;
};

#endif //__UTILS_COLLECTION__STRING_BUILDER_H__
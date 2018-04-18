#include <stdio.h> // for printf
#include <stdlib.h> // for malloc/free
#include <inttypes.h> //for uint32_t
#include <assert.h> //for assert
#include <string.h> //for memset

typedef struct LeakTrackerAlloc
{
	int								m_iCheck;
	struct LeakTrackerAlloc*		m_pPrev;
	struct LeakTrackerAlloc*		m_pNext;
	size_t							m_iSize;
	const char*						m_pFilename;
	int								m_iLine;
} LeakTrackerAllocStruct;


static const uint32_t				s_iLeakTrackerMagicValue = 0x05E4B2AD;
static LeakTrackerAlloc*			s_pLastAlloc = NULL;
static bool							s_bLeakTrackerSetuped = false;

void LeakTrackerSetup()
{
	assert(s_bLeakTrackerSetuped == false);
	s_bLeakTrackerSetuped = true;
}

void LeakTrackerShutdown()
{
	assert(s_bLeakTrackerSetuped == true);
	s_bLeakTrackerSetuped = false;

	if (s_pLastAlloc != NULL)
	{
		LeakTrackerAllocStruct* pAlloc = s_pLastAlloc;

		size_t iCount = 0;
		size_t iTotalSize = 0;
		while (pAlloc != NULL)
		{
			++iCount;
			iTotalSize += pAlloc->m_iSize;
			pAlloc = pAlloc->m_pPrev;
		}

		printf("==========================\nLEAK(S) FOUND\n==========================\n");
		printf("Leak count : %zd(s)\n", iCount);
		printf("Leaks total size: %zd byte(s)\n", iTotalSize);
		
		pAlloc = s_pLastAlloc;
		while (pAlloc != NULL)
		{
			printf("Leak: %llu byte(s)\n%s(%d)\n", pAlloc->m_iSize, pAlloc->m_pFilename, pAlloc->m_iLine);
			pAlloc = pAlloc->m_pPrev;
		}
		assert(false);
	}
}

void* LeakTrackerMemAlloc(size_t iSize, const char* pFilename, int iLine)
{
	assert(s_bLeakTrackerSetuped);
	LeakTrackerAllocStruct* pAllocInfos = (LeakTrackerAllocStruct*)malloc(iSize + sizeof(LeakTrackerAllocStruct));
	pAllocInfos->m_iCheck = s_iLeakTrackerMagicValue;
	pAllocInfos->m_iSize = iSize;
	pAllocInfos->m_pPrev = s_pLastAlloc;
	pAllocInfos->m_pNext = NULL;
	pAllocInfos->m_pFilename = pFilename;
	pAllocInfos->m_iLine = iLine;
	s_pLastAlloc = pAllocInfos;
	return (void*)(pAllocInfos + 1);
}

void LeakTrackerMemFree(void* pMemory, size_t iSize)
{
	assert(s_bLeakTrackerSetuped);
	LeakTrackerAllocStruct* pAllocInfos = ((LeakTrackerAllocStruct*)pMemory) - 1;

	if (pAllocInfos->m_iCheck != s_iLeakTrackerMagicValue
		|| (iSize > 0 && pAllocInfos->m_iSize != iSize) )
	{
		assert(false);
		return;
	}

	if (s_pLastAlloc == pAllocInfos)
	{
		s_pLastAlloc = pAllocInfos->m_pPrev;
	}

	if (pAllocInfos->m_pPrev != NULL)
	{
		pAllocInfos->m_pPrev->m_pNext = pAllocInfos->m_pNext;
	}

	pAllocInfos->m_iCheck = 0;

	free(pAllocInfos);
}

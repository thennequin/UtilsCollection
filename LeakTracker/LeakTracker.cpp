#include <stdio.h> // for printf
#include <stdlib.h> // for malloc/free
#include <inttypes.h> //for uint32_t
#include <assert.h> //for assert
#include <string.h> //for memset

#if defined(_WIN32)
#	include <windows.h>
#	define mutex CRITICAL_SECTION

int mutex_init(mutex *pMutex)
{
	InitializeCriticalSection(pMutex);
	return 0;
}

int mutex_lock(mutex *pMutex)
{
	EnterCriticalSection(pMutex);
	return 0;
}

int mutex_unlock(mutex *pMutex)
{
	LeaveCriticalSection(pMutex);
	return 0;
}

#else //Linux
#	include <pthread.h>
#	define mutex pthread_mutex_t

int mutex_init(mutex *pMutex)
{
	return pthread_mutex_init(pMutex, NULL);
}

int mutex_lock(mutex *pMutex)
{
	return pthread_mutex_lock(pMutex);
}

int mutex_unlock(mutex *pMutex)
{
	return pthread_mutex_unlock(pMutex);
}

#endif


typedef struct LeakTrackerAlloc
{
	int								m_iCheck;
	struct LeakTrackerAlloc*		m_pPrev;
	struct LeakTrackerAlloc*		m_pNext;
	size_t							m_iSize;
	const char*						m_pFilename;
	int								m_iLine;
} LeakTrackerAllocStruct;

class LeakTracker
{
	static const uint32_t				c_iLeakTrackerMagicValue = 0x05E4B2AD;
	LeakTrackerAlloc*					m_pLastAlloc;
	mutex								m_oMutex;
public:
	LeakTracker()
	{
		m_pLastAlloc = NULL;
		mutex_init(&m_oMutex);
	}

	~LeakTracker()
	{
		mutex_lock(&m_oMutex);
		if (m_pLastAlloc != NULL)
		{
			LeakTrackerAllocStruct* pAlloc = m_pLastAlloc;

			size_t iCount = 0;
			size_t iTotalSize = 0;
			while (pAlloc != NULL)
			{
				++iCount;
				iTotalSize += pAlloc->m_iSize;
				pAlloc = pAlloc->m_pPrev;
			}

			char pReableSize[128];

			printf("==========================\nLEAK(S) FOUND\n==========================\n");
			printf("Leak count : %zd (%s)\n\n", iCount, ReadableSize(iTotalSize, pReableSize));

			pAlloc = m_pLastAlloc;
			while (pAlloc != NULL)
			{
				printf("Leak: %s  %s(%d)\n", ReadableSize(pAlloc->m_iSize, pReableSize), pAlloc->m_pFilename, pAlloc->m_iLine);
				pAlloc = pAlloc->m_pPrev;
			}
			assert(false);
		}
		mutex_unlock(&m_oMutex);
	}

	void* MemAlloc(size_t iSize, const char* pFilename, int iLine)
	{
		mutex_lock(&m_oMutex);

		LeakTrackerAllocStruct* pAllocInfos = (LeakTrackerAllocStruct*)malloc(iSize + sizeof(LeakTrackerAllocStruct));
		pAllocInfos->m_iCheck = LeakTracker::c_iLeakTrackerMagicValue;
		pAllocInfos->m_iSize = iSize;
		pAllocInfos->m_pPrev = LeakTracker::m_pLastAlloc;
		pAllocInfos->m_pNext = NULL;
		pAllocInfos->m_pFilename = pFilename;
		pAllocInfos->m_iLine = iLine;

		if (LeakTracker::m_pLastAlloc != NULL)
			LeakTracker::m_pLastAlloc->m_pNext = pAllocInfos;
		LeakTracker::m_pLastAlloc = pAllocInfos;

		mutex_unlock(&m_oMutex);
		return (void*)(pAllocInfos + 1);
	}

	void MemFree(void* pMem, size_t iSize = 0)
	{
		LeakTrackerAllocStruct* pAllocInfos = ((LeakTrackerAllocStruct*)pMem) - 1;

		if (pAllocInfos->m_iCheck != c_iLeakTrackerMagicValue
			|| (iSize > 0 && pAllocInfos->m_iSize != iSize))
		{
			assert(false);
			return;
		}

		mutex_lock(&m_oMutex);

		if (m_pLastAlloc == pAllocInfos)
		{
			m_pLastAlloc = pAllocInfos->m_pPrev;
		}

		if (pAllocInfos->m_pPrev != NULL)
		{
			pAllocInfos->m_pPrev->m_pNext = pAllocInfos->m_pNext;
		}

		if (pAllocInfos->m_pNext != NULL)
		{
			pAllocInfos->m_pNext->m_pPrev = pAllocInfos->m_pPrev;
		}

		pAllocInfos->m_iCheck = 0;

		free(pAllocInfos);

		mutex_unlock(&m_oMutex);
	}

	static char* ReadableSize(size_t iSize, char* pBuffer)
	{
		int i = 0;
		double fSize = (double)iSize;
		const char* pUnits[] = { "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
		while (fSize > 1024.0) {
			fSize /= 1024.0;
			i++;
		}
		sprintf(pBuffer, "%.*f %s", i, fSize, pUnits[i]);
		return pBuffer;
	}
};

static LeakTracker s_oLeakTracker;


void* LeakTrackerMemAlloc(size_t iSize, const char* pFilename, int iLine)
{
	return s_oLeakTracker.MemAlloc(iSize, pFilename, iLine);
}

void LeakTrackerMemFree(void* pMem, size_t iSize)
{
	s_oLeakTracker.MemFree(pMem, iSize);
}

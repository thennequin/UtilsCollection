#ifndef __UTILS_COLLECTION__LEAK_TRACKER_H__
#define __UTILS_COLLECTION__LEAK_TRACKER_H__

void* LeakTrackerMemAlloc(size_t iSize, const char* pFilename, int iLine);
void LeakTrackerMemFree(void* pMem, size_t iSize = 0);

void* operator new (size_t iSize, const char* pFilename, int iLine)
{
	return LeakTrackerMemAlloc(iSize, pFilename, iLine);
}

void operator delete(void* pMem, const char* /*pFilename*/, int /*iLine*/)
{
	LeakTrackerMemFree(pMem, 0);
}

void operator delete(void* pMem, size_t iSize)
{
	LeakTrackerMemFree(pMem, iSize);
}

#endif //__UTILS_COLLECTION__LEAK_TRACKER_H__

#define malloc(size) LeakTrackerMemAlloc(size, __FILE__, __LINE__)
#define free(ptr) LeakTrackerMemFree(ptr)
#define new new(__FILE__, __LINE__)

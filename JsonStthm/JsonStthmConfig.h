
#ifndef __JSON_STTHM_CONFIG_H__
#define __JSON_STTHM_CONFIG_H__

#include <string>

// Configuration

#ifndef STTHM_API
#define STTHM_API
#endif

#ifndef NULL
#define NULL 0
#endif 

#define StthmMalloc(iSize) malloc(iSize)
#define StthmFree(pObj) free(pObj)
#define StthmSafeFree(pObj) {free(pObj); pObj = NULL;}

// End of configuration

#endif // __JSON_STTHM_CONFIG_H__
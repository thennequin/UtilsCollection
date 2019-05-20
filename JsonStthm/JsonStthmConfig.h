
#ifndef __JSON_STTHM_CONFIG_H__
#define __JSON_STTHM_CONFIG_H__

// Configuration

#ifndef STTHM_API
#define STTHM_API
#endif

#ifndef NULL
#define NULL 0
#endif 

#include <stdlib.h> // malloc, free

#define StthmMalloc(iSize) malloc(iSize)
#define StthmFree(pObj) free(pObj)

#define STTHM_USE_STD_STRING

// End of configuration

#endif // __JSON_STTHM_CONFIG_H__
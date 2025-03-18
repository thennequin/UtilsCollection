
#ifndef __JSON_STTHM_CONFIG_H__
#define __JSON_STTHM_CONFIG_H__

// Configuration

#ifndef STTHM_API
#define STTHM_API
#endif

#ifndef NULL
#define NULL 0
#endif

#include <assert.h>
#define JsonStthmAssert(bCondition) assert((bCondition))

#include <stdlib.h> // malloc, free

#define JsonStthmMalloc(iSize) malloc(iSize)
#define JsonStthmFree(pObj) free(pObj)

#define JsonStthmString std::string
#include <string>

//#define STTHM_ENABLE_IMPLICIT_CAST

/*
* Allow to define custom functions to JsonValue
* Usefull to declare implicit cast operator
*/
//#define STTHM_CUSTOM_FUNCTIONS

// End of configuration

#endif // __JSON_STTHM_CONFIG_H__
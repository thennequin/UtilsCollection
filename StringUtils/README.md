# StringBuilder
Create large string easily and optimally

## Usage

### With default allocator (**malloc**/**free**)

``` c++
#include "StringBuilder.h"
StringBuilder oStrBuilder;
oStrBuilder += "Hello";
oStrBuilder += ' ';
oStrBuilder += "World";
oStrBuilder += '!';

char* pString = oStrBuilder.Export();
printf("%s", pString);
free(pString);
```

Result output:
> Hello World!

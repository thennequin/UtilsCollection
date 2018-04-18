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

### With custom allocator

``` c++
#include "StringBuilder.h"

void* mymalloc(size_t size)
{
  ...
}

void myfree(void* pData)
{
  ...
}

StringBuilder oStrBuilder(mymalloc, myfree);
oStrBuilder += "Hello";
oStrBuilder += ' ';
oStrBuilder += "World";
oStrBuilder += '!';

char* pString = oStrBuilder.Export();
printf("%s", pString);
myfree(pString);
```

### Build string with numeric values


``` c++
#include "StringBuilder.h"
#include "StringBuilderExtension.h"

StringBuilder oStrBuilder;

oStrBuilder += "int: ";
oStrBuilder += (int)-123456;
oStrBuilder += '\n';

oStrBuilder += "unsigned int: ";
oStrBuilder += (unsigned int)123456;
oStrBuilder += '\n';

oStrBuilder += "long: ";
oStrBuilder += (long)-1234567890123;
oStrBuilder += '\n';

oStrBuilder += "unsigned long: ";
oStrBuilder += (unsigned long)1234567890123;
oStrBuilder += '\n';

oStrBuilder += "float: ";
oStrBuilder += 3.1415926535897932384626433832795028841971f;
oStrBuilder += '\n';

oStrBuilder += "double: ";
oStrBuilder += 3.1415926535897932384626433832795028841971;
oStrBuilder += '\n';

char* pString = oStrBuilder.Export();
printf("%s", pString);
free(pString);
```

Result output:
> int: -123456
> 
> unsigned int: 123456
> 
> long: -1912276171
> 
> unsigned long: 1912276171
> 
> float: 3.141593
>
> double: 3.141593

**float** and **double** seams the same because the extension use printf with %f for both.


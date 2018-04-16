# JsonStthm
Fast Json reader/writer

## How to use it

### Read json file
```cpp
#include "JsonStthm.h"

JsonStthm::JsonValue oJson;
oJson.ReadFile("data.json");
```

### Create json
```cpp
#include "JsonStthm.h"

JsonStthm::JsonValue oValue;
JsonStthm::JsonValue& oArray = oValue["myArray"];
oArray[0] = "test";
oArray[1] = false;
oArray[2] = 3.14159265359f;
  
JsonStthm::String sOut;
oValue.WriteString(sOut);
```
Content of **sOut**
```json
{
	"myArray": [
		"test",
		false,
		3.1415927410125732
	]
}
```

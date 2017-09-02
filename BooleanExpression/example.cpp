
#include <stdio.h>
#include <string>

#include "BooleanExpression.h"

bool StringFinder(const char* pLiteral, const void* pData)
{
	return strstr((const char*)pData, pLiteral) != NULL;
}

const char* const c_pSentences[] = {
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
	"Curabitur mattis magna id mauris fermentum vehicula.",
	"Duis interdum nibh non felis ornare scelerisque.",
	"Ut ut risus id lectus efficitur egestas.",
	"Sed egestas augue sit amet dui elementum, quis consequat mauris mollis.",
	"Maecenas et leo eu purus porta faucibus at vitae erat.",
	"Cras feugiat nunc tempor erat suscipit iaculis."
};

void main()
{
	const char* const pExpression = "(sit | id) & (! ut | amet)";

	printf("Search: %s\n", pExpression);
	BooleanExpression oExpr;
	if (oExpr.Generate(pExpression))
	{
		oExpr.Print();
		for (int i = 0, iCount = (sizeof(c_pSentences) / sizeof(char*)); i < iCount; ++i)
		{
			if (oExpr.Test((void*)c_pSentences[i], StringFinder))
			{
				printf("Found in: %s\n", c_pSentences[i]);
			}
		}
	}

	system("pause");
}
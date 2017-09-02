# BooleanExpression
Simple Lexer does to make a search engine

## Operators

You can combinate multiple search expressions with operators:

  - **&** or _ (Space) : And
  - **|** : Or
  - **!** : Not
  - **( )** : Isolate expression

> **Warning** : Weight of operator **&** is heavier than operator **|**
> Exemple:
> ```
> a | b & c
> ```
> is same than
> ```
> a | (b & c)
> ```

## How to use it

```cpp
bool StringFinder(const char* pLiteral, const void* pData)
{
    return strstr((const char*)pData, pLiteral) != NULL;
}

BooleanExpression oExpr;
if (oExpr.Generate("string & find"))
{
    if (oExpr.Test("My string to find", StringFinder))
    {
        printf("Found it\n");
    }
}
```
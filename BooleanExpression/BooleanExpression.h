#ifndef __UTILS_COLLECTION__BOOLEAN_EXPRESSION_H__
#define __UTILS_COLLECTION__BOOLEAN_EXPRESSION_H__

class BooleanExpression
{
	enum ELexerType
	{
		E_LEXER_TYPE_NULL,
		E_LEXER_TYPE_LITERAL,
		E_LEXER_TYPE_PARENTHESIS_OPEN,
		E_LEXER_TYPE_PARENTHESIS_CLOSE,
		E_LEXER_TYPE_AND,
		E_LEXER_TYPE_OR,
		E_LEXER_TYPE_NOT
	};
public:
	enum EType
	{
		E_TYPE_LITERAL,
		E_TYPE_AND,
		E_TYPE_OR,
		E_TYPE_NOT
	};

											BooleanExpression();
											~BooleanExpression();

	void									Clear();

	bool									Generate( const char* pExpression );

	void									Print( int iLevel = 0 ) const;

	EType									GetType() const;
	const char*								GetLiteral() const;
	const BooleanExpression**				GetEpxressions() const;

	bool									Test( const void* pData, bool(*pTestFunc)(const char*, const void*) ) const;
protected:
											BooleanExpression( EType eType, const char* pLiteral );

	bool									Expression( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen );
	bool									Term( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen );
	bool									Factor( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen );

	static inline bool						IsSpace( char cChar );
	static inline void						SkipSpaces( const char** pStringPtr );
	static inline bool						IsOneOf( char cChar, const char* pChar );


	EType									m_eType;
	const char*								m_pLiteral;
	const BooleanExpression*				m_pExpressions[2];
};

#endif //__UTILS_COLLECTION__BOOLEAN_EXPRESSION_H__
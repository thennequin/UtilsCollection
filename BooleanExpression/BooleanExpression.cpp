
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "BooleanExpression.h"

#ifndef BE_ASSERT
#	define BE_ASSERT(bTest)
#endif // !BE_ASSERT

BooleanExpression::BooleanExpression()
{
	m_eType = E_TYPE_LITERAL;
	m_pLiteral = NULL;
	m_pExpressions[0] = NULL;
	m_pExpressions[1] = NULL;
}

BooleanExpression::BooleanExpression( EType eType, const char* pLiteral )
{
	m_eType = eType;
	m_pLiteral = pLiteral;
	m_pExpressions[0] = NULL;
	m_pExpressions[1] = NULL;
}

BooleanExpression::~BooleanExpression()
{
	Clear();
}

void BooleanExpression::Clear()
{
	m_eType = E_TYPE_LITERAL;

	if( m_pLiteral != NULL )
		free( (void*)m_pLiteral );

	if( m_pExpressions[0] != NULL )
		delete m_pExpressions[0];
	if( m_pExpressions[1] != NULL )
		delete m_pExpressions[1];
}

bool BooleanExpression::Generate( const char* pExpression )
{
	Clear();

	if( NULL != pExpression )
	{
		const int iMaxBlock = 512;
		ELexerType pBlockType[iMaxBlock];
		const char* pBlockValue[iMaxBlock];
		int iBlockCount = 0;

		const char* pString = pExpression;

		// Lexer
		while( *pString != 0 && iBlockCount < iMaxBlock )
		{
			SkipSpaces( &pString );

			if( *pString == '(' )
			{
				//Open Multiple Expression
				pBlockType[iBlockCount] = E_LEXER_TYPE_PARENTHESIS_OPEN;
				pBlockValue[iBlockCount] = NULL;
				++iBlockCount;
				++pString;
			}
			else if( *pString == ')' )
			{
				pBlockType[iBlockCount] = E_LEXER_TYPE_PARENTHESIS_CLOSE;
				pBlockValue[iBlockCount] = NULL;
				++iBlockCount;
				++pString;
			}
			else if( *pString == '&' )
			{
				// And Expression
				pBlockType[iBlockCount] = E_LEXER_TYPE_AND;
				pBlockValue[iBlockCount] = NULL;
				++iBlockCount;
				++pString;
			}
			else if( *pString == '|' )
			{
				// Or expression
				pBlockType[iBlockCount] = E_LEXER_TYPE_OR;
				pBlockValue[iBlockCount] = NULL;
				++iBlockCount;
				++pString;
			}
			else if( *pString == '!' )
			{
				// Not expression
				pBlockType[iBlockCount] = E_LEXER_TYPE_NOT;
				pBlockValue[iBlockCount] = NULL;
				++iBlockCount;
				++pString;
			}
			else
			{
				//ReadExpression
				//TODO : manage quotes
				const char* pStart = pString;
				while( *pString != 0 && !IsSpace( *pString ) && !IsOneOf( *pString, "()&|!" ) )
				{
					++pString;
				}
				int iLen = (int)(pString - pStart);
				char* pExpressionStr = (char*)malloc( iLen + 1 );
				memcpy( pExpressionStr, pStart, iLen );
				pExpressionStr[iLen] = 0;

				//////////////////////////////////////////////////////////////////////////

				pBlockType[iBlockCount] = E_LEXER_TYPE_LITERAL;
				pBlockValue[iBlockCount] = pExpressionStr;
				++iBlockCount;
			}
		}

		// Parser

		BooleanExpression* pRoot = NULL;
		int iCurrentBlock = 0;
		while( iCurrentBlock < iBlockCount && Expression( pRoot, (ELexerType*)&pBlockType, (const char**)&pBlockValue, iCurrentBlock, iBlockCount, 0 ) )
			Expression( pRoot, (ELexerType*)&pBlockType, (const char**)&pBlockValue, iCurrentBlock, iBlockCount, 0 );

		if( pRoot != NULL )
		{
			m_eType = pRoot->m_eType;
			m_pLiteral = pRoot->m_pLiteral;
			m_pExpressions[0] = pRoot->m_pExpressions[0];
			m_pExpressions[1] = pRoot->m_pExpressions[1];

			pRoot->m_pLiteral = NULL;
			pRoot->m_pExpressions[0] = NULL;
			pRoot->m_pExpressions[1] = NULL;
			delete pRoot;
		}


		return true;
	}

	return false;
}

bool BooleanExpression::Expression( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen )
{
	if( iCurrent >= iMax )
		return false;

	bool bOpen = Term( pRoot, pType, pValues, iCurrent, iMax, iOpen );
	if( bOpen )
	{
		while( bOpen && pType[iCurrent] == E_LEXER_TYPE_OR )
		{
			++iCurrent;

			BooleanExpression* pExpr1 = pRoot;
			bOpen &= Term( pRoot, pType, pValues, iCurrent, iMax, iOpen );
			BooleanExpression* pExpr2 = pRoot;

			if( pExpr1 != NULL && pExpr2 != NULL && pExpr1 != pExpr2 )
			{
				BooleanExpression* pOr = new BooleanExpression(E_TYPE_OR, NULL);
				pOr->m_pExpressions[0] = pExpr1;
				pOr->m_pExpressions[1] = pExpr2;
				pRoot = pOr;
			}
		}
	}

	return bOpen;
}

bool BooleanExpression::Term( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen )
{
	if( iCurrent >= iMax )
		return false;

	bool bOpen = Factor( pRoot, pType, pValues, iCurrent, iMax, iOpen );
	if( bOpen )
	{
		while( bOpen && ( pType[iCurrent] == E_LEXER_TYPE_AND || pType[iCurrent] == E_LEXER_TYPE_LITERAL ) )
		{
			if( pType[iCurrent] != E_LEXER_TYPE_LITERAL )
				++iCurrent;

			BooleanExpression* pExpr1 = pRoot;
			bOpen &= Factor( pRoot, pType, pValues, iCurrent, iMax, iOpen );
			BooleanExpression* pExpr2 = pRoot;

			if( pExpr1 != NULL && pExpr2 != NULL && pExpr1 != pExpr2 )
			{
				BooleanExpression* pAnd = new BooleanExpression(E_TYPE_AND, NULL);
				pAnd->m_pExpressions[0] = pExpr1;
				pAnd->m_pExpressions[1] = pExpr2;
				pRoot = pAnd;
			}
		}
	}
	return bOpen;
}

bool BooleanExpression::Factor( BooleanExpression*& pRoot, ELexerType* pType , const char** pValues, int& iCurrent, int iMax, int iOpen )
{
	if( iCurrent >= iMax )
		return false;

	const char* pValue = pValues[iCurrent];
	ELexerType eType = pType[iCurrent];
	++iCurrent;

	if (eType == E_LEXER_TYPE_LITERAL)
	{
		pRoot = new BooleanExpression( E_TYPE_LITERAL, pValue );
	}
	else if (eType == E_LEXER_TYPE_NOT)
	{
		BooleanExpression* pNot = new BooleanExpression(E_TYPE_NOT, NULL);
		bool bOpen = Factor( pRoot, pType, pValues, iCurrent, iMax, iOpen );
		pNot->m_pExpressions[0] = pRoot;
		pRoot = pNot;
		return bOpen;
	}
	else if (eType == E_LEXER_TYPE_PARENTHESIS_OPEN)
	{
		bool bOpen = true;

		BooleanExpression* pLastRoot = pRoot;
		pRoot = NULL;
		while ( bOpen )
		{
			bOpen = Expression( pRoot, pType, pValues, iCurrent, iMax, iOpen + 1 );
			//BE_ASSERT( pRoot != NULL );
		}

		if( pLastRoot != NULL && iOpen > 0 )
		{
			if( pRoot != NULL )
			{
				if( pLastRoot->m_eType == E_TYPE_AND )
				{
					BooleanExpression* pAnd = new BooleanExpression(E_TYPE_OR, NULL);
					pAnd->m_pExpressions[0] = pLastRoot;
					pAnd->m_pExpressions[1] = pRoot;
					BE_ASSERT( pAnd->m_pExpressions[0] );
					BE_ASSERT( pAnd->m_pExpressions[1] );
					BE_ASSERT( pLastRoot->m_pExpressions[0] );
					BE_ASSERT( pLastRoot->m_pExpressions[1] );
					BE_ASSERT( pAnd->m_pExpressions[0] != pAnd->m_pExpressions[1] );
					BE_ASSERT( pLastRoot->m_pExpressions[0] != pLastRoot->m_pExpressions[1] );

					pLastRoot = pAnd;
				}
				else if( pLastRoot->m_eType == E_TYPE_OR )
				{
					BooleanExpression* pAnd = new BooleanExpression(E_TYPE_OR, NULL);
					pAnd->m_pExpressions[0] = pLastRoot;
					pAnd->m_pExpressions[1] = pRoot;
					BE_ASSERT( pAnd->m_pExpressions[0] );
					BE_ASSERT( pAnd->m_pExpressions[1] );
					BE_ASSERT( pLastRoot->m_pExpressions[0] );
					BE_ASSERT( pLastRoot->m_pExpressions[1] );
					BE_ASSERT( pAnd->m_pExpressions[0] != pAnd->m_pExpressions[1] );
					BE_ASSERT( pLastRoot->m_pExpressions[0] != pLastRoot->m_pExpressions[1] );

					pLastRoot = pAnd;
				}
				else
				{
					BE_ASSERT(false);
				}
			}
			pRoot = pLastRoot;
		}
		BE_ASSERT( pRoot );


		return true;
	}
	else if (eType == E_LEXER_TYPE_PARENTHESIS_CLOSE)
	{
		//Do nothing
		return false;
	}
	else
	{
		//printf("Expression Malformed");
		//return false;
	}
	return true;
}

void BooleanExpression::Print( int iLevel ) const
{
	for (int i = 0; i < iLevel; ++i)
	{
		printf("\t");
	}
	switch (m_eType)
	{
		case E_TYPE_LITERAL:
			printf( "Literal : %s\n", m_pLiteral );
			break;
		case E_TYPE_AND:
			printf( "And:\n" );
			m_pExpressions[0]->Print(iLevel + 1);
			m_pExpressions[1]->Print(iLevel + 1);
			break;
		case E_TYPE_OR:
			printf( "Or:\n" );
			m_pExpressions[0]->Print(iLevel + 1);
			m_pExpressions[1]->Print(iLevel + 1);
			break;
		case E_TYPE_NOT:
			printf( "Not:\n" );
			m_pExpressions[0]->Print(iLevel + 1);
			break;
	}
}

BooleanExpression::EType BooleanExpression::GetType() const
{
	return m_eType;
}

const char* BooleanExpression::GetLiteral() const
{
	return m_pLiteral;
}

const BooleanExpression** BooleanExpression::GetEpxressions() const
{
	return (const BooleanExpression**)m_pExpressions;
}

bool BooleanExpression::Test( const void* pData, bool(*pTestFunc)(const char*, const void*) ) const
{
	switch (m_eType)
	{
	case E_TYPE_LITERAL:
		return pTestFunc( m_pLiteral, pData );
	case E_TYPE_AND:
		return m_pExpressions[0]->Test( pData, pTestFunc ) && m_pExpressions[1]->Test( pData, pTestFunc );
	case E_TYPE_OR:
		return m_pExpressions[0]->Test( pData, pTestFunc ) || m_pExpressions[1]->Test( pData, pTestFunc );
	case E_TYPE_NOT:
		return !m_pExpressions[0]->Test( pData, pTestFunc );
	}
	return false;
}

bool BooleanExpression::IsSpace( char cChar )
{
	return cChar == ' ' || (cChar >= '\t' && cChar <= '\r');
}

void BooleanExpression::SkipSpaces( const char** pStringPtr )
{
	const char*& pString = *pStringPtr;
	while ( IsSpace(*pString) )
		++pString;
}

bool BooleanExpression::IsOneOf( char cChar, const char* pChar )
{
	while ( *pChar != 0 )
	{
		if( cChar == *pChar )
			return true;
		++pChar;
	}
	return false;
}
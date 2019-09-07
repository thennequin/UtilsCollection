
#include "JsonStthm.h"

#include <stdio.h> // FILE, fopen, fclose, fwrite, fread

// Experimental long/double parser
//#define STTHM_USE_CUSTOM_NUMERIC_PARSER

namespace JsonStthm
{
	namespace Internal
	{
		const unsigned long _c_lInfinity[2]	= { 0x00000000, 0x7ff00000 };
		const unsigned long _c_lNaN[2]		= { 0x00000000, 0x7ff80000 };

		const double c_fInfinity			= *(double*)&_c_lInfinity;
		const double c_fNegativeInfinity	= -c_fInfinity;
		const double c_fNaN					= *(double*)&_c_lNaN;

		bool IsNaN(double x)
		{
			return memcmp(&c_fNaN, &x, sizeof(double)) == 0;
		}

		bool IsInfinite(double x)
		{
			return memcmp(&c_fInfinity, &x, sizeof(double)) == 0
				|| memcmp(&c_fNegativeInfinity, &x, sizeof(double)) == 0;
		}
	}

	//////////////////////////////
	// JsonValue::Iterator
	//////////////////////////////
	JsonValue::Iterator::Iterator(const JsonValue* pJson)
	{
		if (pJson != NULL && pJson->IsContainer())
		{
			m_pChild = pJson->m_oChilds.m_pFirst;
		}
		else
		{
			m_pChild = NULL;
		}
	}

	JsonValue::Iterator::Iterator(const Iterator& oIt)
	{
		m_pChild = oIt.m_pChild;
	}

	bool JsonValue::Iterator::IsValid() const
	{
		return m_pChild != NULL;
	}

	bool JsonValue::Iterator::operator!=(const Iterator& oIte) const
	{
		return m_pChild != oIte.m_pChild;
	}

	void JsonValue::Iterator::operator++()
	{
		if (m_pChild != NULL)
			m_pChild = m_pChild->m_pNext;
	}

	JsonValue* JsonValue::Iterator::operator*() const
	{
		return m_pChild;
	}

	JsonValue* JsonValue::Iterator::operator->() const
	{
		return m_pChild;
	}

	//////////////////////////////
	// JsonValue
	//////////////////////////////
	JsonValue JsonValue::INVALID = JsonValue::CreateConst();

	JsonValue::JsonValue()
		: m_bConst( false )
		, m_eType( E_TYPE_INVALID )
		, m_pName( NULL )
		, m_pNext( NULL )
	{
	}

	JsonValue::JsonValue(const JsonValue& oSource)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		m_bConst = oSource.m_bConst;
		*this = oSource;
	}

	JsonValue::JsonValue(bool bValue)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = bValue;
	}

#ifdef STTHM_USE_STD_STRING
	JsonValue::JsonValue(const std::string& sValue)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = sValue;
	}
#endif //STTHM_USE_STD_STRING

	JsonValue::JsonValue(const char* pValue)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = pValue;
	}

	JsonValue::JsonValue(int64_t iValue)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = iValue;
	}

	JsonValue::JsonValue(double fValue)
		: m_bConst(false)
		, m_eType(E_TYPE_INVALID)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = fValue;
	}

	JsonValue::~JsonValue()
	{
		JsonStthmFree(m_pName);
		m_pName = NULL;
		Reset();
	}

	JsonValue JsonValue::CreateConst()
	{
		JsonValue oValue;
		oValue.m_bConst = true;
		return oValue;
	}

	void JsonValue::InitType(EType eType)
	{
		if (m_eType != eType)
		{
			Reset();

			m_eType = eType;
			switch (eType)
			{
			case E_TYPE_OBJECT:
			case E_TYPE_ARRAY:
				m_oChilds.m_pFirst = NULL;
				m_oChilds.m_pLast = NULL;
				break;
			case E_TYPE_STRING:
				m_pString = NULL;
				break;
			default:
				break;
			}
		}
	}

	JsonValue::EType JsonValue::GetType() const
	{
		return m_eType;
	}

	void JsonValue::Reset()
	{
		switch (m_eType)
		{
		case E_TYPE_OBJECT:
		case E_TYPE_ARRAY:
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			while (pChild != NULL)
			{
				JsonValue* pTemp = pChild->m_pNext;
				delete pChild;
				pChild = pTemp;
			}
			m_oChilds.m_pFirst = NULL;
			m_oChilds.m_pLast = NULL;
		}
		break;
		case E_TYPE_STRING:
			JsonStthmFree(m_pString);
			m_pString = NULL;
			break;
		default:
			break;
		}
		m_eType = E_TYPE_INVALID;
	}

	void JsonValue::SetString(const char* pString)
	{
		JsonStthmFree(m_pString);
		m_pString = NULL;
		if (NULL != pString)
		{
			size_t iLen  = 1 + strlen(pString);
			char* pNewString = (char*)JsonStthmMalloc(iLen);
			memcpy(pNewString, pString, iLen);
			m_pString = pNewString;
		}
	}

	void JsonValue::Write(Internal::CharBuffer& sOutJson, int iIndent, bool bCompact)
	{
		if (m_eType == E_TYPE_OBJECT)
		{
			Internal::CharBuffer sIndent(iIndent, '\t');
			Internal::CharBuffer sIndent2(iIndent + 1, '\t');
			sOutJson += '{';
			JsonValue* pChild = m_oChilds.m_pFirst;
			bool bFirst = true;
			while (pChild != NULL)
			{
				if (!bFirst)
				{
					sOutJson += ',';
				}
				else
				{
					bFirst = false;
				}

				if (!bCompact)
				{
					sOutJson += '\n';
					sOutJson.PushRange(sIndent2.Data(), sIndent2.Size());
				}

				sOutJson += '\"';
				WriteStringEscaped(sOutJson, pChild->m_pName);
				sOutJson += '\"';
				sOutJson += ':';
				if (bCompact == false)
					sOutJson += ' ';

				pChild->Write(sOutJson, iIndent + 1, bCompact);
				pChild = pChild->m_pNext;
			}
			if (!bCompact)
			{
				sOutJson += '\n';
				sOutJson.PushRange(sIndent.Data(), sIndent.Size());
			}
			sOutJson += '}';
		}
		else if (m_eType == E_TYPE_ARRAY)
		{
			Internal::CharBuffer sIndent(iIndent, '\t');
			Internal::CharBuffer sIndent2(iIndent + 1, '\t');
			sOutJson += '[';
			JsonValue* pChild = m_oChilds.m_pFirst;
			bool bFirst = true;
			while (pChild != NULL)
			{
				if (!bFirst)
				{
					sOutJson += ',';
				}
				else
				{
					bFirst = false;
				}

				if (!bCompact)
				{
					sOutJson += '\n';
					sOutJson.PushRange(sIndent2.Data(), sIndent2.Size());
				}

				pChild->Write(sOutJson, iIndent + 1, bCompact);
				pChild = pChild->m_pNext;
			}
			if (!bCompact)
			{
				sOutJson += '\n';
				sOutJson.PushRange(sIndent.Data(), sIndent.Size());
			}
			sOutJson += ']';
		}
		else if (m_eType == E_TYPE_STRING)
		{
			sOutJson += '\"';
			WriteStringEscaped(sOutJson, m_pString);
			sOutJson += '\"';
		}
		else if (m_eType == E_TYPE_BOOLEAN)
		{
			if (m_bBoolean)
			{
				sOutJson.PushRange("true", 4);
			}
			else
			{
				sOutJson.PushRange("false", 5);
			}
		}
		else if (m_eType == E_TYPE_INTEGER)
		{
			char sBuffer[256];
			sprintf_s(sBuffer, 256, "%lld", m_iInteger);
			size_t iLen = strlen(sBuffer);
			sOutJson.PushRange(sBuffer, iLen);
		}
		else if (m_eType == E_TYPE_FLOAT)
		{
			if (Internal::IsNaN(m_fFloat))
			{
				sOutJson.PushRange("NaN", 3);
			}
			else if (Internal::IsInfinite(m_fFloat))
			{
				if (m_fFloat < 0.f)
					sOutJson.PushRange("-Infinity", 9);
				else
					sOutJson.PushRange("Infinity", 8);
			}
			else
			{
				char sBuffer[256];
				sprintf_s(sBuffer, 256, "%.17Lg", m_fFloat);
				size_t iLen = strlen(sBuffer);
				sOutJson.PushRange(sBuffer, iLen);
			}
		}
		else
		{
			sOutJson.PushRange("null", 4);
		}
	}

	void JsonValue::WriteStringEscaped(Internal::CharBuffer& sOutJson, const char* pInput)
	{
		while (*pInput != '\0')
		{
			char cChar = *pInput;
			if (cChar == '\n')
			{
				sOutJson.PushRange("\\n", 2);
			}
			else if (cChar == '\r')
			{
				sOutJson.PushRange("\\r", 2);
			}
			else if (cChar == '\t')
			{
				sOutJson.PushRange("\\t", 2);
			}
			else if (cChar == '\b')
			{
				sOutJson.PushRange("\\b", 2);
			}
			else if (cChar == '\f')
			{
				sOutJson.PushRange("\\f", 2);
			}
			else if (cChar == '"')
			{
				sOutJson.PushRange("\\\"", 2);
			}
			else if (cChar == '\\')
			{
				sOutJson.PushRange("\\\\", 2);
			}
			else if ((unsigned char)cChar < 0x80)
			{
				sOutJson += cChar;
			}
			else
			{
				sOutJson.PushRange("\\u", 2);
				unsigned int iChar = (unsigned char)cChar;
				if ((iChar & 0xE0) == 0xC0) // 2 byte
				{
					iChar = ((((unsigned char)pInput[0]) & 0x1F) << 6)
						+	((((unsigned char)pInput[1]) & 0x3F));
					pInput += 1;
				}
				else if ((iChar & 0xF0) == 0xE0) // 3 bytes
				{
					iChar = ((((unsigned char)pInput[0]) & 0x0F) << 12) 
						+	((((unsigned char)pInput[1]) & 0x3F) << 6) 
						+	((((unsigned char)pInput[2]) & 0x3F));
					pInput += 2;
				}
				else if ((iChar & 0xF8) == 0xF0 && iChar <= 0xF4) // 4 bytes
				{
					iChar = ((((unsigned char)pInput[0]) & 0x07) << 18)
						+	((((unsigned char)pInput[1]) & 0x3F) << 12)
						+	((((unsigned char)pInput[2]) & 0x3F) << 6)
						+	((((unsigned char)pInput[3]) & 0x3F));
					pInput += 3;
				}
				else
				{
					//Invalid char
					iChar = -1;
				}

				char sHexa[5];
				const char* const  pHexa = "0123456789ABCDEF";
				sHexa[0] = pHexa[(iChar >> 12) & 0x0f];
				sHexa[1] = pHexa[(iChar >> 8) & 0x0f];
				sHexa[2] = pHexa[(iChar >> 4) & 0x0f];
				sHexa[3] = pHexa[(iChar) & 0x0f];
				sHexa[4] = '\0';

				sOutJson.PushRange(sHexa, 4);
			}

			++pInput;
		}
	}

#ifdef STTHM_USE_STD_STRING
	void JsonValue::WriteString(std::string& sOutJson, bool bCompact)
	{
		Internal::CharBuffer oBuffer;
		Write(oBuffer, 0, bCompact);
		sOutJson.resize(oBuffer.Size());
		oBuffer.WriteTo((char*)sOutJson.data());
	}
#endif //STTHM_USE_STD_STRING

	bool JsonValue::WriteFile(const char* pFilename, bool bCompact)
	{
		FILE* pFile = fopen(pFilename, "w");
		if (NULL != pFile)
		{
			Internal::CharBuffer sJson;
			Write(sJson, 0, bCompact);
			bool bRet = fwrite(sJson.Data(), sizeof(char), sJson.Size(), pFile) == (sizeof(char) * sJson.Size());
			fclose(pFile);
			return bRet;
		}
		return false;
	}

	int JsonValue::GetMemberCount() const
	{
		int iCount = 0;
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			while (pChild != NULL)
			{
				++iCount;
				pChild = pChild->m_pNext;
			}
		}

		return iCount;
	}

	const char* JsonValue::ToString() const
	{
		if (m_eType == E_TYPE_STRING)
			return m_pString;
		return NULL;
	}

	bool JsonValue::ToBoolean() const
	{
		if (m_eType == E_TYPE_BOOLEAN)
			return m_bBoolean;
		return false;
	}

	int64_t JsonValue::ToInteger() const
	{
		if (m_eType == E_TYPE_INTEGER)
			return m_iInteger;
		else if (m_eType == E_TYPE_FLOAT)
			return (int64_t)m_fFloat;
		return 0;
	}

	double JsonValue::ToFloat() const
	{
		if (m_eType == E_TYPE_FLOAT)
			return m_fFloat;
		else if (m_eType == E_TYPE_INTEGER)
			return (double)m_iInteger;
		return 0.0;
	}

	bool JsonValue::operator ==(const JsonValue& oRight) const
	{
		if (m_eType != oRight.m_eType)
			return false;

		switch (m_eType)
		{
		case E_TYPE_OBJECT:
		{
			// We don't care if members order is not the same
			const JsonValue* pChildLeft = m_oChilds.m_pFirst;
			while (pChildLeft != NULL)
			{
				if (*pChildLeft != oRight[pChildLeft->m_pName])
					return false;

				pChildLeft = pChildLeft->m_pNext;
			}
			break;
		}
		case E_TYPE_ARRAY:
		{
			JsonValue* pChildLeft = m_oChilds.m_pFirst;
			JsonValue* pChildRight = oRight.m_oChilds.m_pFirst;
			while (pChildLeft != NULL && pChildRight != NULL)
			{
				if (*pChildLeft != *pChildRight)
					return false;

				pChildLeft = pChildLeft->m_pNext;
				pChildRight = pChildRight->m_pNext;
			}

			if (pChildLeft != NULL || pChildRight != NULL)
				return false;
			break;
		}
		case E_TYPE_STRING:
			if (strcmp(m_pString, oRight.m_pString) != 0)
				return false;
			break;
		case E_TYPE_BOOLEAN:
			if (memcmp(&m_bBoolean, &oRight.m_bBoolean, sizeof(m_bBoolean)) != 0)
				return false;
			break;
		case E_TYPE_INTEGER:
			if (memcmp(&m_iInteger, &oRight.m_iInteger, sizeof(m_iInteger)) != 0)
				return false;
			break;
		case E_TYPE_FLOAT:
			if (memcmp(&m_fFloat, &oRight.m_fFloat, sizeof(m_fFloat)) != 0)
				return false;
			break;
		}

		return true;
	}

	bool JsonValue::operator !=(const JsonValue& oRight) const
	{
		return (*this == oRight) == false;
	}

	const JsonValue& JsonValue::operator[](const char* pName) const
	{
		if (m_eType == E_TYPE_OBJECT)
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			while (pChild != NULL)
			{
				if (strcmp(pChild->m_pName, pName) == 0)
					return *pChild;
				if (pChild->m_pNext == NULL)
					break;
				pChild = pChild->m_pNext;
			}
		}
		return JsonValue::INVALID;
	}

	JsonValue& JsonValue::operator[](const char* pName)
	{
		if (pName == NULL || pName[0] == 0)
			return JsonValue::INVALID;

		if (m_eType == E_TYPE_INVALID)
			InitType(E_TYPE_OBJECT);
		if (m_eType == E_TYPE_OBJECT)
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			while (pChild != NULL)
			{
				if (strcmp(pChild->m_pName, pName) == 0)
					return *pChild;
				if (pChild->m_pNext == NULL)
					break;
				pChild = pChild->m_pNext;
			}
			if (!m_bConst)
			{
				JsonValue* pNewMember = new JsonValue();

				size_t iNameLen = strlen(pName) + 1;
				void* pNewString = JsonStthmMalloc(iNameLen);
				memcpy(pNewString, (const void*)pName, iNameLen);
				pNewMember->m_pName = (char*)pNewString;

				if (NULL != m_oChilds.m_pLast)
					m_oChilds.m_pLast->m_pNext = pNewMember;
				else
					m_oChilds.m_pFirst = pNewMember;

				m_oChilds.m_pLast = pNewMember;
				return *pNewMember;
			}
		}
		return JsonValue::INVALID;
	}

	const JsonValue& JsonValue::operator [](int iIndex) const
	{
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			int iCurrent = 0;
			while (pChild != NULL)
			{
				if (iCurrent++ == iIndex)
					return *pChild;
				pChild = pChild->m_pNext;
			}
		}
		return JsonValue::INVALID;
	}

	JsonValue& JsonValue::operator[](int iIndex)
	{
		if (m_eType == E_TYPE_INVALID)
			InitType(E_TYPE_ARRAY);
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pChild = m_oChilds.m_pFirst;
			int iCurrent = 0;
			while (pChild != NULL)
			{
				if (iCurrent++ == iIndex)
					return *pChild;
				if (pChild->m_pNext == NULL)
					break;
				pChild = pChild->m_pNext;
			}
			if (m_eType == E_TYPE_ARRAY)
			{
				do
				{
					JsonValue* pNewChild = new JsonValue();

					if (NULL != m_oChilds.m_pLast)
						m_oChilds.m_pLast->m_pNext = pNewChild;
					else
						m_oChilds.m_pFirst = pNewChild;

					m_oChilds.m_pLast = pNewChild;
				}
				while (iCurrent++ != iIndex);
				return *m_oChilds.m_pLast;
			}
		}
		return JsonValue::INVALID;
	}

	JsonValue& JsonValue::operator =(const JsonValue& oValue)
	{
		if (oValue.m_eType == E_TYPE_OBJECT || oValue.m_eType == E_TYPE_ARRAY)
		{
			InitType(oValue.m_eType);

			JsonValue* pSourceChild = oValue.m_oChilds.m_pFirst;;
			while (pSourceChild != NULL)
			{
				JsonValue* pNewChild = new JsonValue(*pSourceChild);
				if (NULL != m_oChilds.m_pLast)
					m_oChilds.m_pLast->m_pNext = pNewChild;
				else
					m_oChilds.m_pFirst = pNewChild;

				m_oChilds.m_pLast = pNewChild;

				pSourceChild = pSourceChild->m_pNext;
			}
		}
		else if (oValue.m_eType == E_TYPE_BOOLEAN)
		{
			*this = oValue.ToBoolean();
		}
		else if (oValue.m_eType == E_TYPE_STRING)
		{
			*this = oValue.ToString();
		}
		else if (oValue.m_eType == E_TYPE_INTEGER)
		{
			*this = oValue.ToInteger();
		}
		else if (oValue.m_eType == E_TYPE_FLOAT)
		{
			*this = oValue.ToFloat();
		}
		return *this;
	}

#ifdef STTHM_USE_STD_STRING
	JsonValue& JsonValue::operator =(const std::string& sValue)
	{
		if (!m_bConst)
		{
			InitType(E_TYPE_STRING);
			SetString(sValue.c_str());
		}
		return *this;
	}
#endif //STTHM_USE_STD_STRING

	JsonValue& JsonValue::operator =(const char* pValue)
	{
		if (!m_bConst)
		{
			if (NULL != pValue)
			{
				InitType(E_TYPE_STRING);
				SetString(pValue);
			}
			else
			{
				InitType(E_TYPE_INVALID);
			}
		}
		return *this;
	}

	JsonValue& JsonValue::operator =(bool bValue)
	{
		if (!m_bConst)
		{
			InitType(E_TYPE_BOOLEAN);
			m_bBoolean = bValue;
		}
		return *this;
	}

	JsonValue& JsonValue::operator =(int64_t iValue)
	{
		if (!m_bConst)
		{
			InitType(E_TYPE_INTEGER);
			m_iInteger = iValue;
		}
		return *this;
	}

	JsonValue& JsonValue::operator =(double fValue)
	{
		if (!m_bConst)
		{
			InitType(E_TYPE_FLOAT);
			m_fFloat = fValue;
		}
		return *this;
	}

	JsonValue& JsonValue::operator +=(const JsonValue& oValue)
	{
		if (m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pNewValue = new JsonValue(oValue);

			if (NULL != m_oChilds.m_pLast)
				m_oChilds.m_pLast->m_pNext = pNewValue;
			else
				m_oChilds.m_pFirst = pNewValue;

			m_oChilds.m_pLast = pNewValue;
		}
		else if (m_eType == E_TYPE_STRING)
		{
			if (oValue.IsString())
			{
				Internal::CharBuffer oNewStr;
				const char* pAddString = oValue.ToString();
				size_t iLen1 = strlen(m_pString);
				size_t iLen2 = strlen(pAddString);
				oNewStr.Reserve(iLen1 + iLen2 + 1);
				oNewStr.PushRange(m_pString, iLen1);
				oNewStr.PushRange(pAddString, iLen2);
				oNewStr.Push(0);
				SetString(oNewStr.Data());
			}
		}
		return *this;
	}

	JsonValue::operator const char*() const
	{
		return ToString();
	}

	JsonValue::operator bool() const
	{
		return ToBoolean();
	}

	JsonValue::operator long() const
	{
		return ToInteger();
	}

	JsonValue::operator double() const
	{
		return ToFloat();
	}

	//Reading

	bool JsonValue::IsSpace(char cChar) {
		return cChar == ' ' || (cChar >= '\t' && cChar <= '\r');
	}

	bool JsonValue::IsDigit(char cChar)
	{
		return (cChar >= '0' && cChar <= '9');
	}

	bool JsonValue::IsXDigit(char cChar)
	{
		return (cChar >= '0' && cChar <= '9') || ((cChar & ~' ') >= 'A' && (cChar & ~' ') <= 'F') || ((cChar & ~' ') >= 'a' && (cChar & ~' ') <= 'f');
	}

	int	JsonValue::CharToInt(char cChar)
	{
		if (cChar <= '9')
			return cChar - '0';
		else
			return (cChar & ~' ') - 'A' + 10;
	}

	void JsonValue::SkipSpaces(const char*& pString)
	{
		while (IsSpace(*pString)) ++pString;
	}

	bool JsonValue::ReadSpecialChar(const char*& pString, Internal::CharBuffer& oTempBuffer)
	{
		if (*pString == 'n') oTempBuffer += '\n';
		else if (*pString == 'r') oTempBuffer += '\r';
		else if (*pString == 't') oTempBuffer += '\t';
		else if (*pString == 'b') oTempBuffer += '\b';
		else if (*pString == 'f') oTempBuffer += '\f';
		else if (*pString == '"') oTempBuffer += '"';
		else if (*pString == '\\') oTempBuffer += '\\';
		else if (*pString == '\/') oTempBuffer += '\/';
		else if (*pString == 'u')
		{
			int iChar = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (IsXDigit(*++pString))
					iChar = iChar * 16 + CharToInt((unsigned char)*pString);
				else
					return false;
			}
			if (iChar < 0x0080)
			{
				oTempBuffer += (char)iChar;
			}
			else if (iChar >= 0x80 && iChar < 0x800)
			{
				oTempBuffer += (char)(0xC0 | (iChar >> 6));
				oTempBuffer += (char)(0x80 | (iChar & 0x3F));
			}
			else if (iChar >= 0x800 && iChar < 0x7FFF)
			{
				oTempBuffer += (char)(0xE0 | (iChar >> 12));
				oTempBuffer += (char)(0x80 | ((iChar >> 6) & 0x3F));
				oTempBuffer += (char)(0x80 | (iChar & 0x3F));
			}
			else if (iChar >= 0x8000 && iChar < 0x7FFFF)
			{
				oTempBuffer += (char)(0xF0 | (iChar >> 18));
				oTempBuffer += (char)(0xE0 | ((iChar >> 12) & 0x3F));
				oTempBuffer += (char)(0x80 | ((iChar >> 6) & 0x3F));
				oTempBuffer += (char)(0x80 | (iChar & 0x3F));
			}
			else
			{
				return false;
			}
			return true;
		}
		else
			return false;

		return true;
	}

	bool JsonValue::ReadStringValue(const char*& pString, Internal::CharBuffer& oTempBuffer)
	{
		oTempBuffer.Clear();
		while (*pString != 0)
		{
			if (*pString == '\\')
			{
				++pString;
				if (!ReadSpecialChar(pString, oTempBuffer))
					return false;
				++pString;
			}
			else if (*pString == '"')
			{
				oTempBuffer += '\0';
				++pString;
				return true;
			}
			else if ((unsigned int)*pString < ' ' || *pString == '\x7F')
			{
				return false;
			}
			else
			{
				oTempBuffer += *pString;
				++pString;
			}
		}
		return false;
	}

	bool JsonValue::ReadStringValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer)
	{
		if (ReadStringValue(pString, oTempBuffer))
		{
			oValue.InitType(E_TYPE_STRING);
			oValue.m_pString = oTempBuffer.Take();
			//oValue = oTempBuffer.Data();//Use Take();
			return true;
		}
		return false;
	}

	bool JsonValue::ReadNumericValue(const char*& pString, JsonValue& oValue)
	{
	#ifdef STTHM_USE_CUSTOM_NUMERIC_PARSER
		static double const c_pExpTable[] = {
			1e5, 1e4, 1e3, 1e2, 10, 1,
			0.1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6,
			1e-7, 1e-8, 1e-9, 1e-10, 1e-11, 1e-12,
			1e-13, 1e-14, 1e-15, 1e-16, 1e-17
		};
		static double const* c_pExpLookup = &c_pExpTable[5];

		bool bNeg = false;
		if (*pString == '-')
		{
			++pString;
			bNeg = true;
		}

		uint64_t lValue = 0;
		int iNegFract = 0;

		while (IsDigit(*pString))
			lValue = lValue * 10 + (*pString++ & 0xF);

		if (*pString == '.')
		{
			const char* pStart = ++pString;

			while (IsDigit(*pString))
				lValue = lValue * 10 + (*pString++ & 0xF);

			iNegFract = (int)(pString - pStart);

			if (*pString == 'e' || *pString == 'E')
			{
				++pString;

				bool bNegExp = false;
				if (*pString == '+')
				{
					++pString;
				}
				else if (*pString == '-')
				{
					++pString;
					bNegExp = true;
				}

				uint64_t iExpValue = 0;
				while (IsDigit(*pString))
					iExpValue = iExpValue * 10 + (*pString++ & 0xF);

				iNegFract += bNegExp ? (int)iExpValue : -(int)iExpValue;
			}
			oValue = (bNeg ? -1.0 : 1.0) * lValue * c_pExpLookup[iNegFract];
		}
		else
		{
			//TODO manage E/e for long?
			oValue = (int64_t)(bNeg ? -lValue : lValue);
		}
		return true;
	#else //STTHM_USE_CUSTOM_NUMERIC_PARSER
		char* pEndDouble;
		char* pEndLong;
		double fValue = strtod( pString, &pEndDouble );
		long long iValue = strtoll( pString, &pEndLong, 10 );
		if( pEndDouble > pEndLong )
		{
			pString = pEndDouble;
			oValue = fValue;
		}
		else
		{
			pString = pEndLong;
			oValue = iValue;
		}

		return true;
	#endif // !STTHM_USE_CUSTOM_NUMERIC_PARSER
	}

	bool JsonValue::ReadObjectValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer)
	{
		oValue.InitType(JsonValue::E_TYPE_OBJECT);

		SkipSpaces( pString );

		if( *pString == '}' )
		{
			++pString;
			return true;
		}

		while (*pString != 0)
		{
			SkipSpaces(pString);

			// Read member name
			if (*pString != '"' || !ReadStringValue(++pString, oTempBuffer))
				return false;

			JsonValue* pNewMember = new JsonValue();
			pNewMember->m_pName = oTempBuffer.Take();

			SkipSpaces(pString);

			if (*pString != ':')
				return false;

			++pString;

			SkipSpaces(pString);

			if (!pNewMember->Parse(pString, oTempBuffer))
			{
				delete pNewMember;
				return false;
			}

			if (oValue.m_oChilds.m_pFirst == NULL)
			{
				oValue.m_oChilds.m_pFirst = pNewMember;
			}
			else
			{
				oValue.m_oChilds.m_pLast->m_pNext = pNewMember;
			}
			oValue.m_oChilds.m_pLast = pNewMember;

			SkipSpaces(pString);

			if (*pString == '}')
			{
				++pString;
				return true;
			}
			else if (*pString != ',')
			{
				return false;
			}
			++pString;
		}
		return false;
	}

	bool JsonValue::ReadArrayValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer)
	{
		oValue.InitType(JsonValue::E_TYPE_ARRAY);

		SkipSpaces( pString );
		if( *pString == ']' )
		{
			++pString;
			return true;
		}

		while (*pString != 0)
		{
			SkipSpaces(pString);

			JsonValue* pNewValue = new JsonValue();

			if (!pNewValue->Parse(pString, oTempBuffer))
			{
				delete pNewValue;
				return false;
			}

			if (oValue.m_oChilds.m_pFirst == NULL)
			{
				oValue.m_oChilds.m_pFirst = pNewValue;
			}
			else
			{
				oValue.m_oChilds.m_pLast->m_pNext = pNewValue;
			}
			oValue.m_oChilds.m_pLast = pNewValue;

			SkipSpaces(pString);

			if (*pString == ']')
			{
				++pString;
				return true;
			}
			else if (*pString != ',')
			{
				return false;
			}
			++pString;
		}
		return false;
	}

	int JsonValue::ReadString(const char* pJson)
	{
		if (pJson != NULL)
		{
			Internal::CharBuffer oTempBuffer;
			Reset();
			const char* pEnd = pJson;
			if (!Parse(pEnd, oTempBuffer))
			{
				int iLine = 1;
				int iReturn = 1;
				while (pJson != pEnd)
				{
					if (*pJson == '\n')
						++iLine;
					else if (*pJson == '\r')
						++iReturn;
					++pJson;
				}
				if (iReturn > iLine)
					iLine = iReturn;
				return iLine;
			}
			return 0;
		}
		return -1;
	}

	const bool JsonValue::Parse(const char*& pString, Internal::CharBuffer& oTempBuffer)
	{
		bool bOk = pString != NULL && *pString != 0;
		while (*pString != 0 && bOk)
		{
			while (IsSpace(*pString)) ++pString;
			if (*pString == '"')
			{
				++pString;
				if (!ReadStringValue(pString, *this, oTempBuffer))
					bOk = false;
				break;
			}
			else if (memcmp(pString, "NaN", 3) == 0 )
			{
				pString += 3;
				*this = Internal::c_fNaN;
				break;
			}
			else if (memcmp(pString, "-Infinity", 9) == 0 )
			{
				pString += 9;
				*this = -Internal::c_fInfinity;
				break;
			}
			else if (memcmp(pString, "Infinity", 8) == 0 )
			{
				pString += 8;
				*this = Internal::c_fInfinity;
				break;
			}
			else if (IsDigit(*pString) || *pString == '-')
			{
				if (!ReadNumericValue(pString, *this))
					bOk = false;
				break;
			}
			else if (memcmp(pString, "true", 4) == 0 )
			{
				pString += 4;
				*this = true;
				break;
			}
			else if (memcmp(pString, "false", 5) == 0 )
			{
				pString += 5;
				*this = false;
				break;
			}
			else if (memcmp(pString, "null", 4) == 0 )
			{
				pString += 4;
				InitType(E_TYPE_INVALID);
				break;
			}
			else if (*pString == '{')
			{
				++pString;
				if (!ReadObjectValue(pString, *this, oTempBuffer))
				{
					bOk = false;
				}
				break;
			}
			else if (*pString == '[')
			{
				++pString;
				if (!ReadArrayValue(pString, *this, oTempBuffer))
					bOk = false;
				break;
			}
			else
			{
				//Error
				bOk = false;
				break;
			}
		}
		return bOk;
	}

	int JsonValue::ReadFile(const char* pFilename)
	{
		FILE* pFile = fopen(pFilename, "r");
		if (NULL != pFile)
		{
			Reset();

			fseek(pFile, 0, SEEK_END);
			long iSize = ftell(pFile);
			fseek(pFile, 0, SEEK_SET);

			char* pString = (char*)JsonStthmMalloc(iSize);
			fread(pString, 1, iSize, pFile);
			fclose(pFile);

			int iLine = ReadString(pString);

			JsonStthmFree(pString);
			return iLine;
		}
		return -1;
	}
}
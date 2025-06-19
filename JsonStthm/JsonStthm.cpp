#include "JsonStthm.h"

#include <stdio.h> // FILE, fopen, fclose, fwrite, fread

// Experimental long/double parser
//#define STTHM_USE_CUSTOM_NUMERIC_PARSER

namespace JsonStthm
{
	namespace Internal
	{
		const uint32_t _c_lInfinity[2]			= { 0x00000000, 0x7ff00000 };
		const uint32_t _c_lNaN[2]				= { 0x00000000, 0x7ff80000 };
		const uint32_t _c_lNaNMask				= 0x7ff00000;
		const uint32_t _c_lNaNPayloadMask[2]	= { 0xffffFFFF, 0x000fFFFF };

		const double c_fInfinity				= *(double*)&_c_lInfinity;
		const double c_fNegativeInfinity		= -c_fInfinity;
		const double c_fNaN						= *(double*)&_c_lNaN;

		bool IsNaN(double x)
		{
			// Checking NaN Payload not equal 0
			uint32_t* pLong = (uint32_t*)&x;
			return (pLong[1] & _c_lNaNMask) == _c_lNaNMask &&
				(
					(pLong[0] & _c_lNaNPayloadMask[0]) != 0 ||
					(pLong[1] & _c_lNaNPayloadMask[1]) != 0
				);
		}

		bool IsInfinite(double x)
		{
			return memcmp(&c_fInfinity, &x, sizeof(double)) == 0
				|| memcmp(&c_fNegativeInfinity, &x, sizeof(double)) == 0;
		}

		bool IsSpace(char cChar)
		{
			return cChar == ' ' || (cChar >= '\t' && cChar <= '\r');
		}

		bool IsDigit(char cChar)
		{
			return (cChar >= '0' && cChar <= '9');
		}

		const unsigned char c_pXDigitLut[256] = {
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,
			255,  10,  11,  12,  13,  14,  15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255,  10,  11,  12,  13,  14,  15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
		};

		inline uint8_t XDigitToUInt8(unsigned char cChar)
		{
			return c_pXDigitLut[cChar];
		}

		void SkipSpaces(const char*& pString, const char* pEnd)
		{
			while (pString < pEnd && IsSpace(*pString)) ++pString;
		}

		int64_t StrToInt64(const char* pString, const char* pEnd, char** pCursor)
		{
			bool bNeg = false;
			if (pString < pEnd && *pString == '-')
			{
				++pString;
				bNeg = true;
			}

			int64_t lValue = 0;
			while (pString < pEnd && IsDigit(*pString))
				lValue = lValue * 10 + (*pString++ & 0xF);

			if (pCursor != NULL)
				*pCursor = (char*)pString;

			return bNeg ? -lValue : lValue;
		}
	}

	//////////////////////////////
	// JsonValue::Iterator
	//////////////////////////////

	JsonValue::Iterator::Iterator(const JsonValue* pJson)
	{
		if (pJson != NULL && pJson->IsContainer())
		{
			m_pChild = pJson->m_oValue.Childs.m_pFirst;
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

	const JsonValue& JsonValue::Iterator::operator*() const
	{
		return m_pChild != NULL ? *m_pChild : INVALID;
	}

	const JsonValue* JsonValue::Iterator::operator->() const
	{
		return m_pChild;
	}

	JsonValue::Iterator::operator const JsonValue&() const
	{
		return m_pChild != NULL ? *m_pChild : INVALID;
	}

	//////////////////////////////
	// JsonValue
	//////////////////////////////

	JsonValue JsonValue::INVALID;

	Allocator JsonValue::s_oDefaultAllocator = {
		JsonValue::DefaultAllocatorCreateJsonValue,
		JsonValue::DefaultAllocatorDeleteJsonValue,
		JsonValue::DefaultAllocatorAllocString,
		JsonValue::DefaultAllocatorFreeString,
		NULL
	};

	JsonValue::JsonValue(Allocator* pAllocator)
		: m_pAllocator(pAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		JsonStthmAssert(m_pAllocator != NULL);
	}

	JsonValue::JsonValue()
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
	}

	JsonValue::JsonValue(const JsonValue& oSource)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = oSource;
	}

	JsonValue::JsonValue(bool bValue)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = bValue;
	}

#ifdef JsonStthmString
	JsonValue::JsonValue(const JsonStthmString& sValue)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = sValue;
	}
#endif //JsonStthmString

	JsonValue::JsonValue(const char* pValue)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = pValue;
	}

	JsonValue::JsonValue(int64_t iValue)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = iValue;
	}

	JsonValue::JsonValue(double fValue)
		: m_pAllocator(&s_oDefaultAllocator)
		, m_eType(E_TYPE_NULL)
		, m_pName(NULL)
		, m_pNext(NULL)
	{
		*this = fValue;
	}

	JsonValue::~JsonValue()
	{
		if (m_pName != NULL)
		{
			m_pAllocator->FreeString(m_pName, m_pAllocator->pUserData);
			m_pName = NULL;
		}
		Reset();
	}

	void JsonValue::InitType(EType eType)
	{
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY || m_eType == E_TYPE_STRING)
			Reset();

		if (m_eType != eType)
		{
			m_eType = eType;
			switch (eType)
			{
			case E_TYPE_OBJECT:
			case E_TYPE_ARRAY:
				m_oValue.Childs.m_pFirst = NULL;
				m_oValue.Childs.m_pLast = NULL;
				break;
			case E_TYPE_STRING:
				m_oValue.String = NULL;
				break;
			default:
				break;
			}
		}
	}

	void JsonValue::Reset()
	{
		switch (m_eType)
		{
		case E_TYPE_OBJECT:
		case E_TYPE_ARRAY:
		{
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
			while (pChild != NULL)
			{
				JsonValue* pTemp = pChild->m_pNext;
				m_pAllocator->DeleteJsonValue(pChild, m_pAllocator->pUserData);
				pChild = pTemp;
			}
			m_oValue.Childs.m_pFirst = NULL;
			m_oValue.Childs.m_pLast = NULL;
			break;
		}
		case E_TYPE_STRING:
		{
			m_pAllocator->FreeString(m_oValue.String, m_pAllocator->pUserData);
			m_oValue.String = NULL;
			break;
		}
		default:
			break;
		}
		m_eType = E_TYPE_NULL;
	}

	JsonValue::EType JsonValue::GetType() const
	{
		return m_eType;
	}

	void JsonValue::SetStringValue(const char* pString, const char* pEnd)
	{
		JsonStthmAssert(IsString());
		m_pAllocator->FreeString(m_oValue.String, m_pAllocator->pUserData);
		m_oValue.String = NULL;
		if (NULL != pString)
		{
			JsonStthmAssert(pEnd == NULL || pEnd >= pString);
			size_t iLen = (pEnd != NULL) ? (pEnd - pString) : strlen(pString);
			char* pNewString = m_pAllocator->AllocString(iLen + 1, m_pAllocator->pUserData);
			memcpy(pNewString, pString, iLen);
			pNewString[iLen] = 0;
			m_oValue.String = pNewString;
		}
	}

	int JsonValue::ReadString(const char* pJson, const char* pJsonEnd)
	{
		if (pJson != NULL)
		{
			Reset();
			if (pJsonEnd == NULL)
			{
				pJsonEnd = pJson + strlen(pJson);
			}
			const char* pEnd = pJson;
			if (Parse(pEnd, pJsonEnd) == false)
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

	int JsonValue::ReadFile(const char* pFilename)
	{
		FILE* pFile = fopen(pFilename, "r");
		if (NULL != pFile)
		{
			Reset();

			fseek(pFile, 0, SEEK_END);
			long iSize = ftell(pFile);
			fseek(pFile, 0, SEEK_SET);

			char* pString = (char*)JsonStthmMalloc(iSize + 1);
			if (pString == NULL)
			{
				fclose(pFile);
				return -2;
			}

			fread(pString, 1, iSize, pFile);
			fclose(pFile);
			pString[iSize] = 0;

			int iLine = ReadString(pString, pString + iSize);

			JsonStthmFree(pString);
			return iLine;
		}
		return -1;
	}

	void JsonValue::Write(Internal::CharBuffer& sOutJson, size_t iIndent, bool bCompact) const
	{
		if (m_eType == E_TYPE_OBJECT)
		{
			sOutJson += '{';
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
			bool bFirst = true;
			while (pChild != NULL)
			{
				if (bFirst == false)
				{
					sOutJson += ',';
				}
				else
				{
					bFirst = false;
				}

				if (bCompact == false)
				{
					sOutJson += '\n';
					sOutJson.PushRepeat('\t', iIndent + 1);
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

			if (bCompact == false)
			{
				sOutJson += '\n';
				sOutJson.PushRepeat('\t', iIndent);
			}

			sOutJson += '}';
		}
		else if (m_eType == E_TYPE_ARRAY)
		{
			sOutJson += '[';
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
			bool bFirst = true;
			while (pChild != NULL)
			{
				if (bFirst == false)
				{
					sOutJson += ',';
				}
				else
				{
					bFirst = false;
				}

				if (bCompact == false)
				{
					sOutJson += '\n';
					sOutJson.PushRepeat('\t', iIndent + 1);
				}

				pChild->Write(sOutJson, iIndent + 1, bCompact);
				pChild = pChild->m_pNext;
			}
			if (bCompact == false)
			{
				sOutJson += '\n';
				sOutJson.PushRepeat('\t', iIndent);
			}
			sOutJson += ']';
		}
		else if (m_eType == E_TYPE_STRING)
		{
			sOutJson += '\"';
			WriteStringEscaped(sOutJson, m_oValue.String);
			sOutJson += '\"';
		}
		else if (m_eType == E_TYPE_BOOLEAN)
		{
			if (m_oValue.Boolean)
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
			snprintf(sBuffer, 256, "%jd", m_oValue.Integer);
			size_t iLen = strlen(sBuffer);
			sOutJson.PushRange(sBuffer, iLen);
		}
		else if (m_eType == E_TYPE_FLOAT)
		{
			if (Internal::IsNaN(m_oValue.Float))
			{
				sOutJson.PushRange("NaN", 3);
			}
			else if (Internal::IsInfinite(m_oValue.Float))
			{
				if (m_oValue.Float < 0.f)
					sOutJson.PushRange("-Infinity", 9);
				else
					sOutJson.PushRange("Infinity", 8);
			}
			else
			{
				char sBuffer[256];
				snprintf(sBuffer, 256, "%.17g", m_oValue.Float);
				size_t iLen = strlen(sBuffer);
				sOutJson.PushRange(sBuffer, iLen);
			}
		}
		else
		{
			sOutJson.PushRange("null", 4);
		}
	}

#ifdef JsonStthmString
	void JsonValue::WriteString(JsonStthmString& sOutJson, bool bCompact) const
	{
		Internal::CharBuffer oBuffer;
		Write(oBuffer, 0, bCompact);
		sOutJson.resize(oBuffer.Size());
		oBuffer.WriteTo((char*)sOutJson.data());
	}
#endif //JsonStthmString

	char* JsonValue::WriteString(bool bCompact) const
	{
		Internal::CharBuffer oBuffer;
		Write(oBuffer, 0, bCompact);
		char* pString = (char*)JsonStthmMalloc(oBuffer.Size() + 1);
		if (pString != NULL)
		{
			oBuffer.WriteTo(pString);
			pString[oBuffer.Size()] = '\0';
		}
		return pString;
	}

	bool JsonValue::WriteFile(const char* pFilename, bool bCompact) const
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
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
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
			return m_oValue.String;
		return NULL;
	}

	bool JsonValue::ToBoolean() const
	{
		if (m_eType == E_TYPE_BOOLEAN)
			return m_oValue.Boolean;
		return false;
	}

	int64_t JsonValue::ToInteger() const
	{
		if (m_eType == E_TYPE_INTEGER)
			return m_oValue.Integer;
		else if (m_eType == E_TYPE_FLOAT)
			return (int64_t)m_oValue.Float;
		return 0;
	}

	double JsonValue::ToFloat() const
	{
		if (m_eType == E_TYPE_FLOAT)
			return m_oValue.Float;
		else if (m_eType == E_TYPE_INTEGER)
			return (double)m_oValue.Integer;
		return 0.0;
	}

#ifdef STTHM_ENABLE_IMPLICIT_CAST
	JsonValue::operator const char*() const
	{
		return ToString();
	}

	JsonValue::operator bool() const
	{
		return ToBoolean();
	}

	JsonValue::operator int64_t() const
	{
		return ToInteger();
	}

	JsonValue::operator double() const
	{
		return ToFloat();
	}
#endif //STTHM_ENABLE_IMPLICIT_CAST

	void JsonValue::SetString(const char* pValue, const char* pEnd)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return;

		if (NULL != pValue)
		{
			InitType(E_TYPE_STRING);
			SetStringValue(pValue, pEnd);
		}
		else
		{
			InitType(E_TYPE_NULL);
		}
	}

	void JsonValue::SetBoolean(bool bValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return;

		InitType(E_TYPE_BOOLEAN);
		m_oValue.Boolean = bValue;
	}

	void JsonValue::SetInteger(int64_t iValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return;

		InitType(E_TYPE_INTEGER);
		m_oValue.Integer = iValue;
	}

	void JsonValue::SetFloat(double fValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return;

		InitType(E_TYPE_FLOAT);
		m_oValue.Float = fValue;
	}

	JsonValue& JsonValue::Append()
	{
		JsonStthmAssert(this != &INVALID);
		if (this == &INVALID)
			return INVALID;

		if (m_eType == E_TYPE_NULL)
			InitType(E_TYPE_ARRAY);

		if (m_eType != E_TYPE_ARRAY)
			return INVALID;

		// Append new element
		JsonValue* pNewChild = m_pAllocator->CreateJsonValue(m_pAllocator, m_pAllocator->pUserData);

		if (NULL != m_oValue.Childs.m_pLast)
			m_oValue.Childs.m_pLast->m_pNext = pNewChild;
		else
			m_oValue.Childs.m_pFirst = pNewChild;

		m_oValue.Childs.m_pLast = pNewChild;

		return *m_oValue.Childs.m_pLast;
	}

	bool JsonValue::Combine(const JsonValue& oRight, bool bMergeSubMembers)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return false;

		if (m_eType != oRight.m_eType)
			return false;

		switch(m_eType)
		{
		case E_TYPE_NULL:
			// Nothing to do
			break;
		case E_TYPE_OBJECT:
		{
			// Replace value or try to add recursively values of sub objects
			for (const JsonValue* pChild =  oRight.m_oValue.Childs.m_pFirst; pChild != NULL; pChild = pChild->m_pNext)
			{
				JsonValue& oLeftChild = (*this)[pChild->m_pName];
				if (bMergeSubMembers && oLeftChild.m_eType == pChild->m_eType) // Combine again is the childrens's type is the same otherwise replace it
				{
					(*this)[pChild->m_pName].Combine(*pChild, true);
				}
				else
				{
					(*this)[pChild->m_pName] = *pChild;
				}
			}
			break;
		}
		case E_TYPE_ARRAY:
		{
			// Append to array
			for (const JsonValue* pChild = oRight.m_oValue.Childs.m_pFirst; pChild != NULL; pChild = pChild->m_pNext)
			{
				Append() = *pChild;
			}
			break;
		}
		case E_TYPE_STRING:
		{
			// Concatenate strings
			size_t iLenLeft = strlen(m_oValue.String);
			size_t iLenRight = strlen(oRight.m_oValue.String);
			size_t iLenNew = iLenLeft + iLenRight;
			char* pNewString = m_pAllocator->AllocString(iLenNew + 1, m_pAllocator->pUserData);
			memcpy(pNewString, m_oValue.String, iLenLeft);
			memcpy(pNewString + iLenLeft, oRight.m_oValue.String, iLenRight);
			pNewString[iLenNew] = 0;
			m_pAllocator->FreeString(m_oValue.String, m_pAllocator->pUserData);
			m_oValue.String = pNewString;
			break;
		}
		case E_TYPE_BOOLEAN:
			m_oValue.Boolean = m_oValue.Boolean || oRight.m_oValue.Boolean;
			break;
		case E_TYPE_INTEGER:
			m_oValue.Integer += oRight.m_oValue.Integer;
			break;
		case E_TYPE_FLOAT:
			m_oValue.Float += oRight.m_oValue.Float;
			break;
		}

		return true;
	}

	bool JsonValue::operator ==(const JsonValue& oRight) const
	{
		if (m_eType != oRight.m_eType)
			return false;

		switch (m_eType)
		{
		case E_TYPE_NULL:
			break;
		case E_TYPE_OBJECT:
		{
			// We don't care if members order is not the same
			const JsonValue* pChildLeft = m_oValue.Childs.m_pFirst;
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
			JsonValue* pChildLeft = m_oValue.Childs.m_pFirst;
			JsonValue* pChildRight = oRight.m_oValue.Childs.m_pFirst;
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
			if (strcmp(m_oValue.String, oRight.m_oValue.String) != 0)
				return false;
			break;
		case E_TYPE_BOOLEAN:
			if (memcmp(&m_oValue.Boolean, &oRight.m_oValue.Boolean, sizeof(m_oValue.Boolean)) != 0)
				return false;
			break;
		case E_TYPE_INTEGER:
			if (memcmp(&m_oValue.Integer, &oRight.m_oValue.Integer, sizeof(m_oValue.Integer)) != 0)
				return false;
			break;
		case E_TYPE_FLOAT:
			if (memcmp(&m_oValue.Float, &oRight.m_oValue.Float, sizeof(m_oValue.Float)) != 0)
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
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
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
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		if (pName == NULL || pName[0] == 0)
			return JsonValue::INVALID;

		if (m_eType == E_TYPE_NULL)
			InitType(E_TYPE_OBJECT);
		if (m_eType == E_TYPE_OBJECT)
		{
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
			while (pChild != NULL)
			{
				if (strcmp(pChild->m_pName, pName) == 0)
					return *pChild;
				if (pChild->m_pNext == NULL)
					break;
				pChild = pChild->m_pNext;
			}

			JsonValue* pNewMember = m_pAllocator->CreateJsonValue(m_pAllocator, m_pAllocator->pUserData);

			size_t iNameLen = strlen(pName) + 1;
			void* pNewString = m_pAllocator->AllocString(iNameLen, m_pAllocator->pUserData);
			memcpy(pNewString, (const void*)pName, iNameLen);
			pNewMember->m_pName = (char*)pNewString;

			if (NULL != m_oValue.Childs.m_pLast)
				m_oValue.Childs.m_pLast->m_pNext = pNewMember;
			else
				m_oValue.Childs.m_pFirst = pNewMember;

			m_oValue.Childs.m_pLast = pNewMember;
			return *pNewMember;
		}
		return JsonValue::INVALID;
	}

	const JsonValue& JsonValue::operator[](char* pName) const
	{
		return (*this)[(const char*)pName];
	}

	JsonValue& JsonValue::operator[](char* pName)
	{
		return (*this)[(const char*)pName];
	}

	const JsonValue& JsonValue::operator [](int iIndex) const
	{
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
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
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		if (m_eType == E_TYPE_NULL)
			InitType(E_TYPE_ARRAY);
		if (m_eType == E_TYPE_OBJECT || m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pChild = m_oValue.Childs.m_pFirst;
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
					JsonValue* pNewChild = m_pAllocator->CreateJsonValue(m_pAllocator, m_pAllocator->pUserData);

					if (NULL != m_oValue.Childs.m_pLast)
						m_oValue.Childs.m_pLast->m_pNext = pNewChild;
					else
						m_oValue.Childs.m_pFirst = pNewChild;

					m_oValue.Childs.m_pLast = pNewChild;
				}
				while (iCurrent++ != iIndex);
				return *m_oValue.Childs.m_pLast;
			}
		}
		return JsonValue::INVALID;
	}

	JsonValue& JsonValue::operator =(const JsonValue& oValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		if (oValue.m_eType == E_TYPE_OBJECT)
		{
			InitType(E_TYPE_OBJECT);

			JsonValue* pSourceChild = oValue.m_oValue.Childs.m_pFirst;
			while (pSourceChild != NULL)
			{
				JsonValue* pNewChild = new JsonValue(*pSourceChild);

				if (pSourceChild->m_pName != NULL)
				{
					size_t iNameLen = strlen(pSourceChild->m_pName) + 1;
					char* pNewString = m_pAllocator->AllocString(iNameLen, m_pAllocator->pUserData);
					memcpy(pNewString, pSourceChild->m_pName, iNameLen);
					pNewChild->m_pName = pNewString;
				}

				if (NULL != m_oValue.Childs.m_pLast)
					m_oValue.Childs.m_pLast->m_pNext = pNewChild;
				else
					m_oValue.Childs.m_pFirst = pNewChild;

				m_oValue.Childs.m_pLast = pNewChild;

				pSourceChild = pSourceChild->m_pNext;
			}
		}
		else if (oValue.m_eType == E_TYPE_ARRAY)
		{
			InitType(E_TYPE_ARRAY);

			JsonValue* pSourceChild = oValue.m_oValue.Childs.m_pFirst;
			while (pSourceChild != NULL)
			{
				JsonValue* pNewChild = new JsonValue(*pSourceChild);

				if (NULL != m_oValue.Childs.m_pLast)
					m_oValue.Childs.m_pLast->m_pNext = pNewChild;
				else
					m_oValue.Childs.m_pFirst = pNewChild;

				m_oValue.Childs.m_pLast = pNewChild;

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
		else
		{
			InitType(E_TYPE_NULL);
		}
		return *this;
	}

#ifdef JsonStthmString
	JsonValue& JsonValue::operator =(const JsonStthmString& sValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		InitType(E_TYPE_STRING);
		SetStringValue(sValue.c_str());

		return *this;
	}
#endif //JsonStthmString

	JsonValue& JsonValue::operator =(const char* pValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		SetString(pValue);
		return *this;
	}

	JsonValue& JsonValue::operator =(bool bValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		SetBoolean(bValue);
		return *this;
	}

	JsonValue& JsonValue::operator =(int64_t iValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		SetInteger(iValue);
		return *this;
	}

	JsonValue& JsonValue::operator =(double fValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		SetFloat(fValue);
		return *this;
	}

	JsonValue& JsonValue::operator +=(const JsonValue& oValue)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID)
			return JsonValue::INVALID;

		if (m_eType == E_TYPE_ARRAY)
		{
			JsonValue* pNewValue = new JsonValue(oValue);

			if (NULL != m_oValue.Childs.m_pLast)
				m_oValue.Childs.m_pLast->m_pNext = pNewValue;
			else
				m_oValue.Childs.m_pFirst = pNewValue;

			m_oValue.Childs.m_pLast = pNewValue;
		}
		else if (m_eType == E_TYPE_STRING)
		{
			if (oValue.IsString())
			{
				Internal::CharBuffer oNewStr;
				const char* pAddString = oValue.ToString();
				size_t iLen1 = strlen(m_oValue.String);
				size_t iLen2 = strlen(pAddString);
				oNewStr.Reserve(iLen1 + iLen2 + 1);
				oNewStr.PushRange(m_oValue.String, iLen1);
				oNewStr.PushRange(pAddString, iLen2);
				oNewStr.Push(0);
				SetStringValue(oNewStr.Data());
			}
		}
		return *this;
	}

	bool JsonValue::Parse(const char*& pString, const char* pEnd)
	{
		JsonStthmAssert(this != &JsonStthm::JsonValue::INVALID);
		if (this == &JsonStthm::JsonValue::INVALID || pString == NULL)
			return false;

		Internal::SkipSpaces(pString, pEnd);
		if (pString >= pEnd || *pString == 0)
		{
			return true;
		}
		else if (*pString == '"')
		{
			char* pValue = ReadStringValue(++pString, pEnd, m_pAllocator);
			if (pValue == NULL)
			{
				return false;
			}

			InitType(E_TYPE_STRING);
			m_oValue.String = pValue;
			return true;
		}
		else if ((pEnd - pString) >= 3 && memcmp(pString, "NaN", 3) == 0)
		{
			pString += 3;
			InitType(E_TYPE_FLOAT);
			m_oValue.Float = Internal::c_fNaN;
			return true;
		}
		else if ((pEnd - pString) >= 9 && memcmp(pString, "-Infinity", 9) == 0)
		{
			pString += 9;
			InitType(E_TYPE_FLOAT);
			m_oValue.Float = -Internal::c_fInfinity;
			return true;
		}
		else if ((pEnd - pString) >= 8 && memcmp(pString, "Infinity", 8) == 0)
		{
			pString += 8;
			InitType(E_TYPE_FLOAT);
			m_oValue.Float = Internal::c_fInfinity;
			return true;
		}
		else if (Internal::IsDigit(*pString) || *pString == '-')
		{
			return ReadNumericValue(pString, pEnd, *this);
		}
		else if ((pEnd - pString) >= 4 && memcmp(pString, "true", 4) == 0)
		{
			pString += 4;
			InitType(E_TYPE_BOOLEAN);
			m_oValue.Boolean = true;
			return true;
		}
		else if ((pEnd - pString) >= 5 && memcmp(pString, "false", 5) == 0)
		{
			pString += 5;
			InitType(E_TYPE_BOOLEAN);
			m_oValue.Boolean = false;
			return true;
		}
		else if ((pEnd - pString) >= 4 && memcmp(pString, "null", 4) == 0)
		{
			pString += 4;
			InitType(E_TYPE_NULL);
			return true;
		}
		else if (*pString == '{')
		{
			++pString;
			return ReadObjectValue(pString, pEnd, *this);
		}
		else if (*pString == '[')
		{
			++pString;
			return ReadArrayValue(pString, pEnd, *this);
		}

		// Error
		return false;
	}

	// Static functions

	int JsonValue::ReadSpecialChar(const char*& pString, const char* pEnd, char* pOut)
	{
		if( pString >= pEnd )
			return 0;

		if (*pString == 'n')		{ pOut[0] = '\n';	return 1; }
		else if (*pString == 'r')	{ pOut[0] = '\r';	return 1; }
		else if (*pString == 't')	{ pOut[0] = '\t';	return 1; }
		else if (*pString == 'b')	{ pOut[0] = '\b';	return 1; }
		else if (*pString == 'f')	{ pOut[0] = '\f';	return 1; }
		else if (*pString == '"')	{ pOut[0] = '"';	return 1; }
		else if (*pString == '\\')	{ pOut[0] = '\\';	return 1; }
		else if (*pString == '/')	{ pOut[0] = '/';	return 1; }
		else if (*pString == 'u')
		{
			uint32_t iChar = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (pString >= pEnd)
					return 0;

				uint8_t iXDigit = Internal::XDigitToUInt8(*++pString);
				if (iXDigit != 255)
					iChar = iChar * 16 + iXDigit;
				else
					return 0;
			}

			if (iChar >= 0xD800 && iChar <= 0xDFFF) // UTF16 Surrogate pair
			{
				if ((iChar & 0xFC00) != 0xD800)
					return 0; //Invalid first pair code

				if (pString >= pEnd || *++pString != '\\' || *++pString != 'u')
					return 0; //Not a valid pair

				uint32_t iChar2 = 0;
				for (int i = 0; i < 4; ++i)
				{
					if (pString >= pEnd)
						return 0;

					uint8_t iXDigit = Internal::XDigitToUInt8(*++pString);
					if (iXDigit != 255)
						iChar2 = iChar2 * 16 + iXDigit;
					else
						return 0;
				}

				if ((iChar2 & 0xFC00) != 0xDC00)
					return 0; //Invalid second pair code

				iChar = 0x10000 + (((iChar & 0x3FF) << 10) | (iChar2 & 0x3FF));
			}

			if (iChar < 0x80)
			{
				pOut[0] = (char)iChar;
				return 1;
			}
			if (iChar >= 0x80 && iChar <= 0x7FF)
			{
				pOut[0] = (char)(0xC0 | ((iChar >> 6)));
				pOut[1] = (char)(0x80 | ((iChar >> 0) & 0x3F));
				return 2;
			}
			if (iChar >= 0x800 && iChar <= 0xFFFF)
			{
				pOut[0] = (char)(0xE0 | ((iChar >> 12)));
				pOut[1] = (char)(0x80 | ((iChar >> 6) & 0x3F));
				pOut[2] = (char)(0x80 | ((iChar >> 0) & 0x3F));
				return 3;
			}
			if (iChar >= 0x10000 && iChar <= 0x10FFFF)
			{
				pOut[0] = (char)(0xF0 | ((iChar >> 18)));
				pOut[1] = (char)(0x80 | ((iChar >> 12) & 0x3F));
				pOut[2] = (char)(0x80 | ((iChar >> 6) & 0x3F));
				pOut[3] = (char)(0x80 | ((iChar >> 0) & 0x3F));
				return 4;
			}
		}

		return 0;
	}

	char* JsonValue::ReadStringValue(const char*& pString, const char* pEnd, Allocator* pAllocator)
	{
		size_t iLen = 0;
		// Read string length
		{
			char pTemp[4];
			const char* pCursor = pString;
			while (pCursor < pEnd && *pCursor != 0)
			{
				if (*pCursor == '\\')
				{
					int iCharLen = ReadSpecialChar(++pCursor, pEnd, pTemp);
					if (iCharLen == 0)
						return NULL;
					iLen += iCharLen;
					++pCursor;
					continue;
				}
				else if (*pCursor == '"')
				{
					++pCursor;
					break;
				}

				++iLen;
				++pCursor;
			}
		}

		// Alloc string
		char* pNewString = pAllocator->AllocString(iLen + 1, pAllocator->pUserData);
		pNewString[iLen] = '\0';
		char* pNewStringCursor = pNewString;

		// Read string
		{
			while (pString < pEnd && *pString != 0)
			{
				if (*pString == '\\')
				{
					int iCharLen = ReadSpecialChar(++pString, pEnd, pNewStringCursor);
					if (iCharLen == 0)
						return NULL;
					pNewStringCursor += iCharLen;
					++pString;
					continue;
				}
				else if (*pString == '"')
				{
					++pString;
					return pNewString;
				}

				*pNewStringCursor = *pString;
				++pNewStringCursor;
				++pString;
			}
		}
		return NULL;
	}

	bool JsonValue::ReadNumericValue(const char*& pString, const char* pEnd, JsonValue& oValue)
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

		while (pString < pEnd && Internal::IsDigit(*pString))
			lValue = lValue * 10 + (*pString++ & 0xF);

		if (*pString == '.')
		{
			const char* pStart = ++pString;

			while (pString < pEnd && Internal::IsDigit(*pString))
				lValue = lValue * 10 + (*pString++ & 0xF);

			iNegFract = (int)(pString - pStart);

			if (pString < pEnd && (*pString == 'e' || *pString == 'E'))
			{
				++pString;

				bool bNegExp = false;
				if (pString < pEnd && *pString == '+')
				{
					++pString;
				}
				else if (pString < pEnd && *pString == '-')
				{
					++pString;
					bNegExp = true;
				}

				uint64_t iExpValue = 0;
				while (pString < pEnd && Internal::IsDigit(*pString))
					iExpValue = iExpValue * 10 + (*pString++ & 0xF);

				iNegFract += bNegExp ? (int)iExpValue : -(int)iExpValue;
			}
			oValue = (bNeg ? -1.0 : 1.0) * lValue * c_pExpLookup[iNegFract];
		}
		else
		{
			//TODO manage E/e for long?
			oValue = (int64_t)(bNeg ? -(int64_t)lValue : lValue);
		}
		return true;
	#else //STTHM_USE_CUSTOM_NUMERIC_PARSER
		char* pEndDouble;
		char* pEndLong;
		double fValue = strtod(pString, &pEndDouble); // TODO : Unsafe
		int64_t iValue = Internal::StrToInt64(pString, pEnd, &pEndLong);
		if (pEndDouble > pEndLong)
		{
			pString = pEndDouble;
			oValue.InitType(E_TYPE_FLOAT);
			oValue.m_oValue.Float = fValue;
		}
		else
		{
			pString = pEndLong;
			oValue.InitType(E_TYPE_INTEGER);
			oValue.m_oValue.Integer= iValue;
		}

		return true;
	#endif // !STTHM_USE_CUSTOM_NUMERIC_PARSER
	}

	bool JsonValue::ReadObjectValue(const char*& pString, const char* pEnd, JsonValue& oValue)
	{
		oValue.InitType(JsonValue::E_TYPE_OBJECT);

		Internal::SkipSpaces(pString, pEnd);

		if (pString < pEnd && *pString == '}')
		{
			++pString;
			return true;
		}

		while (pString < pEnd && *pString != 0)
		{
			Internal::SkipSpaces(pString, pEnd);

			// Read member name
			if (pString >= pEnd || *pString != '"')
				return false;

			char* pName = ReadStringValue(++pString, pEnd, oValue.m_pAllocator);
			if (pName == NULL)
				return false;

			JsonValue* pNewMember = oValue.m_pAllocator->CreateJsonValue(oValue.m_pAllocator, oValue.m_pAllocator->pUserData);
			pNewMember->m_pName = pName;

			Internal::SkipSpaces(pString, pEnd);

			if (pString >= pEnd || *pString != ':')
				return false;

			++pString;

			Internal::SkipSpaces(pString, pEnd);

			if (pNewMember->Parse(pString, pEnd) == false)
			{
				oValue.m_pAllocator->DeleteJsonValue(pNewMember, oValue.m_pAllocator->pUserData);
				return false;
			}

			if (oValue.m_oValue.Childs.m_pFirst == NULL)
			{
				oValue.m_oValue.Childs.m_pFirst = pNewMember;
			}
			else
			{
				oValue.m_oValue.Childs.m_pLast->m_pNext = pNewMember;
			}
			oValue.m_oValue.Childs.m_pLast = pNewMember;

			Internal::SkipSpaces(pString, pEnd);

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

	bool JsonValue::ReadArrayValue(const char*& pString, const char* pEnd, JsonValue& oValue)
	{
		oValue.InitType(JsonValue::E_TYPE_ARRAY);

		Internal::SkipSpaces(pString, pEnd);
		if (*pString == ']')
		{
			++pString;
			return true;
		}

		while (pString < pEnd && *pString != 0)
		{
			Internal::SkipSpaces(pString, pEnd);

			JsonValue* pNewValue = oValue.m_pAllocator->CreateJsonValue(oValue.m_pAllocator, oValue.m_pAllocator->pUserData);

			if (pNewValue->Parse(pString, pEnd) == false)
			{
				oValue.m_pAllocator->DeleteJsonValue(pNewValue, oValue.m_pAllocator->pUserData);

				return false;
			}

			if (oValue.m_oValue.Childs.m_pFirst == NULL)
			{
				oValue.m_oValue.Childs.m_pFirst = pNewValue;
			}
			else
			{
				oValue.m_oValue.Childs.m_pLast->m_pNext = pNewValue;
			}
			oValue.m_oValue.Childs.m_pLast = pNewValue;

			Internal::SkipSpaces(pString, pEnd);

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
				uint32_t iChar = (unsigned char)cChar;
				if ((iChar & 0xE0) == 0xC0) // 2 byte
				{
					iChar = ((((unsigned char)pInput[0]) & 0x1F) << 6)
						+ ((((unsigned char)pInput[1]) & 0x3F) << 0);
					pInput += 1;
				}
				else if ((iChar & 0xF0) == 0xE0) // 3 bytes
				{
					iChar = ((((unsigned char)pInput[0]) & 0x0F) << 12)
						+ ((((unsigned char)pInput[1]) & 0x3F) << 6)
						+ ((((unsigned char)pInput[2]) & 0x3F) << 0);
					pInput += 2;
				}
				else if ((iChar & 0xF8) == 0xF0 && iChar <= 0xF4) // 4 bytes
				{
					iChar = ((((unsigned char)pInput[0]) & 0x07) << 18)
						+ ((((unsigned char)pInput[1]) & 0x3F) << 12)
						+ ((((unsigned char)pInput[2]) & 0x3F) << 6)
						+ ((((unsigned char)pInput[3]) & 0x3F) << 0);
					pInput += 3;
				}
				else
				{
					//Invalid char
					JsonStthmAssert(false);
					iChar = 0;
				}

				const char* const pHexa = "0123456789abcdef";

				if (iChar < 0xFFFF)
				{
					sOutJson.PushRange("\\u", 2);
					sOutJson.Push(pHexa[(iChar >> 12) & 0x0f]);
					sOutJson.Push(pHexa[(iChar >> 8) & 0x0f]);
					sOutJson.Push(pHexa[(iChar >> 4) & 0x0f]);
					sOutJson.Push(pHexa[(iChar >> 0) & 0x0f]);
				}
				else
				{
					//UTF-16 pair surrogate
					uint32_t iCharBis = iChar - 0x10000;
					uint32_t iW1 = 0xD800 | ((iCharBis >> 10) & 0x3FF);
					uint32_t iW2 = 0xDC00 | (iCharBis & 0x3FF);
					sOutJson.PushRange("\\u", 2);
					sOutJson.Push(pHexa[(iW1 >> 12) & 0x0f]);
					sOutJson.Push(pHexa[(iW1 >> 8) & 0x0f]);
					sOutJson.Push(pHexa[(iW1 >> 4) & 0x0f]);
					sOutJson.Push(pHexa[(iW1 >> 0) & 0x0f]);
					sOutJson.PushRange("\\u", 2);
					sOutJson.Push(pHexa[(iW2 >> 12) & 0x0f]);
					sOutJson.Push(pHexa[(iW2 >> 8) & 0x0f]);
					sOutJson.Push(pHexa[(iW2 >> 4) & 0x0f]);
					sOutJson.Push(pHexa[(iW2 >> 0) & 0x0f]);
				}
			}

			++pInput;
		}
	}

	JsonValue* JsonValue::DefaultAllocatorCreateJsonValue(Allocator* pAllocator, void* /*pUserData*/)
	{
		return new JsonValue(pAllocator);
	}

	void JsonValue::DefaultAllocatorDeleteJsonValue(JsonValue* pValue, void* /*pUserData*/)
	{
		delete pValue;
	}

	char* JsonValue::DefaultAllocatorAllocString(size_t iSize, void* /*pUserData*/)
	{
		return (char*)JsonStthmMalloc(iSize);
	}

	void JsonValue::DefaultAllocatorFreeString(char* pString, void* /*pUserData*/)
	{
		JsonStthmFree(pString);
	}

	//////////////////////////////
	// JsonDoc
	//////////////////////////////

	JsonDoc::JsonDoc(size_t iBlockSize)
		: m_oRoot(&m_oAllocator)
		, m_iBlockSize(iBlockSize)
		, m_pLastBlock(NULL)
	{
		m_oAllocator.CreateJsonValue	= &JsonDoc::CreateJsonValue;
		m_oAllocator.DeleteJsonValue	= &JsonDoc::DeleteJsonValue;
		m_oAllocator.AllocString		= &JsonDoc::AllocString;
		m_oAllocator.FreeString			= &JsonDoc::FreeString;
		m_oAllocator.pUserData			= this;
	}

	JsonDoc::~JsonDoc()
	{
		Clear();
	}

	void JsonDoc::Clear()
	{
		m_oRoot.m_eType = JsonValue::E_TYPE_NULL;
		Block* pBlock = m_pLastBlock;
		while (pBlock != NULL)
		{
			Block* pPrevious = pBlock->m_pPrevious;
			JsonStthmFree(pBlock);
			pBlock = pPrevious;
		}
		m_pLastBlock = NULL;
	}

	int JsonDoc::ReadString(const char* pJson, const char* pEnd)
	{
		Clear();
		return m_oRoot.ReadString(pJson, pEnd);
	}

	int JsonDoc::ReadFile(const char* pFilename)
	{
		Clear();
		return m_oRoot.ReadFile(pFilename);
	}

	void* JsonDoc::Allocate(JsonDoc* pDoc, size_t iSize, size_t iAlign)
	{
		Block* pHead = pDoc->m_pLastBlock;
		if (pHead != NULL && (iSize + iAlign + pHead->m_iUsed) <= pDoc->m_iBlockSize)
		{
			char* pMem = ((char*)pHead) + pHead->m_iUsed;
			size_t iAlignOffset = iAlign - ((intptr_t)pMem % iAlign);
			pMem += iAlignOffset;

			JsonStthmAssert((intptr_t)pMem % iAlign == 0);

			pHead->m_iUsed += iSize + iAlignOffset;
			return pMem;
		}

		size_t iAllocSize = sizeof(Block) + iSize + iAlign;
		size_t iBlockSize = (iAllocSize <= pDoc->m_iBlockSize) ? pDoc->m_iBlockSize : iAllocSize;
		Block* pBlock = (Block*)JsonStthmMalloc(iBlockSize);

		char* pMem = (char*)(pBlock + 1);
		size_t iAlignOffset = iAlign - ((intptr_t)pMem % iAlign);
		pMem += iAlignOffset;

		pBlock->m_iUsed = sizeof(Block) + iSize + iAlignOffset;

		if (iAllocSize <= iBlockSize || pHead == NULL)
		{
			pBlock->m_pPrevious = pHead;
			pDoc->m_pLastBlock = pBlock;
		}
		else
		{
			pBlock->m_pPrevious = pHead->m_pPrevious;
			pHead->m_pPrevious = pBlock;
		}

		JsonStthmAssert((intptr_t)pMem % iAlign == 0);

		return pMem;
	}

	JsonValue* JsonDoc::CreateJsonValue(Allocator* pAllocator, void* pUserData)
	{
		JsonValue* pValue = (JsonValue*)Allocate((JsonDoc*)pUserData, sizeof(JsonValue), alignof(JsonValue));
		if (pValue != NULL)
		{
			memset(pValue, 0, sizeof(JsonValue));
			pValue->m_pAllocator = pAllocator;
			return pValue;
		}
		return NULL;
	}

	void JsonDoc::DeleteJsonValue(JsonValue* /*pValue*/, void* /*pUserData*/)
	{
		// Do nothing
	}

	char* JsonDoc::AllocString(size_t iSize, void* pUserData)
	{
		return (char*)Allocate((JsonDoc*)pUserData, iSize, 1);
	}

	void JsonDoc::FreeString(char* /*pString*/, void* /*pUserData*/)
	{
		// Do nothing
	}

	size_t JsonDoc::MemoryUsage() const
	{
		Block* pBlock = m_pLastBlock;
		size_t iSize = 0;
		while (pBlock != NULL)
		{
			iSize += (pBlock->m_iUsed > m_iBlockSize) ? pBlock->m_iUsed : m_iBlockSize;
			pBlock = pBlock->m_pPrevious;
		}
		return iSize;
	}
}

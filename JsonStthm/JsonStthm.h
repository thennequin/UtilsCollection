#ifndef __JSON_STTHM_H__
#define __JSON_STTHM_H__

#include "JsonStthmConfig.h"

#include <string.h> //memcpy
#include <stdint.h> //int64_t

namespace JsonStthm
{
	namespace Internal
	{
		bool							IsNaN(double x);
		bool							IsInfinite(double x);

		bool							IsSpace(char cChar);
		bool							IsDigit(char cChar);
		bool							IsXDigit(char cChar);
		int								CharToInt(char cChar);
		void							SkipSpaces(const char*& pString);
		int64_t							StrToInt64(const char* pString, char** pEnd);

		template <typename T, size_t HeapSize = 1024>
		struct Buffer
		{
		public:
			Buffer(size_t iReserve = 8)
			{
				m_pData = m_pHeapData;
				m_iSize = m_iCapacity = 0;
				m_bUseHeap = true;
				Reserve(iReserve);
			}

			Buffer(size_t iSize, const T& oValue)
			{
				m_pData = m_pHeapData;
				m_iSize = m_iCapacity = 0;
				m_bUseHeap = true;
				Resize(iSize);
				for (size_t i = 0; i < iSize; ++i)
				{
					m_pData[i] = oValue;
				}
			}

			~Buffer()
			{
				if (!m_bUseHeap)
					JsonStthmFree(m_pData);
			}

			Buffer<T>& operator +=(const T& oValue)
			{
				Push(oValue);
				return *this;
			}

			void Push(const T& oValue)
			{
				Resize(m_iSize + 1);
				m_pData[m_iSize - 1] = oValue;
			}

			void PushRange(const T* pBegin, size_t iLength)
			{
				Resize(m_iSize + iLength);
				memcpy(&m_pData[m_iSize - iLength], pBegin, iLength);
			}

			size_t Size() const
			{
				return m_iSize;
			}

			void Reserve(size_t iCapacity, bool bForceAlloc = false)
			{
				if (iCapacity != m_iCapacity)
				{
					if (!m_bUseHeap || iCapacity >= HeapSize || bForceAlloc)
					{
						T* pTemp = (T*)JsonStthmMalloc(iCapacity * sizeof(T));
						JsonStthmAssert(pTemp != NULL);
						memcpy(pTemp, m_pData, m_iSize * sizeof(T));
						if (!m_bUseHeap)
							JsonStthmFree(m_pData);
						m_pData = pTemp;
						m_bUseHeap = false;
					}

					m_iCapacity = iCapacity;
				}
			}

			void Resize(size_t iSize)
			{
				size_t iNewCapacity = m_iCapacity > 0 ? m_iCapacity : 8;
				while (iSize > iNewCapacity)
					iNewCapacity *= 2;
				if (iNewCapacity != m_iCapacity)
					Reserve(iNewCapacity);
				m_iSize = iSize;
			}

			const T* Data() const { return m_pData; }

			T* Take()
			{
				char* pTemp;
				if (m_bUseHeap)
				{
					pTemp = (T*)JsonStthmMalloc(m_iSize * sizeof(T));
					memcpy(pTemp, m_pHeapData, m_iSize * sizeof(T));
				}
				else
				{
					pTemp = m_pData;
					m_pData = NULL;
				}
				m_iCapacity = 0;
				m_iSize = 0;
				m_bUseHeap = true;
				m_pData = m_pHeapData;
				return pTemp;
			}

			void WriteTo(T* pOut)
			{
				memcpy(pOut, m_pData, m_iSize * sizeof(T));
			}

			void Clear()
			{
				m_iSize = 0;
			}
		protected:
			T		m_pHeapData[HeapSize];
			T*		m_pData;
			size_t	m_iSize;
			size_t	m_iCapacity;
			bool	m_bUseHeap;
		};

		typedef Buffer<char> CharBuffer;
	}

	class STTHM_API JsonValue
	{
	public:
		enum EType
		{
			E_TYPE_INVALID = 0,	//null
			E_TYPE_OBJECT,		//JsonMembers
			E_TYPE_ARRAY,		//JsonArray
			E_TYPE_STRING,		//String
			E_TYPE_BOOLEAN,		//bool
			E_TYPE_INTEGER,		//long
			E_TYPE_FLOAT		//double
		};

		class Iterator
		{
		public:
			Iterator(const JsonValue* pJson);
			Iterator(const Iterator& oIt);

			bool IsValid() const;
			bool operator!=(const Iterator& oIte) const;
			void operator++();
			JsonValue* operator*() const;
			JsonValue* operator->() const;
		protected:
			JsonValue* m_pChild;
		};

		static JsonValue	INVALID;
	public:
							JsonValue();
							JsonValue(const JsonValue& oSource);
							JsonValue(bool bValue);
#ifdef JsonStthmString
							JsonValue(const JsonStthmString& sValue);
#endif //JsonStthmString
							JsonValue(const char* pValue);
							JsonValue(int64_t iValue);
							JsonValue(double fValue);
							~JsonValue();

		void				InitType(EType eType);
		void				Reset();
		EType				GetType() const;

		int					ReadString(const char* pJson);
		int					ReadFile(const char* pFilename);

		void				Write(Internal::CharBuffer& sOutJson, int iIndent, bool bCompact);
#ifdef JsonStthmString
		void				WriteString(JsonStthmString& sOutJson, bool bCompact = false);
#endif //JsonStthmString
		char*				WriteString(bool bCompact);
		bool				WriteFile(const char* pFilename, bool bCompact = false);

		int					GetMemberCount() const;

		const char*			GetName() const { return m_pName; }

		bool				IsNull() const { return m_eType == E_TYPE_INVALID; }
		bool				IsObject() const { return m_eType == E_TYPE_OBJECT; }
		bool				IsArray() const { return m_eType == E_TYPE_ARRAY; }
		bool				IsString() const { return m_eType == E_TYPE_STRING; }
		bool				IsBoolean() const { return m_eType == E_TYPE_BOOLEAN; }
		bool				IsInteger() const { return m_eType == E_TYPE_INTEGER; }
		bool				IsFloat() const { return m_eType == E_TYPE_FLOAT; }

		bool				IsNumeric() const { return m_eType == E_TYPE_INTEGER || m_eType == E_TYPE_FLOAT; }
		bool				IsContainer() const { return m_eType == E_TYPE_ARRAY || m_eType == E_TYPE_OBJECT; }

		const char*			ToString() const;
		bool				ToBoolean() const;
		int64_t				ToInteger() const;
		double				ToFloat() const;

		void				SetString(const char* pValue);
		void				SetBoolean(bool bValue);
		void				SetInteger(int64_t iValue);
		void				SetFloat(double fValue);

		bool				operator ==(const JsonValue& oRight) const;
		bool				operator !=(const JsonValue& oRight) const;

		const JsonValue&	operator [](const char* pName) const;
		JsonValue&			operator [](const char* pName);

		const JsonValue&	operator [](int iIndex) const;
		JsonValue&			operator [](int iIndex);

		JsonValue&			operator =(const JsonValue& oValue);
#ifdef JsonStthmString
		JsonValue&			operator =(const JsonStthmString& sValue);
#endif //JsonStthmString
		JsonValue&			operator =(const char* pValue);
		JsonValue&			operator =(bool bValue);
		JsonValue&			operator =(int64_t iValue);
		JsonValue&			operator =(double fValue);

		JsonValue&			operator +=(const JsonValue& oValue);
	protected:
		void				SetStringValue(const char* pString);

		bool				m_bConst;
		EType				m_eType;
		char*				m_pName;
		JsonValue*			m_pNext;

		struct JsonChilds
		{
			JsonValue*		m_pFirst;
			JsonValue*		m_pLast;
		};
		union
		{
			JsonChilds		m_oChilds;
			char*			m_pString;
			bool			m_bBoolean;
			int64_t			m_iInteger;
			double			m_fFloat;
		};

		bool				Parse(const char*& pString, Internal::CharBuffer& oTempBuffer);

		static JsonValue	CreateConst();

		static inline bool	ReadSpecialChar(const char*& pString, Internal::CharBuffer& oTempBuffer);
		static inline bool	ReadStringValue(const char*& pString, Internal::CharBuffer& oTempBuffer);
		static inline bool	ReadStringValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer);
		static inline bool	ReadNumericValue(const char*& pString, JsonValue& oValue);
		static inline bool	ReadObjectValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer);
		static inline bool	ReadArrayValue(const char*& pString, JsonValue& oValue, Internal::CharBuffer& oTempBuffer);
		static void			WriteStringEscaped(Internal::CharBuffer& sOutJson, const char* pBuffer);
	};
}

#endif // __JSON_STTHM_H__

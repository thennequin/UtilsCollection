#ifndef __JSON_STTHM_H__
#define __JSON_STTHM_H__

#include "JsonStthmConfig.h"

#include <string.h> //memcpy
#include <stdint.h> //int64_t

namespace JsonStthm
{
	class JsonValue;

	struct Allocator
	{
		JsonValue*					(*CreateJsonValue)	(Allocator* pAllocator, void* pUserData);
		void						(*DeleteJsonValue)	(JsonValue* pValue, void* pUserData);
		char*						(*AllocString)		(size_t iSize, void* pUserData);
		void						(*FreeString)		(char* pAlloc, void* pUserData);
		void*						pUserData;
	};

	namespace Internal
	{
		bool IsNaN(double x);
		bool IsInfinite(double x);

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
				memcpy(&m_pData[m_iSize - iLength], pBegin, iLength * sizeof(T));
			}

			void PushRepeat(const T& oValue, size_t iCount)
			{
				Resize(m_iSize + iCount);
				for (size_t i = 0; i < iCount; ++i)
				{
					m_pData[m_iSize - iCount + i] = oValue;
				}
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

			T* Take(Allocator* pAllocator)
			{
				char* pTemp;
				if (pAllocator != NULL)
				{
					pTemp = pAllocator->AllocString(m_iSize * sizeof(T), pAllocator->pUserData);
					memcpy(pTemp, m_bUseHeap ? m_pHeapData : m_pData, m_iSize * sizeof(T));
					m_iSize = 0;
					return pTemp;
				}
				else if (m_bUseHeap)
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
		friend class JsonDoc;
	public:
		enum EType
		{
			E_TYPE_NULL = 0,	//null
			E_TYPE_OBJECT,		//JsonMembers
			E_TYPE_ARRAY,		//JsonArray
			E_TYPE_STRING,		//String
			E_TYPE_BOOLEAN,		//bool
			E_TYPE_INTEGER,		//long
			E_TYPE_FLOAT		//double
		};

		class STTHM_API Iterator
		{
		public:
			Iterator(const JsonValue* pJson);
			Iterator(const Iterator& oIt);

			bool IsValid() const;
			bool operator!=(const Iterator& oIte) const;
			void operator++();
			const JsonValue& operator*() const;
			const JsonValue* operator->() const;
			operator const JsonValue& () const;
		protected:
			const JsonValue* m_pChild;
		};

		static JsonValue	INVALID;
	protected:
							JsonValue(Allocator* pAllocator);
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

		int					ReadString(const char* pJson, const char* pJsonEnd = NULL);
		int					ReadFile(const char* pFilename);

		void				Write(Internal::CharBuffer& sOutJson, size_t iIndent, bool bCompact) const;
#ifdef JsonStthmString
		void				WriteString(JsonStthmString& sOutJson, bool bCompact = false) const;
#endif //JsonStthmString
		char*				WriteString(bool bCompact) const;
		bool				WriteFile(const char* pFilename, bool bCompact = false) const;

		int					GetMemberCount() const;

		const char*			GetName() const { return m_pName; }
		const JsonValue*	GetNext() const { return m_pNext; }

		bool				IsValid() const		{ return this != &JsonValue::INVALID; }
		bool				IsNull() const		{ return m_eType == E_TYPE_NULL; }
		bool				IsObject() const	{ return m_eType == E_TYPE_OBJECT; }
		bool				IsArray() const		{ return m_eType == E_TYPE_ARRAY; }
		bool				IsString() const	{ return m_eType == E_TYPE_STRING; }
		bool				IsBoolean() const	{ return m_eType == E_TYPE_BOOLEAN; }
		bool				IsInteger() const	{ return m_eType == E_TYPE_INTEGER; }
		bool				IsFloat() const		{ return m_eType == E_TYPE_FLOAT; }

		bool				IsNumeric() const	{ return m_eType == E_TYPE_INTEGER || m_eType == E_TYPE_FLOAT; }
		bool				IsContainer() const	{ return m_eType == E_TYPE_ARRAY || m_eType == E_TYPE_OBJECT; }

		const char*			ToString() const;
		bool				ToBoolean() const;
		int64_t				ToInteger() const;
		double				ToFloat() const;

#ifdef STTHM_ENABLE_IMPLICIT_CAST
							operator const char*() const;
							operator bool() const;
							operator int64_t() const;
							operator double() const;
#endif //STTHM_ENABLE_IMPLICIT_CAST

		void				SetString(const char* pValue, const char* pEnd = NULL);
		void				SetBoolean(bool bValue);
		void				SetInteger(int64_t iValue);
		void				SetFloat(double fValue);

		JsonValue&			Append();

		// bMergeSubMembers parameter is for object type and gonna try to merge sub members when possible
		// Other types will just add values
		bool				Combine(const JsonValue& oRight, bool bMergeSubMembers);

		bool				operator ==(const JsonValue& oRight) const;
		bool				operator !=(const JsonValue& oRight) const;

		const JsonValue&	operator [](const char* pName) const;
		JsonValue&			operator [](const char* pName);

		const JsonValue&	operator [](char* pName) const;
		JsonValue&			operator [](char* pName);

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

		const Iterator		begin() const
		{
			return Iterator(this);
		}
		const Iterator		end() const
		{
			return Iterator(NULL);
		}

		Iterator			begin()
		{
			return Iterator(this);
		}
		Iterator			end()
		{
			return Iterator(NULL);
		}

#ifdef STTHM_CUSTOM_FUNCTIONS
		STTHM_CUSTOM_FUNCTIONS
#endif // STTHM_CUSTOM_FUNCTIONS
	protected:
		void				SetStringValue(const char* pString, const char* pEnd = NULL);

		Allocator*			m_pAllocator;

		EType				m_eType;
		char*				m_pName;
		JsonValue*			m_pNext;

		struct JsonChilds
		{
			JsonValue*		m_pFirst;
			JsonValue*		m_pLast;
		};

		union ValueUnion
		{
			JsonChilds		Childs;
			char*			String;
			bool			Boolean;
			int64_t			Integer;
			double			Float;
		};

		ValueUnion			m_oValue;

		bool				Parse(const char*& pString, const char* pEnd);

		static inline int	ReadSpecialChar(const char*& pString, const char* pEnd, char* pOut);
		static inline char*	ReadStringValue(const char*& pString, const char* pEnd, Allocator* pAllocator);
		static inline bool	ReadNumericValue(const char*& pString, const char* pEnd, JsonValue& oValue);
		static inline bool	ReadObjectValue(const char*& pString, const char* pEnd, JsonValue& oValue);
		static inline bool	ReadArrayValue(const char*& pString, const char* pEnd, JsonValue& oValue);
		static void			WriteStringEscaped(Internal::CharBuffer& sOutJson, const char* pBuffer);

		static JsonValue*	DefaultAllocatorCreateJsonValue(Allocator* pAllocator, void* pUserData);
		static void			DefaultAllocatorDeleteJsonValue(JsonValue* pValue, void* pUserData);
		static char*		DefaultAllocatorAllocString(size_t iSize, void* pUserData);
		static void			DefaultAllocatorFreeString(char* pString, void* pUserData);
		static Allocator	s_oDefaultAllocator;
	};

	// Quicker and use less memory than loading a Json with JsonValue, but read only
	class STTHM_API JsonDoc
	{
	public:
							JsonDoc(size_t iBlockSize = 4096);
							~JsonDoc();

		const JsonValue&	GetRoot() { return m_oRoot; }

		void				Clear();

		int					ReadString(const char* pJson, const char* pJsonEnd = NULL);
		int					ReadFile(const char* pFilename);

		size_t				MemoryUsage() const;
	protected:
		Allocator			m_oAllocator;
		JsonValue			m_oRoot;

		struct Block
		{
			size_t			m_iUsed;
			Block*			m_pPrevious;
		};

		size_t				m_iBlockSize;
		Block*				m_pLastBlock;

		static void*		Allocate(JsonDoc* pDoc, size_t iSize, size_t iAlign);

		static JsonValue*	CreateJsonValue(Allocator* pAllocator, void* pUserData);
		static void			DeleteJsonValue(JsonValue* pValue, void* pUserData);
		static char*		AllocString(size_t iSize, void* pUserData);
		static void			FreeString(char* pString, void* pUserData);
	};
}

#endif // __JSON_STTHM_H__

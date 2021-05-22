/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Array.h"
#include "LinkedList.h"

namespace iSTD
{

/*****************************************************************************\
Struct: IsHashTableTypeSupported
\*****************************************************************************/
template<typename T>
struct IsHashTableTypeSupported                     { enum { value = false }; };

template<>
struct IsHashTableTypeSupported<bool>               { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<char>               { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<unsigned char>      { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<int>                { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<unsigned int>       { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<>
struct IsHashTableTypeSupported<long>               { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<unsigned long>      { enum { value = true }; };
#endif

template<>
struct IsHashTableTypeSupported<float>              { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<INT64>              { enum { value = true }; };

template<>
struct IsHashTableTypeSupported<UINT64>             { enum { value = true }; };

template<typename T>
struct IsHashTableTypeSupported<T*>                 { enum { value = true }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define HashTableTemplateList       class KeyType, class ElementType, class CAllocatorType
#define CHashTableType              CHashTable<KeyType, ElementType, CAllocatorType>

/*****************************************************************************\

Class:
    CHashTable

Description:
    Template Hash Table

\*****************************************************************************/
template<HashTableTemplateList>
class CHashTable : public CObject<CAllocatorType>
{
public:

    // HashKey definition - converts KeyType to HashCode
    class CHashKey : public CObject<CAllocatorType>
    {
    public:

        CHashKey( void );
        CHashKey( const KeyType &keyValue );
        CHashKey( const KeyType &keyValue, const DWORD hashCode );
        CHashKey( const CHashKey &hashKey );
        virtual ~CHashKey( void );

        DWORD   GetHashCode( void ) const;
        const KeyType&  GetKeyValue( void ) const;

        bool        operator == ( const CHashKey &hashKey ) const;

    protected:
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable:4324 ) // structure was padded due to __declspec(align())
#endif
        ALIGN(16)   KeyType     m_KeyValue;
        ALIGN(16)   DWORD       m_HashCode;
#ifdef _MSC_VER
#pragma warning( pop )
#endif
    };

    // Constructor \ Destructor
    CHashTable( const DWORD size );
    virtual ~CHashTable( void );

    // Interfaces using KeyType
    bool    Add(
                const KeyType &key,
                const ElementType &element );

    bool    Get(
                const KeyType &key,
                ElementType &element ) const;

    bool    Remove(
                const KeyType &key,
                ElementType &element );

    bool    Remove(
                const ElementType &element );

    // Interfaces using HashKey
    bool    Add(
                const CHashKey &hashKey,
                const ElementType &element );

    bool    Get(
                const CHashKey &hashKey,
                ElementType &element ) const;

    bool    Remove(
                const CHashKey &hashKey,
                ElementType &element );

    // Other interfaces
    bool    IsEmpty( void ) const;
    DWORD   GetCount( void ) const;

    void    DebugPrint( void ) const;

    C_ASSERT( IsHashTableTypeSupported<ElementType>::value == true );

protected:

    // HashNode definition - hash table storage
    class CHashNode : public CObject<CAllocatorType>
    {
    public:

        CHashNode( const CHashKey &hashKey, const ElementType &element );
        virtual ~CHashNode( void );

        const CHashKey&     GetHashKey( void ) const;
        ElementType         GetElement( void ) const;

    protected:

        CHashKey    m_HashKey;
        ElementType m_Element;
    };

    // Internal data structures - Array of Linked-Lists
    typedef CLinkedList<CHashNode*, CAllocatorType>     CHashTableKeyList;
    typedef CArray<CHashTableKeyList*, CAllocatorType>  CHashTableArray;

    CHashTableKeyList*  CreateHashKeyList( void );
    void                DeleteHashKeyList( CHashTableKeyList* pHashKeyList );

    CHashNode*          CreateHashNode( const CHashKey &hashKey, const ElementType &element );
    void                DeleteHashNode( CHashNode* pHashNode );

    DWORD   GetHashSize( const DWORD size ) const;
    DWORD   GetHashIndex( const CHashKey &hashKey ) const;

    // Internal data
    CHashTableArray     m_HashArray;

    DWORD   m_Count;

#ifdef _DEBUG
    DWORD   m_AddCount;
    DWORD   m_RemoveCount;
    DWORD   m_CollisionCount;
#endif

public:

    // Iterator definition
    class CIterator
    {
    public:

        CIterator( void );
        CIterator( const CIterator &iterator );

        CIterator&  operator--( void );
        CIterator&  operator++( void );

        bool operator==( const CIterator &iterator ) const;
        bool operator!=( const CIterator &iterator ) const;

        CIterator&  operator=( const CIterator &iterator );
        ElementType operator*( void );

        const KeyType&  GetKeyValue( void );

        friend class CHashTable;

    protected:

        CHashTableArray*    m_Array;
        DWORD               m_ArrayIndex;

        typename CHashTableKeyList::CIterator m_Iterator;
    };

    // Begin \ End iterators of HashTable
    CIterator   Begin( void ) const;
    CIterator   End( void ) const;

    // Interfaces using Iterator
    bool    Add(
                const CHashKey &hashKey,
                const ElementType &element,
                CIterator &iterator );

    bool    Get(
                const CHashKey &hashKey,
                CIterator &iterator ) const;

    bool    Remove(
                CIterator iterator );
};

/*****************************************************************************\

Function:
    CHashTable Constructor

Description:
    Initializes internal data

Input:
    const DWORD size - max size of the hash table

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashTable( const DWORD size )
    : CObject<CAllocatorType>(),
      m_HashArray( GetHashSize( size ) )
{
    m_Count = 0;

#ifdef _DEBUG
    m_AddCount = 0;
    m_RemoveCount = 0;
    m_CollisionCount = 0;
#endif
}

/*****************************************************************************\

Function:
    CHashTable Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::~CHashTable( void )
{
    const DWORD size = m_HashArray.GetSize();
    for( DWORD i = 0; i < size; ++i )
    {
        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement(i);
        if( pHashKeyList )
        {
            DeleteHashKeyList( pHashKeyList );
        }
    }
}

/*****************************************************************************\

Function:
    CHashTable::Add

Description:
    Adds an element to the hash table

Input:
    const KeyType &key - the key to which the element is referenced
    const ElementType &element - the element to be added

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Add(
    const KeyType &key,
    const ElementType &element )
{
    CHashKey hashKey( key );
    return Add( hashKey, element );
}

/*****************************************************************************\

Function:
    CHashTable::Get

Description:
    Gets the element in the hash table referenced by the key

Input:
    const KeyType &key - the key of the element to get

Output:
    ElementType &element - the element
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Get(
    const KeyType &key,
    ElementType &element ) const
{
    CHashKey hashKey( key );
    return Get( hashKey, element );
}

/*****************************************************************************\

Function:
    CHashTable::Remove

Description:
    Removes an element from the hash table

Input:
    const KeyType &key - the key of the element to remove

Output:
    ElementType &element - the element
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Remove(
    const KeyType &key,
    ElementType &element )
{
    CHashKey hashKey( key );
    return Remove( hashKey, element );
}

/*****************************************************************************\

Function:
    CHashTable::Remove

Description:
    Removes an element from the hash table

Input:
    ElementType &element - the element to remove

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Remove(
    const ElementType &element )
{
    // Walk the entire array to find the element
    const DWORD size = m_HashArray.GetSize();
    for( DWORD i = 0; i < size; ++i )
    {
        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement(i);
        if( pHashKeyList )
        {
            // Search the list for the node with the same element
            typename CHashTableKeyList::CIterator iterator = pHashKeyList->Begin();

            while( iterator != pHashKeyList->End() )
            {
                CHashNode* pHashNode = (*iterator);

                if( pHashNode->GetElement() == element )
                {
                    if( pHashKeyList->Remove( iterator ) )
                    {
                        DeleteHashNode( pHashNode );

                        if( pHashKeyList->IsEmpty() )
                        {
                            m_HashArray.SetElement( i, NULL );
                            DeleteHashKeyList( pHashKeyList );
                        }

                        ASSERT( m_Count > 0 );
                        --m_Count;

#ifdef _DEBUG
                        ++m_RemoveCount;
#endif
                        return true;
                    }
                    else
                    {
                        ASSERT(0);
                        return false;
                    }
                }

                ++iterator;
            }
        }
    }

    // Element was not found
    ASSERT(0);
    return false;
}

/*****************************************************************************\

Function:
    CHashTable::Add

Description:
    Adds an element to the hash table

Input:
    const CHashKey &hashKey - the key to which the element is referenced
    const ElementType &element - the element to be added

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Add(
    const CHashKey &hashKey,
    const ElementType &element )
{
    if( m_HashArray.GetSize() == 0 )
    {
        return false;
    }
    const DWORD index = GetHashIndex( hashKey );
    ASSERT( index < m_HashArray.GetSize() );

    CHashTableKeyList* pHashKeyList = m_HashArray.GetElement( index );

    // Use the hash code to find a linked list containing all keys with the
    // same hash code
    if( !pHashKeyList )
    {
        // if the list doesn't exist then create a new one
        pHashKeyList = CreateHashKeyList();

        if( pHashKeyList )
        {
            // Add the new hash key list to hash array
            if( !m_HashArray.SetElement( index, pHashKeyList ) )
            {
                ASSERT(0);
                SafeDelete( pHashKeyList );
                return false;
            }
        }
        else
        {
            ASSERT(0);
            return false;
        }
    }

    // Create the node to add to the list
    CHashNode* pHashNode = CreateHashNode( hashKey, element );

    if( pHashNode )
    {
        // Add the node to the hash key list
        if( pHashKeyList->Add( pHashNode ) )
        {
            ++m_Count;

#ifdef _DEBUG
            ++m_AddCount;

            if( pHashKeyList->GetCount() > 1 )
            {
                ++m_CollisionCount;
            }
#endif
            return true;
        }
        else
        {
            DeleteHashNode( pHashNode );

            ASSERT(0);
            return false;
        }
    }
    else
    {
        ASSERT(0);
        return false;
    }
}

/*****************************************************************************\

Function:
    CHashTable::Get

Description:
    Gets the element in the hash table referenced by the key

Input:
    const CHashKey &hashKey - the key of the element to get

Output:
    ElementType &element - the element
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Get(
    const CHashKey &hashKey,
    ElementType &element ) const
{
    if( m_HashArray.GetSize() != 0 )
    {
        const DWORD index = GetHashIndex( hashKey );
        ASSERT( index < m_HashArray.GetSize() );

        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement( index );

        if( pHashKeyList )
        {
            // Search the list for the node with the same key
            typename CHashTableKeyList::CIterator iterator = pHashKeyList->Begin();

            while( iterator != pHashKeyList->End() )
            {
                CHashNode* pHashNode = (*iterator);

                if( pHashNode->GetHashKey() == hashKey )
                {
                    element = pHashNode->GetElement();
                    return true;
                }

                ++iterator;
            }
        }
    }
    return false;
}

/*****************************************************************************\

Function:
    CHashTable::Remove

Description:
    Removes an element from the hash table

Input:
    const CHashKey &hashKey - the key of the element to remove

Output:
    ElementType &element - the element
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Remove(
    const CHashKey &hashKey,
    ElementType &element )
{
    if( m_HashArray.GetSize() != 0 )
    {
        const DWORD index = GetHashIndex( hashKey );
        ASSERT( index < m_HashArray.GetSize() );

        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement( index );

        if( pHashKeyList )
        {
            // Search the list for the node with the same key
            typename CHashTableKeyList::CIterator iterator = pHashKeyList->Begin();

            while( iterator != pHashKeyList->End() )
            {
                CHashNode* pHashNode = (*iterator);

                if( pHashNode->GetHashKey() == hashKey )
                {
                    element = pHashNode->GetElement();

                    if( pHashKeyList->Remove( iterator ) )
                    {
                        DeleteHashNode( pHashNode );

                        if( pHashKeyList->IsEmpty() )
                        {
                            m_HashArray.SetElement( index, NULL );
                            DeleteHashKeyList( pHashKeyList );
                        }

                        ASSERT( m_Count > 0 );
                        --m_Count;

    #ifdef _DEBUG
                        ++m_RemoveCount;
    #endif
                        return true;
                    }
                    else
                    {
                        ASSERT(0);
                        return false;
                    }
                }

                ++iterator;
            }
        }
    }

    ASSERT(0);
    return false;
}

/*****************************************************************************\

Function:
    CHashTable::IsEmpty

Description:
    Determines if the hash table is empty

Input:
    void

Output:
    bool

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::IsEmpty( void ) const
{
    return ( m_Count == 0 );
}

/*****************************************************************************\

Function:
    CHashTable::GetCount

Description:
    Returns number of elements.

Input:
    void

Output:
    DWORD

\*****************************************************************************/
template<HashTableTemplateList>
DWORD CHashTableType::GetCount( void ) const
{
    return m_Count;
}

/*****************************************************************************\

Function:
    CHashTable::DebugPrint

Description:
    Prints the hashtable to std output for debug only

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
void CHashTableType::DebugPrint( void ) const
{
#ifdef _DEBUG
    DPF( GFXDBG_STDLIB, "%s\n", __FUNCTION__ );
    DPF( GFXDBG_STDLIB, "\tAddress = %p\n", this );

    DPF( GFXDBG_STDLIB, "\tCount = %u\n", m_Count );
    DPF( GFXDBG_STDLIB, "\tAdd Count = %u\n", m_AddCount );
    DPF( GFXDBG_STDLIB, "\tRemove Count = %u\n", m_RemoveCount );
    DPF( GFXDBG_STDLIB, "\tCollision Count = %u\n", m_CollisionCount );

    for( DWORD i = 0; i < m_HashArray.GetSize(); i++ )
    {
        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement(i);

        if( pHashKeyList )
        {
            DPF( GFXDBG_STDLIB, "\tHashArray.Element[%u].Count = %u\n",
                i,
                pHashKeyList->GetCount() );
        }
        else
        {
            DPF( GFXDBG_STDLIB, "\tHashArray.Element[%u].Count = 0\n",
                i );
        }
    }
#endif
}

/*****************************************************************************\

Function:
    CHashTable::CreateHashKeyList

Description:
    Creates a hashtable key list

Input:
    none

Output:
    CHashTableKeyList*

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CHashTableKeyList* CHashTableType::CreateHashKeyList( void )
{
    CHashTableKeyList* pHashKeyList = new CHashTableKeyList();
    ASSERT( pHashKeyList );
    return pHashKeyList;
}

/*****************************************************************************\

Function:
    CHashTable::DeleteHashKeyList

Description:
    Deletes a hashtable key list

Input:
    CHashTableKeyList* pHashKeyList

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
void CHashTableType::DeleteHashKeyList(
    CHashTableKeyList* pHashKeyList )
{
    if( pHashKeyList )
    {
        while( !pHashKeyList->IsEmpty() )
        {
            typename CHashTableKeyList::CIterator iterator = pHashKeyList->Begin();

            CHashNode* pHashNode = (*iterator);
            DeleteHashNode( pHashNode );

            pHashKeyList->Remove( iterator );
        }

        iSTD::SafeDelete( pHashKeyList );
    }
}

/*****************************************************************************\

Function:
    CHashTable::CreateHashNode

Description:
    Creates a hashtable list node

Input:
    const CHashKey &hashKey
    const ElementType &element

Output:
    CHashNode*

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CHashNode* CHashTableType::CreateHashNode(
    const CHashKey &hashKey,
    const ElementType &element )
{
    CHashNode* pHashNode = new CHashNode( hashKey, element );
    ASSERT( pHashNode );
    return pHashNode;
}

/*****************************************************************************\

Function:
    CHashTable::DeleteHashNode

Description:
    Deletes a hashtable list node

Input:
    CHashNode* pHashNode

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
void CHashTableType::DeleteHashNode( CHashNode* pHashNode )
{
    if( pHashNode )
    {
        iSTD::SafeDelete( pHashNode );
    }
}

/*****************************************************************************\

Function:
    CHashTable::GetHashSize

Description:
    Determines the size of the hash array

Input:
    const DWORD size - max number of elements in the hash table

Output:
    DWORD

\*****************************************************************************/
template<HashTableTemplateList>
DWORD CHashTableType::GetHashSize( const DWORD size ) const
{
    // the size of the hash table array is a large prime number near but
    // smaller than the maximum size of the cache but not near a power of two
    if( size >= 6151 )
    {
        return 6151;
    }
    else if( size >= 3079 )
    {
        return 3079;
    }
    else if( size >= 1543 )
    {
        return 1543;
    }
    else if( size >= 769 )
    {
        return 769;
    }
    else if( size >= 389 )
    {
        return 389;
    }
    else if( size >= 193 )
    {
        return 193;
    }
    else if( size >= 97 )
    {
        return 97;
    }
    else
    {
        return 53;
    }
}

/*****************************************************************************\

Function:
    CHashTable::GetHashIndex

Description:
    Determines the index in the hash array for the hash key

Input:
    const CHashKey &hashKey

Output:
    DWORD

\*****************************************************************************/
template<HashTableTemplateList>
DWORD CHashTableType::GetHashIndex( const CHashKey &hashKey ) const
{
    return ( hashKey.GetHashCode() % m_HashArray.GetSize() );
}

/*****************************************************************************\

Function:
    CHashKey constructor

Description:
    Initializes internal data

Input:
    void

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashKey::CHashKey( void )
    : CObject<CAllocatorType>()
{
    SafeMemSet( &m_KeyValue, 0, sizeof(KeyType) );
    m_HashCode = 0;
}

/*****************************************************************************\

Function:
    CHashKey constructor

Description:
    Initializes internal data

Input:
    const KeyType &keyValue

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashKey::CHashKey( const KeyType &keyValue )
    : CObject<CAllocatorType>()
{
#if defined(_WIN32) && defined(_MSC_VER)
    ASSERT( IsAligned( &m_KeyValue, sizeof(DQWORD) ) );
    ASSERT( IsAligned( &m_HashCode, sizeof(DQWORD) ) );

    m_KeyValue = keyValue;
    ASSERT( m_KeyValue == keyValue );

    if( sizeof(KeyType) < sizeof(DQWORD) )
    {
        const DWORD* pKeyValue = (const DWORD*)&m_KeyValue;
        m_HashCode = *pKeyValue++;

        const DWORD count = (DWORD)( sizeof(KeyType) / sizeof(DWORD) );
        for( DWORD i = 1; i < count; ++i )
        {
            const DWORD data = *pKeyValue++;
            m_HashCode = ( m_HashCode << 1 ) ^ data;
        }
    }
    else
    {
        const DWORD szAlignedKeyValue = (DWORD)(
            ( ( ( sizeof(KeyType) - 1 ) / sizeof(DQWORD) ) + 1 )  * sizeof(DQWORD) );

        const DWORD szPad = szAlignedKeyValue - (DWORD)sizeof(KeyType);

        // Initialize structure padding due to packing alignment
        if( szPad )
        {
            SafeMemSet( (BYTE*)&m_KeyValue + sizeof(KeyType), 0, szPad );
        }

        // Generate hash code
        const __m128i* pKeyValue = (const __m128i*)&m_KeyValue;
        __m128i hash = _mm_load_si128( pKeyValue++ );

        const DWORD count = szAlignedKeyValue / sizeof(DQWORD);
        for( DWORD i = 1; i < count; ++i )
        {
            const __m128i data = _mm_load_si128( pKeyValue++ );
            hash = _mm_xor_si128( _mm_slli_si128( hash, 1 ), data );
        }

        // Combine four 32-bit integers into single 32-bit integer
        hash = _mm_xor_si128(
            _mm_unpackhi_epi32( hash, hash ),
            _mm_unpacklo_epi32( hash, hash ) );
        hash = _mm_xor_si128(
            _mm_shuffle_epi32( hash, 0x2 ),
            hash );
        m_HashCode = _mm_cvtsi128_si32( hash );
    }
#else // _MSC_VER
    ASSERT( IsAligned( &m_KeyValue, sizeof(DWORD) ) );
    ASSERT( IsAligned( &m_HashCode, sizeof(DWORD) ) );

    m_KeyValue = keyValue;
    ASSERT( m_KeyValue == keyValue );

    const DWORD* pKeyValue = (const DWORD*)&m_KeyValue;
    m_HashCode = *pKeyValue++;

    const DWORD count = (DWORD)( sizeof(KeyType) / sizeof(DWORD) );
    for( DWORD i = 1; i < count; ++i )
    {
        const DWORD data = *pKeyValue++;
        m_HashCode = ( m_HashCode << 1 ) ^ data;
    }
#endif // _MSC_VER
}


/*****************************************************************************\

Function:
    CHashKey constructor

Description:
    Initializes internal data

Input:
    const KeyType &keyValue
    const DWORD hashCode

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashKey::CHashKey( const KeyType &keyValue, const DWORD hashCode )
    : CObject<CAllocatorType>()
{
    m_KeyValue = keyValue;
    ASSERT( m_KeyValue == keyValue );
    m_HashCode = hashCode;
}

/*****************************************************************************\

Function:
    CHashKey copy constructor

Description:
    Initializes internal data

Input:
    const CHashKey &hashKey

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashKey::CHashKey( const CHashKey &hashKey )
{
    m_KeyValue = hashKey.m_KeyValue;
    m_HashCode = hashKey.m_HashCode;
}

/*****************************************************************************\

Function:
    CHashKey destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashKey::~CHashKey( void )
{
}

/*****************************************************************************\

Function:
    CHashKey::GetHashCode

Description:
    Returns the 32-bit hash code

Input:
    none

Output:
    DWORD

\*****************************************************************************/
template<HashTableTemplateList>
DWORD CHashTableType::CHashKey::GetHashCode( void ) const
{
    return m_HashCode;
}

/*****************************************************************************\

Function:
    CHashKey::GetKeyValue

Description:
    Returns the key value

Input:
    none

Output:
    const KeyType&

\*****************************************************************************/
template<HashTableTemplateList>
const KeyType& CHashTableType::CHashKey::GetKeyValue( void ) const
{
    return m_KeyValue;
}

/*****************************************************************************\

Function:
    CHashKey::operator ==

Description:
    Determines if the hash keys are equal

Input:
    const CHashKey &hashKey

Output:
    bool - true or false

\*****************************************************************************/
template<HashTableTemplateList>
inline bool CHashTableType::CHashKey::operator == ( const CHashKey &hashKey ) const
{
    if( m_HashCode == hashKey.m_HashCode )
    {
        return ( m_KeyValue == hashKey.m_KeyValue );
    }
    else
    {
        return false;
    }
}

/*****************************************************************************\

Function:
    CHashNode constructor

Description:
    Initializes internal data

Input:
    const CHashKey &hashKey
    const ElementType &element

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashNode::CHashNode(
    const CHashKey &hashKey,
    const ElementType &element )
    : CObject<CAllocatorType>(),
      m_HashKey( hashKey )
{
    m_Element = element;
}

/*****************************************************************************\

Function:
    CHashNode destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CHashNode::~CHashNode( void )
{
}

/*****************************************************************************\

Function:
    CHashKey::GetHashKey

Description:
    Returns the hash key

Input:
    none

Output:
    const CHashKey&

\*****************************************************************************/
template<HashTableTemplateList>
const typename CHashTableType::CHashKey& CHashTableType::CHashNode::GetHashKey( void ) const
{
    return m_HashKey;
}

/*****************************************************************************\

Function:
    CHashKey::GetElement

Description:
    Returns the element

Input:
    none

Output:
    ElementType

\*****************************************************************************/
template<HashTableTemplateList>
ElementType CHashTableType::CHashNode::GetElement( void ) const
{
    return m_Element;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator constructor

Description:
    Default constructor

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CIterator::CIterator( void )
    : m_Iterator()
{
    m_Array = NULL;
    m_ArrayIndex = 0;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator copy constructor

Description:
    Copy constructor

Input:
    none

Output:
    none

\*****************************************************************************/
template<HashTableTemplateList>
CHashTableType::CIterator::CIterator( const CIterator &iterator )
    : m_Iterator( iterator.m_Iterator )
{
    m_Array = iterator.m_Array;
    m_ArrayIndex = iterator.m_ArrayIndex;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator operator--

Description:
    Iterates backwards through the hash table via predecrement

Input:
    none

Output:
    CHashTable::CIterator& - iterator of previous entry

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CIterator& CHashTableType::CIterator::operator--( void )
{
    --m_Iterator;

    CHashTableKeyList* pHashKeyList = m_Array->GetElement( m_ArrayIndex );
    ASSERT( pHashKeyList );

    while( ( m_Iterator == pHashKeyList->End() ) && ( m_ArrayIndex != 0 ) )
    {
        CHashTableKeyList* pPrevHashKeyList = m_Array->GetElement( --m_ArrayIndex );
        if( pPrevHashKeyList )
        {
            pHashKeyList = pPrevHashKeyList;
            m_Iterator = pHashKeyList->End();
            --m_Iterator;
        }
    }

    return *this;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator operator++

Description:
    Iterates forwards through the hash table via preincrement

Input:
    none

Output:
    CHashTable::CIterator& - iterator of next entry

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CIterator& CHashTableType::CIterator::operator++( void )
{
    ++m_Iterator;

    CHashTableKeyList* pHashKeyList = m_Array->GetElement( m_ArrayIndex );
    ASSERT( pHashKeyList );

    const DWORD maxIndex = m_Array->GetSize() - 1;
    while( ( m_Iterator == pHashKeyList->End() ) && ( m_ArrayIndex != maxIndex ) )
    {
        CHashTableKeyList* pNextHashKeyList = m_Array->GetElement( ++m_ArrayIndex );
        if( pNextHashKeyList )
        {
            pHashKeyList = pNextHashKeyList;
            m_Iterator = pHashKeyList->Begin();
        }
    }

    return *this;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator::operator==

Description:
    Determines if the iterators are equal

Input:
    const CIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to the same
           hash table node.

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::CIterator::operator==(
    const CIterator& iterator ) const
{
    return m_Iterator == iterator.m_Iterator;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator::operator!=

Description:
    Determines if the iterators are not equal

Input:
    const CIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to different
           hash table nodes.

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::CIterator::operator!=(
    const CIterator& iterator ) const
{
    return m_Iterator != iterator.m_Iterator;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator::operator=

Description:
    Sets the iterators equal

Input:
    const CIterator& - reference to the iterator to copy from.

Output:
    const CIterator & - reference to self

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CIterator& CHashTableType::CIterator::operator=(
    const CIterator &iterator )
{
    m_Array = iterator.m_Array;
    m_ArrayIndex = iterator.m_ArrayIndex;
    m_Iterator = iterator.m_Iterator;
    return *this;
}

/*****************************************************************************\

Function:
    CHashTable::CIterator::operator*

Description:
    Returns a reference to the element that this iterator points to

Input:
    none

Output:
    ElementType

\*****************************************************************************/
template<HashTableTemplateList>
ElementType CHashTableType::CIterator::operator*( void )
{
    return (*m_Iterator)->GetElement();
}

/*****************************************************************************\

Function:
    CHashTable::CIterator::GetKey()

Description:
    Returns key value.

Input:
    none

Output:
    KeyType

\*****************************************************************************/
template<HashTableTemplateList>
const KeyType& CHashTableType::CIterator::GetKeyValue( void )
{
    return (*m_Iterator)->GetHashKey().GetKeyValue();
}

/*****************************************************************************\

Function:
    CHashTable::Begin

Description:
    Returns an iterator to the first node in the hash table.

Input:
    none

Output:
    CHashTable::CIterator

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CIterator CHashTableType::Begin( void ) const
{
    CIterator iterator;

    const DWORD size = m_HashArray.GetSize();
    for( DWORD i = 0; i < size; ++i )
    {
        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement(i);
        if( pHashKeyList )
        {
            iterator.m_Iterator = pHashKeyList->Begin();
            iterator.m_Array = const_cast<CHashTableArray*>(&m_HashArray);
            iterator.m_ArrayIndex = i;
            return iterator;
        }
    }

    return iterator;
}

/*****************************************************************************\

Function:
    CHashTable::End

Description:
    Returns an iterator to the last (dummy) node in the hash table

Input:
    none

Output:
    CHashTable::CIterator

\*****************************************************************************/
template<HashTableTemplateList>
typename CHashTableType::CIterator CHashTableType::End( void ) const
{
    CIterator iterator;

    const DWORD size = m_HashArray.GetSize();
    for( int i = (int)size-1; i >= 0; --i )
    {
        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement((DWORD)i);
        if( pHashKeyList )
        {
            iterator.m_Iterator = pHashKeyList->End();
            iterator.m_Array = const_cast<CHashTableArray*>(&m_HashArray);
            iterator.m_ArrayIndex = (DWORD)i;
            return iterator;
        }
    }

    return iterator;
}

/*****************************************************************************\

Function:
    CHashTable::Add

Description:
    Adds the element to the hash table and returns its iterator

Input:
    const CHashKey &hashKey
    const ElementType &element

Output:
    CIterator &iterator
    bool - true if element was added

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Add(
    const CHashKey &hashKey,
    const ElementType &element,
    CIterator &iterator )
{
    if( m_HashArray.GetSize() == 0 )
    {
        return false;
    }
    const DWORD index = GetHashIndex( hashKey );
    ASSERT( index < m_HashArray.GetSize() );

    CHashTableKeyList* pHashKeyList = m_HashArray.GetElement( index );

    // Use the hash code to find a linked list containing all keys with the
    // same hash code
    if( !pHashKeyList )
    {
        // if the list doesn't exist then create a new one
        pHashKeyList = CreateHashKeyList();

        if( pHashKeyList )
        {
            // Add the new hash key list to hash array
            if( !m_HashArray.SetElement( index, pHashKeyList ) )
            {
                ASSERT(0);
                SafeDelete( pHashKeyList );
                return false;
            }
        }
        else
        {
            ASSERT(0);
            return false;
        }
    }

    // Create the node to add to the list
    CHashNode* pHashNode = CreateHashNode( hashKey, element );

    if( pHashNode )
    {
        // Add the node to the hash key list
        if( pHashKeyList->Add( pHashNode ) )
        {
            ++m_Count;

#ifdef _DEBUG
            ++m_AddCount;

            if( pHashKeyList->GetCount() > 1 )
            {
                ++m_CollisionCount;
            }
#endif
            iterator.m_Array = &m_HashArray;
            iterator.m_ArrayIndex = index;
            iterator.m_Iterator = pHashKeyList->Begin();
            return true;
        }
        else
        {
            DeleteHashNode( pHashNode );

            ASSERT(0);
            return false;
        }
    }
    else
    {
        ASSERT(0);
        return false;
    }
}

/*****************************************************************************\

Function:
    CHashTable::Get

Description:
    Gets the iterator of the element from the hash table

Input:
    const CHashKey &hashKey

Output:
    CIterator &iterator
    bool - true if element was removed

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Get(
    const CHashKey &hashKey,
    CIterator &iterator ) const
{
    if( m_HashArray.GetSize() != 0 )
    {
        const DWORD index = GetHashIndex( hashKey );
        ASSERT( index < m_HashArray.GetSize() );

        CHashTableKeyList* pHashKeyList = m_HashArray.GetElement( index );

        if( pHashKeyList )
        {
            // Search the list for the node with the same key
            typename CHashTableKeyList::CIterator list_iterator = pHashKeyList->Begin();

            while( list_iterator != pHashKeyList->End() )
            {
                CHashNode* pHashNode = (*list_iterator);

                if( pHashNode->GetHashKey() == hashKey )
                {
                    iterator.m_Array = const_cast<CHashTableArray*>(&m_HashArray);
                    iterator.m_ArrayIndex = index;
                    iterator.m_Iterator = list_iterator;
                    return true;
                }

                ++list_iterator;
            }
        }
    }
    return false;
}

/*****************************************************************************\

Function:
    CHashTable::Remove

Description:
    Removes element from the hash table via iterator

Input:
    CIterator iterator

Output:
    bool - true if element was removed

\*****************************************************************************/
template<HashTableTemplateList>
bool CHashTableType::Remove( CIterator iterator )
{
    CHashTableKeyList* pHashKeyList = m_HashArray.
        GetElement( iterator.m_ArrayIndex );

    if( pHashKeyList )
    {
        CHashNode* pHashNode = *(iterator.m_Iterator);

        pHashKeyList->Remove( iterator.m_Iterator );

        DeleteHashNode( pHashNode );

        if( pHashKeyList->IsEmpty() )
        {
            m_HashArray.SetElement( iterator.m_ArrayIndex, NULL );
            DeleteHashKeyList( pHashKeyList );
        }

        ASSERT( m_Count > 0 );
        --m_Count;

#ifdef _DEBUG
        ++m_RemoveCount;
#endif
        return true;
    }

    return false;
}

} // iSTD

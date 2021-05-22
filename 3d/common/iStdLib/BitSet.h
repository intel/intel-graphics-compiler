/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "utility.h"
#include "MemCopy.h"
#include "Threading.h"

namespace iSTD
{

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define BitSetTemplateList  class CAllocatorType
#define CBitSetType         CBitSet<CAllocatorType>

/*****************************************************************************\
Struct: 
    TPtrSize
Description: 
    Defines type that is size of a pointer. It also provides a set of static 
    functions for managing on bits, depending on pointer size.
                
\*****************************************************************************/
template <int T> struct TPtrSize
{
};
// Specialization for 32bit pointers:
template <> struct TPtrSize<4>
{
    typedef unsigned int type;

    static inline type Bit( const DWORD index )
    {
        return BIT( index );
    }

    static inline DWORD Bsf( const type mask )
    {
        return iSTD::bsf( mask );
    }

    static inline DWORD Bsr( const type mask )
    {
        return iSTD::bsr( mask );
    }

    static inline DWORD Count( const type mask )
    {
        return iSTD::BitCount( mask );
    }
};
// Specialization for 64bit pointers:
template <> struct TPtrSize<8>
{
    typedef unsigned long long type;

    static inline type Bit( const DWORD index )
    {
        return QWBIT( index );
    }

    static inline DWORD Bsf( const type mask )
    {
#if defined( _WIN64 ) || defined( __x86_64__ )
        return iSTD::bsf64( mask );
#else
        // Should never compile this code - added to get rid of compilation 
        // warnings on GCC.
        ASSERT( 0 );
        return 0;
#endif
    }

    static inline DWORD Bsr( const type mask )
    {
#if defined( _WIN64 ) || defined( __x86_64__ )
        return iSTD::bsr64( mask );
#else
        // Should never compile this code - added to get rid of compilation 
        // warnings on GCC.
        ASSERT( 0 );
        return 0;
#endif
    }

    static inline DWORD Count( const type mask )
    {
        return iSTD::BitCount64( mask );
    }
};

/*****************************************************************************\

Class:
    CBitSet

Description:
    Implements a dynamic bit set. For sets smaller than size of pointer, bits 
    are stored in a pointer to the array, to prevent dynamic allocations.

\*****************************************************************************/
template<BitSetTemplateList>
class CBitSet : public CObject<CAllocatorType>
{
private:
    static const DWORD BITS_PER_BYTE = 8;
    typedef DWORD   BITSET_ARRAY_TYPE;
    static const DWORD cBitsPerArrayElement = 
        sizeof( BITSET_ARRAY_TYPE ) * BITS_PER_BYTE; 

    static const DWORD cBitsInPtr = sizeof(BITSET_ARRAY_TYPE*) * 8;

    typedef TPtrSize<sizeof(void*)> CPtrSize;
    typedef CPtrSize::type bitptr_t;

public:

    CBitSet( void );
    CBitSet( DWORD size );
    CBitSet( const CBitSetType& other );

    virtual ~CBitSet( void );

    void    Resize( DWORD size );

    void    Clear( void );
    void    SetAll( void );
    void    Invert( void );

    bool    IsEmpty() const;
    bool    IsEmpty( DWORD start, DWORD length ) const;

    bool    IsSet( DWORD index ) const;
    bool    Intersects( const CBitSetType& other ) const;

    DWORD   GetNextMember( DWORD start ) const;

    void    Set( DWORD index );
    void    Set( const CBitSetType& other );
    void    UnSet( DWORD index );
    void    UnSet( const CBitSetType& other );

    DWORD   GetSize( void ) const;
    DWORD   BitCount( void ) const;
    DWORD   BitCount( DWORD limit ) const;
    long    Min( void ) const;
    long    Max( void ) const;

    template<class T>
    T   ConvertTo() const;

    bool    operator==( const CBitSetType& other ) const;
    bool    operator!=( const CBitSetType& other ) const;

    CBitSetType& operator=  ( const CBitSetType& other );
    CBitSetType& operator|= ( const CBitSetType& other );
    CBitSetType& operator&= ( const CBitSetType& other );

protected:

    // Depending on a set size, bits can be stored in dynamically allocated 
    // memory, or in a pointer.
    union 
    {
        BITSET_ARRAY_TYPE*  m_BitSetArray;
        bitptr_t            m_PtrBits;
    };
    DWORD   m_Size;

    void    Create( DWORD size );
    void    Copy( const CBitSetType& other );
    void    Delete( void );

    bool        StoredInPtr( void ) const;
    bitptr_t    GetActivePtrMask( void ) const;

    BITSET_ARRAY_TYPE*          GetArrayPointer( void );
    const BITSET_ARRAY_TYPE*    GetArrayPointer( void ) const;

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
    CBitSet Constructor

Description:
    Initializes the BitSet

Input:
    none

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType::CBitSet( void )
    : CObject<CAllocatorType>()
{
    m_BitSetArray = NULL;
    m_Size = 0;

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet Constructor

Description:
    Initializes the BitSet

Input:
    DWORD size - initial size of the BitSet

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType::CBitSet( DWORD size )
    : CObject<CAllocatorType>()
{
    m_BitSetArray = NULL;
    m_Size = 0;

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );

    Create( size );
}

/*****************************************************************************\

Function:
    CBitSet Copy Constructor

Description:
    Initializes the BitSet

Input:
    const CBitSetType& other - other bitset to copy

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType::CBitSet( const CBitSetType& other )
    : CObject<CAllocatorType>()
{
    m_BitSetArray = NULL;
    m_Size = 0;

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );

    Copy( other );
}

/*****************************************************************************\

Function:
    CBitSet Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType::~CBitSet( void )
{
    Delete();

    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::Resize

Description:
    Resizes the bitset

Input:
    DWORD size - new size of the BitSet

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Resize( DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    Create( size );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::Clear

Description:
    Unsets all bits in the bitset

Input:
    none

Output:
    none

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Clear( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( StoredInPtr() )
    {
        m_PtrBits = static_cast<bitptr_t>( 0 );
    }
    else
    {
        const DWORD cArraySizeInBytes = 
            ( m_Size + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;

        SafeMemSet( m_BitSetArray, 0, cArraySizeInBytes );
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::SetAll

Description:
    Sets all the bits in this bitset, from bit zero to bit "size".  Note that
    any "extra" bits (bits that are part of the array but that are less than
    "size" are not set and remain unset.

Input:
    void

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::SetAll( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( StoredInPtr() )
    {
        m_PtrBits = GetActivePtrMask();
    }
    else
    {
        ASSERT( m_BitSetArray );

        const DWORD cArraySize = 
            ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        const BITSET_ARRAY_TYPE cExtraBits = 
            m_Size % cBitsPerArrayElement;
        const BITSET_ARRAY_TYPE cExtraBitMask = 
            cExtraBits ? ( ( 1 << cExtraBits ) - 1 ) : ( ~0 );

        DWORD   index;

        for( index = 0; index < cArraySize - 1; index++ )
        {
            m_BitSetArray[index] = ~((BITSET_ARRAY_TYPE)0);
        }
        if( index < cArraySize )
        {
            m_BitSetArray[index] = ~((BITSET_ARRAY_TYPE)0) & cExtraBitMask;
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::Invert

Description:
    Computes the inverse of this bitset.  Note that any "extra" bits (bits 
    that are part of the array but that are less than "size" are not inverted 
    and remain un-set.

Input:
    void

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Invert( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( StoredInPtr() )
    {
        m_PtrBits = (~m_PtrBits) & GetActivePtrMask();
    }
    else
    {
        ASSERT( m_BitSetArray );

        const DWORD cArraySize = 
            ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        const BITSET_ARRAY_TYPE cExtraBits = 
            m_Size % cBitsPerArrayElement;
        const BITSET_ARRAY_TYPE cExtraBitMask = 
            cExtraBits ? ( ( 1 << cExtraBits ) - 1 ) : ( ~0 );

        DWORD   index;

        for( index = 0; index < cArraySize - 1; index++ )
        {
            m_BitSetArray[index] = ~m_BitSetArray[index];
        }
        if( index < cArraySize )
        {
            m_BitSetArray[index] = ~m_BitSetArray[index] & cExtraBitMask;
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::IsEmpty

Description:
    Determines if any bits are on in the bit set.

Input:
    none

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::IsEmpty( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool isEmpty = true;

    if( StoredInPtr() )
    {
        isEmpty = ( m_PtrBits == static_cast<bitptr_t>( 0 ) );
    }
    else
    {
        DWORD index = ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        while( isEmpty && index-- )
        {
            isEmpty = (m_BitSetArray[ index ] == 0 );
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return isEmpty;
}

/*****************************************************************************\

Function:
    CBitSet::IsEmpty

Description:
    Determines if any bits are set in the bit set in the specified range.

Input:
    DWORD start - Start of the range to check, inclusive.
    DWORD length - Length of the range to check.

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::IsEmpty( DWORD start, DWORD length ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool isEmpty = true;

    ASSERT( start < m_Size );
    ASSERT( length != 0 );
    ASSERT( start + length <= m_Size );

    if( StoredInPtr() )
    {
        const DWORD end = start + length;

        // Create a bit mask for this range:
        bitptr_t mask = ~( CPtrSize::Bit( start ) - 1 );
        if( end < cBitsInPtr )
        {
            mask &= CPtrSize::Bit( end ) - 1;
        }

        isEmpty = ( ( mask & m_PtrBits ) == static_cast<bitptr_t>( 0 ) );
    }
    else
    {
        const DWORD end = start + length;
    
        const DWORD startArrayIndex = start / cBitsPerArrayElement;
        const DWORD endArrayIndex = ( end - 1 ) / cBitsPerArrayElement;

        const DWORD startBit = start % cBitsPerArrayElement;
        const DWORD endBit = end % cBitsPerArrayElement;

        const BITSET_ARRAY_TYPE startMask = ~( ( 1 << startBit ) - 1 );
        const BITSET_ARRAY_TYPE endMask = endBit ? ( ( 1 << endBit ) - 1 ) : ~0;

        DWORD   arrayIndex = startArrayIndex;

        BITSET_ARRAY_TYPE   data = m_BitSetArray[ arrayIndex ];

        data &= startMask;

        if( startArrayIndex == endArrayIndex )
        {
            data &= endMask;

            isEmpty = ( data == 0 );
        }
        else
        {
            isEmpty = ( data == 0 );

            if( isEmpty )
            {
                for( arrayIndex = arrayIndex + 1; 
                     arrayIndex < endArrayIndex && isEmpty; 
                     arrayIndex++ )
                {
                    data = m_BitSetArray[ arrayIndex ];

                    isEmpty = ( data == 0 );
                }
            }

            if( isEmpty )
            {
                data = m_BitSetArray[ endArrayIndex ];
                data &= endMask;

                isEmpty = ( data == 0 );
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return isEmpty;
}

/*****************************************************************************\

Function:
    CBitSet::IsSet

Description:
    Returns true if the bit at the specified index is set, false otherwise.

Input:
    DWORD index - index of the bit to check

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::IsSet( DWORD index ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool    isSet = false;

    if( index < m_Size )
    {
        if( StoredInPtr() )
        {
            isSet = ( ( m_PtrBits & CPtrSize::Bit( index ) ) != static_cast<bitptr_t>( 0 ) );
        }
        else
        {
            DWORD   arrayIndex = index / cBitsPerArrayElement;
            DWORD   bitIndex = index % cBitsPerArrayElement;

            isSet = ( ( m_BitSetArray[arrayIndex] & BIT(bitIndex) ) != 0 );
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return isSet;
}

/*****************************************************************************\

Function:
    CBitSet::Intersects

Description:
    Returns true if any bits are on in both this bit set and the passed-in
    bit set.  This is a shortcut for ( BitSet1 & BitSet2 ).IsEmpty().

Input:
    CBitSetType other - Other bit set

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::Intersects( const CBitSetType& other ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool intersects = false;

    if( StoredInPtr() && other.StoredInPtr() )
    {
        intersects = ( ( m_PtrBits & other.m_PtrBits ) != static_cast<bitptr_t>( 0 ) );
    }
    else
    {
        // Size of bitptr_t must be multiplicity of size of BITSET_ARRAY_TYPE.
        C_ASSERT( sizeof( bitptr_t ) % sizeof( BITSET_ARRAY_TYPE ) == 0 );

        // If stored in pointer, get pointer to this pointer and use 
        // common implementation:
        const BITSET_ARRAY_TYPE* ptrThis = GetArrayPointer();
        const BITSET_ARRAY_TYPE* ptrOther = other.GetArrayPointer();

        DWORD minSize = ( m_Size < other.m_Size ) ? m_Size : other.m_Size;

        DWORD index = ( minSize + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        while( !intersects && index-- )
        {
            if( ( ptrThis[ index ] & ptrOther[ index ] ) != 0 )
            {
                intersects = true;
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return intersects;
}

/*****************************************************************************\

Function:
    CBitSet::GetNextMember

Description:
    Gets the next member in the set, starting from the specified index.
    If there are no additional members in the set, returns the size of
    the set.

    Example usage pattern:

    for(
        DWORD value = bitSet.GetNextMember( 0 );
        value < bitSet.GetSize();
        value = bitSet.GetNextMember( ++value ) )

Input:
    DWORD start - Index to start the search

Output:
    DWORD - The next member of the set, or the size of the set if there are
            no more members in the set.

\*****************************************************************************/
template<BitSetTemplateList>
DWORD CBitSetType::GetNextMember( DWORD start ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    DWORD   nextMember = m_Size;

    if( start < m_Size )
    {
        if( StoredInPtr() )
        {
            bitptr_t ptrBits = m_PtrBits;

            // Mask out bits to start:
            ptrBits &= ~( CPtrSize::Bit( start ) - 1 );

            // Find first bit:
            if( ptrBits != static_cast<bitptr_t>( 0 ) )
            {
                nextMember = CPtrSize::Bsf( ptrBits );
            }
        }
        else
        {
            const DWORD startArrayIndex = start / cBitsPerArrayElement;
            const DWORD endArrayIndex = ( m_Size - 1 ) / cBitsPerArrayElement;

            const DWORD startBit = start % cBitsPerArrayElement;
            const DWORD endBit = m_Size % cBitsPerArrayElement;

            const BITSET_ARRAY_TYPE startMask = ~( ( 1 << startBit ) - 1 );
            const BITSET_ARRAY_TYPE endMask = endBit ? ( ( 1 << endBit ) - 1 ) : ~0;

            DWORD   arrayIndex = startArrayIndex;

            BITSET_ARRAY_TYPE   data = m_BitSetArray[ arrayIndex ];

            data &= startMask;

            if( arrayIndex == endArrayIndex )
            {
                data &= endMask;
            }
            else
            {
                if( data == 0 )
                {
                    for( arrayIndex = arrayIndex + 1;
                         arrayIndex < endArrayIndex;
                         arrayIndex++ )
                    {
                        data = m_BitSetArray[ arrayIndex ];

                        if( data != 0 )
                        {
                            break;
                        }
                    }

                    if( data == 0 )
                    {
                        data = m_BitSetArray[ endArrayIndex ];
                        data &= endMask;
                    }
                }
            }

            if( data != 0 )
            {
                // Found, find the first bit that's on in "data".
                const DWORD lsb = bsf( data );
                nextMember = ( arrayIndex * cBitsPerArrayElement ) + lsb;
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return nextMember;
}

/*****************************************************************************\

Function:
    CBitSet::Set

Description:
    Sets the bit at the given index.

Input:
    DWORD index - index of the bit to set

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Set( DWORD index )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    // If the index is larger than the size of the BitSet then grow the BitSet
    if( index >= m_Size )
    {
        Create( index + 1 );
    }

    ASSERT( index < m_Size );

    if( StoredInPtr() )
    {
        m_PtrBits |= CPtrSize::Bit( index );
    }
    else
    {
        DWORD   arrayIndex = index / cBitsPerArrayElement;
        DWORD   bitIndex = index % cBitsPerArrayElement;

        m_BitSetArray[arrayIndex] |= BIT(bitIndex);
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::Set

Description:
    Sets all of the given bits.

Input:
    CBitSetType other - bits to set.

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Set( const CBitSetType& other )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DWORD size = other.m_Size;
    
    if( m_Size < other.m_Size )
    {
        Create( other.m_Size );
        size = m_Size;
    }

    ASSERT( m_Size >= other.m_Size );

    if( StoredInPtr() )
    {
        ASSERT( other.StoredInPtr() );
        m_PtrBits |= other.m_PtrBits;
    }
    else
    {
        const BITSET_ARRAY_TYPE* pOtherArray = other.GetArrayPointer();
        DWORD index = ( size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;
        while( index-- )
        {
            m_BitSetArray[ index ] |= pOtherArray[ index ];
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::UnSet

Description:
    Un-Sets the bit at the given index.

Input:
    DWORD index - index of the bit to un-set

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::UnSet( DWORD index )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    // If the index is larger than the size of the BitSet then grow the BitSet
    if( index >= m_Size )
    {
        Create( index + 1 );
        if( index >= m_Size )
        {
            // In case allocation failed...
            ASSERT( index < m_Size );
            return;
        }
    }

    if( StoredInPtr() )
    {
        m_PtrBits &= ( ~CPtrSize::Bit( index ) ) & GetActivePtrMask();
    }
    else
    {
        DWORD   arrayIndex = index / cBitsPerArrayElement;
        DWORD   bitIndex = index % cBitsPerArrayElement;

        m_BitSetArray[arrayIndex] &= ~BIT(bitIndex);
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::UnSet

Description:
    Un-Sets all of the given bits.

Input:
    CBitSetType other - bits to un-set.    

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::UnSet( const CBitSetType& other )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DWORD size = other.m_Size;

    if( m_Size < other.m_Size )
    {
        Create( other.m_Size );
        size = m_Size;
    }

    ASSERT( m_Size >= other.m_Size );

    if( StoredInPtr() )
    {
        ASSERT( other.StoredInPtr() );
        m_PtrBits &= ~other.m_PtrBits;
        m_PtrBits &= GetActivePtrMask();
    }
    else
    {
        const BITSET_ARRAY_TYPE* pOtherArray = other.GetArrayPointer();
        DWORD index = ( size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;
        while( index-- )
        {
            m_BitSetArray[ index ] &= ~pOtherArray[ index ];
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBitSet::GetSize

Description:
    Returns the number of bits in the BitSet

Input:
    void

Output:
    DWORD length

\*****************************************************************************/
template<BitSetTemplateList>
DWORD CBitSetType::GetSize( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const DWORD size = m_Size;

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return size;
}

/*****************************************************************************\

Function:
    CBitSet::Min

Description:
    Returns the minimum bit set

Input:
    void

Output:
    long

\*****************************************************************************/
template<BitSetTemplateList>
long CBitSetType::Min( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    long minBit = -1;

    if( StoredInPtr() )
    {
        if( m_PtrBits )
        {
            minBit = CPtrSize::Bsf( m_PtrBits );
        }
    }
    else
    {
        const DWORD count = ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        for( DWORD i = 0; i < count; ++i )
        {
            if( m_BitSetArray[i] != 0 )
            {
                const DWORD lsb = bsf( m_BitSetArray[i] );
                minBit = (i * cBitsPerArrayElement) + lsb;
                break;
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return minBit;
}

/*****************************************************************************\

Function:
    CBitSet::Max

Description:
    Returns the maximum bit set

Input:
    void

Output:
    long

\*****************************************************************************/
template<BitSetTemplateList>
long CBitSetType::Max( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    long maxBit = -1;

    if( StoredInPtr() )
    {
        if( m_PtrBits )
        {
            maxBit = CPtrSize::Bsr( m_PtrBits );
        }
    }
    else
    {
        const DWORD count = ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        for( long i = count-1; i >= 0; --i )
        {
            if( m_BitSetArray[i] != 0 )
            {
                const DWORD msb = bsr( m_BitSetArray[i] );
                maxBit = (i * cBitsPerArrayElement) + msb;
                break;
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return maxBit;
}

/*****************************************************************************\

Function:
    template<class T>
    CBitSet::ConvertTo

Description:
    Returns a T representation of the current bit set.  Only valid if the
    bit set consists of less than (or equal to) sizeof(T) * BITS_PER_BYTE bits.

    It is expected that the types that bitsets get converted to will be BYTES,
    WORDS, or DWORDS, although other types may work as well.

Input:
    void

Output:
    T representation of the bitfield.

\*****************************************************************************/
template<BitSetTemplateList> template<class T>
T CBitSetType::ConvertTo() const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    T   mask = 0;

    if( StoredInPtr() )
    {
        bitptr_t bits = m_PtrBits;
        mask = (T)( bits );
    }
    else
    {
        ASSERT( m_BitSetArray );
        mask = ((T*)m_BitSetArray)[0];
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return mask;
}

/*****************************************************************************\

Function:
    CBitSet::operator ==

Description:
    Tests this bitset and another bitset for equality.

Input:
    CBitSetType& other - other BitSet

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::operator==( const CBitSetType& other ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool isEqual = false;

    if( m_Size == other.m_Size )
    {
        ASSERT( this->StoredInPtr() == other.StoredInPtr() );

        if( StoredInPtr() )
        {
            isEqual = ( m_PtrBits == other.m_PtrBits );
        }
        else
        {
            const DWORD cArraySizeInBytes = 
                ( m_Size + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;

            isEqual = ( 0 ==
                SafeMemCompare( m_BitSetArray,
                other.m_BitSetArray,
                cArraySizeInBytes ) );
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return isEqual;
}

/*****************************************************************************\

Function:
    CBitSet::operator !=

Description:
    Tests this bitset and another bitset for inequality.

Input:
    CBitSetType& other - other BitSet

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::operator!=( const CBitSetType& other ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool isNotEqual = true;

    if( m_Size == other.m_Size )
    {
        ASSERT( this->StoredInPtr() == other.StoredInPtr() );

        if( StoredInPtr() )
        {
            isNotEqual = ( m_PtrBits != other.m_PtrBits );
        }
        else
        {
            const DWORD cArraySizeInBytes = 
                ( m_Size + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;

            isNotEqual = ( 0 !=
                SafeMemCompare( m_BitSetArray,
                other.m_BitSetArray,
                cArraySizeInBytes ) );
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return isNotEqual;
}

/*****************************************************************************\

Function:
    CBitSet::operator =

Description:
    Equal operator to copy a BitSet

Input:
    CBitSetType& other - BitSet to copy

Output:
    *this

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType& CBitSetType::operator= ( const CBitSetType &other )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    Copy( other );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return *this;
}

/*****************************************************************************\

Function:
    CBitSet::operator |=

Description:
    Computes the union of this bitset with another bitset.

Input:
    CBitSetType& other - other BitSet

Output:
    *this

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType& CBitSetType::operator|= ( const CBitSetType &other )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DWORD size = other.m_Size;

    if( m_Size < other.m_Size )
    {
        Create( other.m_Size );
        size = m_Size;
    }

    ASSERT( m_Size >= other.m_Size );

    if( StoredInPtr() )
    {
        ASSERT( other.StoredInPtr() );
        m_PtrBits |= other.m_PtrBits;
    }
    else
    {
        const BITSET_ARRAY_TYPE* pOtherArray = other.GetArrayPointer();
        DWORD index = ( size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;
        while( index-- )
        {
            m_BitSetArray[ index ] |= pOtherArray[ index ];
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return *this;
}

/*****************************************************************************\

Function:
    CBitSet::operator &=

Description:
    Computes the intersection of this bitset with another bitset.

Input:
    CBitSetType& other - other BitSet

Output:
    *this

\*****************************************************************************/
template<BitSetTemplateList>
CBitSetType& CBitSetType::operator&= ( const CBitSetType &other )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DWORD size = other.m_Size;

    if( m_Size < other.m_Size )
    {
        Create( other.m_Size );
        size = m_Size;
    }

    ASSERT( m_Size >= other.m_Size );

    if( StoredInPtr() )
    {
        ASSERT( other.StoredInPtr() );
        m_PtrBits &= other.m_PtrBits;
    }
    else
    {
        const BITSET_ARRAY_TYPE* pOtherArray = other.GetArrayPointer();
        DWORD index = ( size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;
        while( index-- )
        {
            m_BitSetArray[ index ] &= pOtherArray[ index ];
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return *this;
}

/*****************************************************************************\

Function:
    CBitSet::Create

Description:
    Creates the internal BitSet structure of the specified size

Input:
    DWORD size - number of elements

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Create( DWORD size )
{
    if( size == m_Size )
    {
        // Nothing to do...
    }
    if( size <= cBitsInPtr )
    {
        // Bitset will fit in pointer.
        bitptr_t newPtrBits = this->ConvertTo<bitptr_t>();

        Delete();

        m_Size = size;
        m_PtrBits = newPtrBits & GetActivePtrMask();
    }
    else
    {
        BITSET_ARRAY_TYPE* ptrThis = GetArrayPointer();

        const DWORD cNewArraySize = 
            (   size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;
        const DWORD cOldArraySize = 
            ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        const BITSET_ARRAY_TYPE cExtraBits = 
            size % cBitsPerArrayElement;
        const BITSET_ARRAY_TYPE cExtraBitMask = 
            cExtraBits ? ( ( 1 << cExtraBits ) - 1 ) : ( ~0 );

        if( cNewArraySize == cOldArraySize )
        {
            m_Size = size;
            if( cNewArraySize )
            {
                ptrThis[ cNewArraySize - 1 ] &= cExtraBitMask;
            }
        }
        else if( cNewArraySize )
        {
            BITSET_ARRAY_TYPE*  ptr = (BITSET_ARRAY_TYPE*)
                CAllocatorType::Allocate( sizeof(BITSET_ARRAY_TYPE) * cNewArraySize );

            if( ptr )
            {
                if( ptrThis )
                {
                    if( cNewArraySize > cOldArraySize )
                    {
                        MemCopy( ptr, 
                            ptrThis, 
                            cOldArraySize * sizeof(ptrThis[0]) );
                        SafeMemSet( ptr + cOldArraySize,
                            0,
                            ( cNewArraySize - cOldArraySize) * sizeof(ptrThis[0]) );
                    }
                    else
                    {
                        MemCopy( ptr,
                            ptrThis,
                            cNewArraySize * sizeof(ptrThis[0]) );
                        if( cNewArraySize )
                        {
                            ptr[ cNewArraySize - 1 ] &= cExtraBitMask;
                        }
                    }
                }
                else
                {
                    SafeMemSet( ptr,
                        0,
                        cNewArraySize * sizeof(ptrThis[0]) );
                }

                Delete();

                m_BitSetArray = ptr;
                m_Size = size;
            }
            else
            {
                ASSERT( 0 );
            }
        }
        else
        {
            Delete();
        }
    }
}

/*****************************************************************************\

Function:
    CBitSet::Copy

Description:
    Copies information from one bitset to this bitset.

Input:
    const CBitSetType& other - bitset to copy.

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Copy( const CBitSetType& other )
{
    if( this != &other )
    {
        if( m_Size != other.m_Size )
        {
            Create( other.m_Size );
        }

        if( m_Size == other.m_Size )
        {
            ASSERT( this->StoredInPtr() == other.StoredInPtr() );

            if( StoredInPtr() )
            {
                m_PtrBits = other.m_PtrBits;
            }
            else
            {
                const DWORD cArraySizeInBytes = 
                    ( other.m_Size + BITS_PER_BYTE - 1 ) / BITS_PER_BYTE;

                MemCopy( m_BitSetArray,
                    other.m_BitSetArray,
                    cArraySizeInBytes );
            }
        }
        else
        {
            // Should not happen - indicates failed memory allocation.
            ASSERT( 0 );
        }
    }
}

/*****************************************************************************\

Function:
    CBitSet::Delete

Description:
    Deletes the internal BitSet structure

Input:
    void

Output:
    void

\*****************************************************************************/
template<BitSetTemplateList>
void CBitSetType::Delete( void )
{
    ASSERT( m_BitSetArray || StoredInPtr() );
    if( !StoredInPtr() && m_BitSetArray )
    {
        CAllocatorType::Deallocate( m_BitSetArray );
    }
    m_BitSetArray = NULL;

    m_Size = 0;
}

/*****************************************************************************\

Function:
    CBitSet::BitCount

Description:
    Counts number of bits set in BitSet

Input:
    void

Output:
    DWORD number of bits set in BitSet

\*****************************************************************************/
template<BitSetTemplateList>
DWORD CBitSetType::BitCount( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    DWORD bitCount = 0;

    if( StoredInPtr() )
    {
        bitCount = CPtrSize::Count( m_PtrBits );
    }
    else
    {
        const DWORD cBitsPerArrayElement = 
            ( sizeof(m_BitSetArray[0]) * 8 );

        DWORD index = ( m_Size + cBitsPerArrayElement - 1 ) / cBitsPerArrayElement;

        while( index-- )
        {
            BITSET_ARRAY_TYPE Elem = m_BitSetArray[index];
            bitCount += iSTD::BitCount( Elem );
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return bitCount;
}

/*****************************************************************************\

Function:
    CBitSet::BitCount

Description:
    Counts number of bits set in BitSet up to (and including) limit-th index

Input:
    DWORD index

Output:
    DWORD number of bits set in BitSet up to (and including) limit-th index

\*****************************************************************************/
template<BitSetTemplateList>
DWORD CBitSetType::BitCount( DWORD limit ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    DWORD bitCount = 0;

    limit = iSTD::Min( limit, m_Size-1 );
    
    for( DWORD i=0; i <= limit; i++ )
    {
        if( IsSet( i ) )
        {
            bitCount++;
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return bitCount;
}

/*****************************************************************************\

Function:
    CBitSet::StoredInPtr

Description:
    True if bits are stored in pointer.

Input:

Output:
    bool

\*****************************************************************************/
template<BitSetTemplateList>
bool CBitSetType::StoredInPtr( void ) const 
{ 
    return m_Size <= cBitsInPtr; 
}

/*****************************************************************************\

Function:
    CBitSet::GetActivePtrMask

Description:
    Returns pointer mask depending on set size. Can only be called if bits are 
    stored in pointer.

Input:

Output:
    bitptr_t    - active mask

\*****************************************************************************/
template<BitSetTemplateList>
typename CBitSetType::bitptr_t CBitSetType::GetActivePtrMask( void ) const
{
    ASSERT( StoredInPtr() );

    if( m_Size == cBitsInPtr )
    {
        return static_cast<bitptr_t>( -1 );
    }
    return CPtrSize::Bit( m_Size ) - 1;
}

/*****************************************************************************\

Function:
    CBitSet::GetArrayPointer

Description:
    Return pointer to bitset array. If bits are stored in pointer itself, 
    return pointer to this pointer.

Input:

Output:
    BITSET_ARRAY_TYPE*

\*****************************************************************************/
template<BitSetTemplateList>
typename CBitSetType::BITSET_ARRAY_TYPE* CBitSetType::GetArrayPointer( void )
{
    if( StoredInPtr() )
    {
        return ( reinterpret_cast<BITSET_ARRAY_TYPE*>( &m_PtrBits ) );
    }
    else
    {
        return m_BitSetArray;
    }
}

/*****************************************************************************\

Function:
    CBitSet::GetArrayPointer

Description:
    Return pointer to bitset array. If bits are stored in pointer itself, 
    return pointer to this pointer.

Input:

Output:
    const BITSET_ARRAY_TYPE*

\*****************************************************************************/
template<BitSetTemplateList>
const typename CBitSetType::BITSET_ARRAY_TYPE* CBitSetType::GetArrayPointer( void ) const
{
    if( StoredInPtr() )
    {
        return ( reinterpret_cast<const BITSET_ARRAY_TYPE*>( &m_PtrBits ) );
    }
    else
    {
        return m_BitSetArray;
    }
}

} // iSTD

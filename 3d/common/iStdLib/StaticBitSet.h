/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace iSTD
{

/******************************************************************************\

  CStaticBitSet provides standard bitset operations when the bitset size is 
  known at compile time.  The number of bits supported is up to the template
  parameter MaxBits.  This gets rounded up to the next DWORD aligned value.
  The API is the similar to iSTD::CBitSet, but this class will not grow if you 
  access an index higher than what is allocated.  Instead, this class will
  provided a nice blue screen to alert you of the error.  It is the client's
  responsibility to do any bounds checking.

  Bits indices start at 0.  For a CStaticBitSet< 32 >, the size is 32 and the 
  valid range of indicies is 0 through 31.

  Constructor - Initializes all bits to off.
  Clear       - Turns all bits to off.
  SetAll      - Turns all bits to on.

  Set         - Turns one or more bits on.
  Unset       - Turns one bit off.
  IsSet       - Returns true if a bit is on, false otherwise.

\******************************************************************************/
template< DWORD MaxBits >
class CStaticBitSet 
{
public:
    CStaticBitSet( void );

    void Clear( void );
    void SetAll( void );

    void Set( const DWORD index );
    void Set( const DWORD index, const DWORD count );

    void UnSet( const DWORD index );
    bool IsSet( const DWORD index ) const;

    DWORD BitCount() const;
    DWORD BitCount( DWORD limit ) const;

protected:
    enum { ArraySize = (MaxBits + sizeof(DWORD)*8 - 1 ) >> 5 };

    DWORD GetArrayIndex( const DWORD bitNum ) const { return bitNum >> 5; }
    DWORD GetBitIndex( const DWORD bitNum )   const { return bitNum & 0x1F; }
    DWORD BitNumber( const DWORD number ) const { return 1 << number; }

    DWORD   m_bits[ ArraySize ];
#ifdef _DEBUG
    bool    m_debugBits[ MaxBits ];
#endif
};

/******************************************************************************\
  CStaticBitSet
\******************************************************************************/
template< DWORD MaxBits >
CStaticBitSet< MaxBits >::CStaticBitSet( void ) 
{
    C_ASSERT( MaxBits >= 1 );
    Clear();
}

/******************************************************************************\
  CStaticBitSet::Clear
\******************************************************************************/
template< DWORD MaxBits >
void CStaticBitSet< MaxBits >::Clear( void )
{
    SafeMemSet( &m_bits, 0, sizeof( m_bits[ 0 ] ) * ArraySize );

#ifdef _DEBUG
    for( DWORD ndx = 0; ndx <  MaxBits; ndx++ )
    {
        m_debugBits[ ndx ] = false;
    }
#endif
}

/******************************************************************************\
  CStaticBitSet::SetAll
\******************************************************************************/
template< DWORD MaxBits >
void CStaticBitSet< MaxBits >::SetAll( void )
{
    SafeMemSet( &m_bits, 0xFF, sizeof( m_bits[ 0 ] ) * ArraySize );

#ifdef _DEBUG
    for( DWORD ndx = 0; ndx <  MaxBits; ndx++ )
    {
        m_debugBits[ ndx ] = true;
    }
#endif
}

/******************************************************************************\
  CStaticBitSet::Set
\******************************************************************************/
template< DWORD MaxBits >
void CStaticBitSet< MaxBits >::Set( const DWORD index )
{
#ifdef _DEBUG
    ASSERT( IsSet( index ) == m_debugBits[ index ] );
#endif
    ASSERT( GetArrayIndex( index ) <= ArraySize );

    DWORD arrayIndex = GetArrayIndex( index );
    DWORD bitIndex   = GetBitIndex( index );
        
    m_bits[ arrayIndex ] |= BitNumber( bitIndex );

#ifdef _DEBUG
    m_debugBits[ index ] = true;
#endif
}

/******************************************************************************\

  CStaticBitSet::Set (contiguous version) - Sets a contiguous number of bits
  that could span multiple DWORDS.  Optimized towards setting a contiguous 
  amount of bits that span many DWORDS.

  This algorithm takes advantage of the property that if you set a contiguous
  set of bits that span multiple DWORDs, all the DWORDs between the first
  and last bits can be set to 0xFFFFFFFF.  There is not need to calculate
  which bits need to be set.  Only the bits in the first and last DWORDs
  need to be calculated.

  Notes: This function is specifically coded for m_bits to be DWORDs.  
  If you change the m_bits type, you will need to change this function.

\******************************************************************************/
template< DWORD MaxBits >
void CStaticBitSet< MaxBits >::Set( const DWORD index, DWORD count )
{
    ASSERT( GetArrayIndex( index ) <= ArraySize );

#ifdef _DEBUG
    for( DWORD ndx = 0; ndx < count; ndx++) 
    {
        m_debugBits[ index + ndx ] = true;
    }
#endif

    DWORD arrayIndex = GetArrayIndex( index );
    DWORD bitIndex   = GetBitIndex( index );
    DWORD mask;

    const DWORD BITS_PER_ELEMENT = 32;

    if( ( bitIndex + count ) <= BITS_PER_ELEMENT ) // Spans only a single DWORD
    {
        // Promote to QWORD due to bug when shifting 0x1 by 32 becomes 0x1 
        // instead of our desired 0x0.  Seems like it is a rotate shift.
        mask = (((QWORD)1 << count ) - 1 ) << bitIndex;
        m_bits[ arrayIndex ] |= mask;
    }
    else
    {
        // Set the bits in the first DWORD
        mask = (DWORD) (QWORD) ( ( (DWORD) -1 ) << bitIndex ) ;
        m_bits[ arrayIndex ] |= mask ;
        arrayIndex++; 
        count = count - ( BITS_PER_ELEMENT - bitIndex );

        // Set the bits in the middle DWORDs
        while( count >= BITS_PER_ELEMENT )
        {
            m_bits[ arrayIndex ] = 0xFFFFFFFF;
            arrayIndex++;
            count -= BITS_PER_ELEMENT;
        }
        // Set the bits in the last DWORD
        mask = ( (QWORD)1 << count ) - 1;
        m_bits[ arrayIndex ] |= mask;
    }

#ifdef _DEBUG    
    for( DWORD ndx = 0; ndx < MaxBits; ndx++) 
    {
        ASSERT( m_debugBits[ ndx ] == IsSet( ndx ) );
    }
#endif
}

/******************************************************************************\
  CStaticBitSet::UnSet
\******************************************************************************/
template< DWORD MaxBits >
void CStaticBitSet< MaxBits >::UnSet( const DWORD index )
{
#ifdef _DEBUG
    ASSERT( IsSet( index ) == m_debugBits[ index ] );
#endif
    ASSERT( GetArrayIndex( index ) <= ArraySize );

    DWORD arrayIndex = GetArrayIndex( index );
    DWORD bitIndex   = GetBitIndex( index );
    m_bits[ arrayIndex ] &= ~BitNumber( bitIndex );

#ifdef _DEBUG
    m_debugBits[ index ] = false;
#endif
}

/******************************************************************************\
  CStaticBitSet::IsSet
\******************************************************************************/
template< DWORD MaxBits >
bool CStaticBitSet< MaxBits >::IsSet( const DWORD index ) const
{
    ASSERT( GetArrayIndex( index ) <= ArraySize );

    DWORD arrayIndex = GetArrayIndex( index );
    DWORD bitIndex   = GetBitIndex( index );

    bool isSet = ( m_bits[ arrayIndex ] & BitNumber( bitIndex ) ) ? true : false;

#ifdef _DEBUG
    ASSERT( isSet == m_debugBits[ index ] );
#endif
    return isSet;
}

/******************************************************************************\
  CStaticBitSet::BitCount
\******************************************************************************/
template< DWORD MaxBits >
DWORD CStaticBitSet< MaxBits >::BitCount() const
{
    DWORD bitCount = 0;

    const DWORD cBitsPerArrayElement = sizeof(m_bits[0]) * 8;

    DWORD index = ArraySize;

    while( index-- )
    {
        if( m_bits[ index ] != 0 )
        {
            for( DWORD i = 0; i < cBitsPerArrayElement; i++ )
            {
                if( m_bits[index] & (1<<i) )
                {
                    bitCount++;
                }
            }
        }
    }

    return bitCount;
}


/******************************************************************************\
  CStaticBitSet::BitCount
\******************************************************************************/
template< DWORD MaxBits >
DWORD CStaticBitSet< MaxBits >::BitCount(DWORD limit) const
{
    DWORD bitCount = 0;

    limit = iSTD::Min<DWORD>( limit, ArraySize-1 );
    
    for( DWORD i=0; i <= limit; i++ )
    {
        if( IsSet( i ) )
        {
            bitCount++;
        }
    }

    return bitCount;
}

}

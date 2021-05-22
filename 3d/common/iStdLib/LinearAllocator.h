/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Threading.h"

namespace iSTD
{

/*****************************************************************************\

Class:
    CLinearAllocator

Description:
    Manages a memory buffer via linear allocation

\*****************************************************************************/
template<class CAllocatorType>
class CLinearAllocator : public CObject<CAllocatorType>
{
public:

    CLinearAllocator( void* pBaseAddress, const DWORD size );
    virtual ~CLinearAllocator( void );

    DWORD   GetAvailableSpace( void ) const;
    DWORD   GetUsedSpace( void ) const;

    void*   GetSpace( const DWORD size );
    void*   GetSpaceAligned( const DWORD size, const DWORD alignSize );

    bool    IsEmpty( void ) const;
    bool    IsFull( void ) const;

    void    Align( const DWORD alignSize );

    void    PutSpace( const DWORD size );
    void    PutAllSpace( void );

    void*   ReserveSpace( const DWORD size );

    virtual void    Resize( const DWORD size );

protected:

    void*   m_pBaseAddress;
    DWORD   m_Size;             // Total size of memory
    DWORD   m_SizeUsed;         // Size of used memory
    DWORD   m_SizeReserved;     // Size of reserved memory

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
    CLinearAllocator Constructor

Description:
    Initializes internal data

Input:
    void* pBaseAddress
    const DWORD size

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CLinearAllocator<CAllocatorType>::CLinearAllocator(
    void* pBaseAddress,
    const DWORD size )
    : CObject<CAllocatorType>()
{
    m_pBaseAddress = pBaseAddress;
    m_Size = size;
    m_SizeUsed = 0;
    m_SizeReserved = 0;

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinearAllocator Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CLinearAllocator<CAllocatorType>::~CLinearAllocator( void )
{
    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinearAllocator::GetAvailableSpace

Description:
    Gets the amount of space available in the buffer

Input:
    none

Output:
    DWORD - size in bytes

\*****************************************************************************/
template<class CAllocatorType>
inline DWORD CLinearAllocator<CAllocatorType>::GetAvailableSpace( void ) const
{
    const DWORD size = m_Size - GetUsedSpace();
    return size;
}

/*****************************************************************************\

Function:
    CLinearAllocator::GetUsedSpace

Description:
    Gets the amount of space used in the buffer

Input:
    none

Output:
    DWORD - size in bytes

\*****************************************************************************/
template<class CAllocatorType>
inline DWORD CLinearAllocator<CAllocatorType>::GetUsedSpace( void ) const
{
    const DWORD size = m_SizeUsed + m_SizeReserved;
    return size;
}

/*****************************************************************************\

Function:
    CLinearAllocator::GetSpace

Description:
    Gets space from the top of the buffer

Input:
    const DWORD size - size in bytes

Output:
    void* - linear address of space

\*****************************************************************************/
template<class CAllocatorType>
inline void* CLinearAllocator<CAllocatorType>::GetSpace( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    void* pAddress = NULL;

    if( GetAvailableSpace() >= size )
    {
        pAddress = (BYTE*)m_pBaseAddress + m_SizeUsed;
        m_SizeUsed += size;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return pAddress;
}

/*****************************************************************************\

Function:
    CLinearAllocator::GetSpaceAligned

Description:
    Gets space from the top of the buffer

Input:
    const DWORD size - size in bytes
    const DWORD alignSize - alignment in bytes

Output:
    void* - linear address of space

\*****************************************************************************/
template<class CAllocatorType>
inline void* CLinearAllocator<CAllocatorType>::GetSpaceAligned(
    const DWORD size,
    const DWORD alignSize )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    void* pAddress = NULL;

    if( GetAvailableSpace() >= size )
    {
        // Determine the number of bytes required to
        // align the allocation
        const DWORD offset = GetAlignmentOffset(
            (BYTE*)m_pBaseAddress + m_SizeUsed,
            alignSize );

        if( offset )
        {
            if( ( GetAvailableSpace() >= offset ) &&
                ( GetAvailableSpace() >= offset + size ) )
            {
                pAddress = (BYTE*)m_pBaseAddress + m_SizeUsed + offset;
                m_SizeUsed += size + offset;
            }
        }
        else
        {
            pAddress = (BYTE*)m_pBaseAddress + m_SizeUsed;
            m_SizeUsed += size;
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return pAddress;
}

/*****************************************************************************\

Function:
    CLinearAllocator::IsEmpty

Description:
    Determines if the buffer is empty

Input:
    void

Output:
    bool

\*****************************************************************************/
template<class CAllocatorType>
inline bool CLinearAllocator<CAllocatorType>::IsEmpty( void ) const
{
    const bool isEmpty = ( m_SizeUsed == 0 ) && ( m_SizeReserved == 0 );
    return isEmpty;
}

/*****************************************************************************\

Function:
    CLinearAllocator::IsFull

Description:
    Determines if the buffer is full

Input:
    void

Output:
    bool

\*****************************************************************************/
template<class CAllocatorType>
inline bool CLinearAllocator<CAllocatorType>::IsFull( void ) const
{
    const bool isFull = ( GetAvailableSpace() == 0 );
    return isFull;
}

/*****************************************************************************\

Function:
    CLinearAllocator::Align

Description:
    Aligns the buffer and pads with zeros

Input:
    const DWORD alignSize - alignment in bytes

Output:
    void

\*****************************************************************************/
template<class CAllocatorType>
inline void CLinearAllocator<CAllocatorType>::Align( const DWORD alignSize )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const DWORD offset = GetAlignmentOffset(
        (BYTE*)m_pBaseAddress + m_SizeUsed,
        alignSize );

    if( offset )
    {
        if( m_Size >= m_SizeUsed + offset )
        {
            m_SizeUsed += offset;
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinearAllocator::PutSpace

Description:
    Puts space back at the top of the buffer

Input:
    const DWORD size - size in bytes

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CLinearAllocator<CAllocatorType>::PutSpace( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    m_SizeUsed -= size;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinearAllocator::PutAllSpace

Description:
    Puts all space back

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CLinearAllocator<CAllocatorType>::PutAllSpace( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    m_SizeUsed = 0;
    m_SizeReserved = 0;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinearAllocator::ReserveSpace

Description:
    Reserves space at the bottom of the buffer

Input:
    const DWORD size - size in bytes

Output:
    void* - linear address of reserved space

\*****************************************************************************/
template<class CAllocatorType>
inline void* CLinearAllocator<CAllocatorType>::ReserveSpace( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    void* pAddress = NULL;

    if( GetAvailableSpace() >= size )
    {
        pAddress = (BYTE*)m_pBaseAddress + m_Size -
            ( m_SizeReserved + size );
        m_SizeReserved += size;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return pAddress;
}

/*****************************************************************************\

Function:
    CLinearAllocator::Resize

Description:
    Changes the size of the buffer

Input:
    const DWORD size - size in bytes

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
void CLinearAllocator<CAllocatorType>::Resize( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    m_Size = size;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

} // iSTD

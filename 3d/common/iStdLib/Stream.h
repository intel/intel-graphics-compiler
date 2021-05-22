/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Array.h"
#include "Buffer.h"
#include "utility.h"
#include "MemCopy.h"

namespace iSTD
{

/*****************************************************************************\

Class:
    CStream

Description:
    Allocates and manages a system memory buffer of unknown size

\*****************************************************************************/
template<class CAllocatorType>
class CStream : public CObject<CAllocatorType>
{
public:

    CStream( void );
    virtual ~CStream( void );

    void*   GetSpace( const DWORD dwSize );
    void*   GetLinearAddress( void );
    DWORD   GetUsedSpace( void ) const;
    void    SetUsedSpace( DWORD currentSizeUsed );
    void    PutAllSpace( void );

    void*   GetSegmentAddress( const DWORD offset, const DWORD size ) const;

protected:

    CDynamicArray<CBuffer<CAllocatorType>*,CAllocatorType> m_StreamArray;

    CBuffer<CAllocatorType>*    m_pCurrentBuffer;
    DWORD                       m_StreamBufferCount;
    CBuffer<CAllocatorType>*    m_pStreamBuffer;
    DWORD                       m_dwSizeUsed;
};

/*****************************************************************************\

Function:
    CStream Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CStream<CAllocatorType>::CStream( void )
    : CObject<CAllocatorType>(),
      m_StreamArray(0)
{
    m_pCurrentBuffer = NULL;
    m_StreamBufferCount = 0;
    m_pStreamBuffer = NULL;
    m_dwSizeUsed = 0;
}

/*****************************************************************************\

Function:
    CStream Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CStream<CAllocatorType>::~CStream( void )
{
    const DWORD dwCount = m_StreamArray.GetSize();
    for( DWORD i = 0; i < dwCount; i++ )
    {
        CBuffer<CAllocatorType>* pBuffer = m_StreamArray.GetElement(i);
        SafeDelete( pBuffer );
    }

    SafeDelete( m_pStreamBuffer );
}

/*****************************************************************************\

Function:
    CStream::GetSpace

Description:
    Gets space from the stream

Input:
    const DWORD dwSize - size in bytes

Output:
    void* - linear address of space

\*****************************************************************************/
template<class CAllocatorType>
inline void* CStream<CAllocatorType>::GetSpace( const DWORD dwSize )
{
    // If this is the first GetSpace call, or
    if( ( !m_pCurrentBuffer ) ||
        // the size requested is larger than the size available
        ( dwSize > m_pCurrentBuffer->GetAvailableSpace() ) )
    {
        // Create a new buffer large enough for the requested size
        m_pCurrentBuffer = new CBuffer<CAllocatorType>();
        ASSERT( m_pCurrentBuffer );

        if( m_pCurrentBuffer &&
            m_pCurrentBuffer->Allocate( Max( (DWORD)0x1000, dwSize ), sizeof(DWORD) ) )
        {
            // Add the buffer to the array of buffers
            if( m_StreamArray.SetElement( m_StreamBufferCount, m_pCurrentBuffer ) )
            {
                // Keep track of the number of buffers in the array
                m_StreamBufferCount++;
            }
            else
            {
                ASSERT(0);
                SafeDelete( m_pCurrentBuffer );
                m_pCurrentBuffer = NULL;
            }
        }
        else
        {
            ASSERT(0);
            SafeDelete( m_pCurrentBuffer );
            m_pCurrentBuffer = NULL;
        }
    }

    if( m_pCurrentBuffer )
    {
        // Keep track of the total size of all buffers
        m_dwSizeUsed += dwSize;
        return m_pCurrentBuffer->GetSpace( dwSize );
    }
    else
    {
        return NULL;
    }
}

/*****************************************************************************\

Function:
    CStream::GetLinearAddress

Description:
    Gets the contiguous linear address of the entire stream

Input:
    none

Output:
    void* - linear address

\*****************************************************************************/
template<class CAllocatorType>
inline void* CStream<CAllocatorType>::GetLinearAddress( void )
{
    // If there are buffers in the array that have not yet been combined
    if( m_StreamBufferCount )
    {
        // Create a new stream buffer large enough for all buffers
        CBuffer<CAllocatorType>* pStreamBuffer = new CBuffer<CAllocatorType>();
        ASSERT( pStreamBuffer );

        if( pStreamBuffer &&
            pStreamBuffer->Allocate( m_dwSizeUsed, sizeof(DWORD) ) )
        {
            // If there already exists another stream buffer
            if( m_pStreamBuffer )
            {
                // Add the contents of the previous buffer to the new one
                MemCopy(
                    pStreamBuffer->GetSpace( m_pStreamBuffer->GetUsedSpace() ),
                    m_pStreamBuffer->GetLinearAddress(),
                    m_pStreamBuffer->GetUsedSpace() );

                SafeDelete( m_pStreamBuffer );
            }

            // For each buffer in the buffer array
            for( DWORD i = 0; i < m_StreamBufferCount; i++ )
            {
                CBuffer<CAllocatorType>* pBuffer = m_StreamArray.GetElement(i);

                if( pBuffer )
                {
                    // Append the contents of the buffers to the stream buffer
                    MemCopy(
                        pStreamBuffer->GetSpace( pBuffer->GetUsedSpace() ),
                        pBuffer->GetLinearAddress(),
                        pBuffer->GetUsedSpace() );

                    SafeDelete( pBuffer );

                    // Remove the buffer from the array
                    m_StreamArray.SetElement( i, NULL );
                }
            }

            m_pCurrentBuffer = NULL;
            m_StreamBufferCount = 0;
            m_pStreamBuffer = pStreamBuffer;
        }
        else
        {
            ASSERT(0);
            SafeDelete( pStreamBuffer );
            pStreamBuffer = NULL;

            // If we fail to recombine the buffers into a single linear allocation
            // then return NULL to indicate the failure
            return NULL;
        }
    }

    return ( m_pStreamBuffer )
           ? m_pStreamBuffer->GetLinearAddress()
           : NULL;
}

/*****************************************************************************\

Function:
    CStream::GetUsedSpace

Description:
    Gets the current size of the stream

Input:
    none

Output:
    DWORD - size in bytes

\*****************************************************************************/
template<class CAllocatorType>
inline DWORD CStream<CAllocatorType>::GetUsedSpace( void ) const
{
    return m_dwSizeUsed;
}

/*****************************************************************************\

Function:
    CStream::SetUsedSpace

Description:
    Sets the current size of the stream.
    Function should be used only to sets smaller size.

Input:
    DWORD - size in bytes

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CStream<CAllocatorType>::SetUsedSpace( DWORD currentSizeUsed )
{
    ASSERT( currentSizeUsed == 0 );
    ASSERT( currentSizeUsed < m_dwSizeUsed );

    m_dwSizeUsed = currentSizeUsed;
}

/*****************************************************************************\

Function:
    CStream::PutAllSpace

Description:
    Puts all space back into the CStream.  All space becomes unused.

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CStream<CAllocatorType>::PutAllSpace( void )
{
    const DWORD dwCount = m_StreamArray.GetSize();

    for( DWORD i = 0; i < dwCount; i++ )
    {
        CBuffer<CAllocatorType>* pBuffer = m_StreamArray.GetElement(i);
        if( pBuffer )
        {
            pBuffer->PutAllSpace();
        }
    }
    m_dwSizeUsed = 0;
    
    if( m_pStreamBuffer )
    {
        m_pStreamBuffer->PutAllSpace();
    }
}

/*****************************************************************************\

Function:
    CStream::GetSegmentAddress

Description:
    Gets the contiguous linear address a segment of the stream

Input:
    const DWORD offset - the offset in the stream of the segment
    const DWORD size - the size in bytes of the segment

Output:
    void* - linear address

Notes:
    The address returned is only valid for the segment requested. It should
    not be expected that memory access outside the returned segment is valid.

    If a segment straddles more than one stream buffer, then the request will
    fail.  This condition will not happen as long as the user accesses segments
    in the same location and size in which they were added to the stream using
    GetSpace calls.

\*****************************************************************************/
template<class CAllocatorType>
inline void* CStream<CAllocatorType>::GetSegmentAddress(
    const DWORD offset,
    const DWORD size ) const
{
    DWORD currentOffset = offset;

    if( m_pStreamBuffer )
    {
        if( currentOffset < m_pStreamBuffer->GetUsedSpace() )
        {
            if( ( currentOffset + size ) <= m_pStreamBuffer->GetUsedSpace() )
            {
                return (BYTE*)m_pStreamBuffer->GetLinearAddress() + offset;
            }
            else
            {
                // The offset is within the buffer, but the size requested
                // is too large for a contiguous address
                ASSERT(0);
                return NULL;
            }
        }
        else
        {
            currentOffset -= m_pStreamBuffer->GetUsedSpace();
        }
    }

    for( DWORD i = 0; i < m_StreamBufferCount; i++ )
    {
        CBuffer<CAllocatorType>* pBuffer = m_StreamArray.GetElement(i);

        if( pBuffer )
        {
            if( currentOffset < pBuffer->GetUsedSpace() )
            {
                if( ( currentOffset + size ) <= pBuffer->GetUsedSpace() )
                {
                    return (BYTE*)pBuffer->GetLinearAddress() + currentOffset;
                }
                else
                {
                    // The offset is within the buffer, but the size requested
                    // is too large for a contiguous address
                    ASSERT(0);
                    return NULL;
                }
            }
            else
            {
                currentOffset -= pBuffer->GetUsedSpace();
            }
        }
    }

    // offset not found
    ASSERT(0);
    return NULL;
}

} // iSTD

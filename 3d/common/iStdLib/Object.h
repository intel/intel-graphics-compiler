/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include "Alloc.h"
#include <limits.h>

#ifdef ISTDLIB_MT
#include "Threading.h"
#endif

#if defined(MSVC)
    #define STD_API_CALL __stdcall
#else
    #define STD_API_CALL
#endif

#define NO_VTABLE __declspec(novtable)

namespace iSTD
{

/*****************************************************************************\

Class:
    CObject

Description:
    Base class for all objects

\*****************************************************************************/
template<class CAllocatorType>
class NO_VTABLE CObject
{
public:

    long    Acquire( void );
    long    Release( void );

    long    GetRefCount( void ) const;

    static long STD_API_CALL   SafeAcquire(CObject* ptr);
    static long STD_API_CALL   SafeRelease(CObject* ptr);

    void*   operator new( size_t size );
    void*   operator new[]( size_t size );
    void*   operator new( size_t size, void* placement);
    void    operator delete(void* ptr);
    void    operator delete[]( void* ptr );
    void    operator delete(void* ptr, void* placement);

protected:

    CObject( void );
    virtual ~CObject(void);

#ifdef ISTDLIB_MT
    volatile long   m_RefCount;
#else
    long    m_RefCount;
#endif
};

/*****************************************************************************\

Function:
    CObject constructor

Description:
    Initializes internal data

Input:
    default

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CObject<CAllocatorType>::CObject( void )
{
#ifdef ISTDLIB_MT
    ASSERT( IsAligned( (void*)&m_RefCount, sizeof(DWORD) ) );
#endif
    m_RefCount = 0;
}

/*****************************************************************************\

Function:
    CObject destructor

Description:
    Deletes internal data

Input:
    default

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CObject<CAllocatorType>::~CObject(void)
{
    ASSERT( m_RefCount == 0 );
}

/*****************************************************************************\

Function:
    CObject::Acquire

Description:
    Increments and returns the current reference count

Input:
    none

Output:
    long - reference count

\*****************************************************************************/
template<class CAllocatorType>
inline long CObject<CAllocatorType>::Acquire( void )
{
    ASSERT( m_RefCount >= 0 );
    ASSERT( m_RefCount < LONG_MAX );

#if defined(ISTDLIB_MT) && defined(__linux__)
    __sync_fetch_and_add(&m_RefCount, 1);
#elif defined(ISTDLIB_MT)
    ::InterlockedIncrement(&m_RefCount);
#else
    ++m_RefCount;
#endif

    return m_RefCount;
}

/*****************************************************************************\

Function:
    CObject::Release

Description:
    Decrements the current reference count.  Deletes the object when the
    reference count reaches zero.  Returns the current reference count.

Input:
    none

Output:
    long - reference count

\*****************************************************************************/
template<class CAllocatorType>
inline long CObject<CAllocatorType>::Release( void )
{
    ASSERT( m_RefCount > 0 );

#if defined(ISTDLIB_MT) && defined(__linux__)
    __sync_sub_and_fetch(&m_RefCount, 1);
#elif defined(ISTDLIB_MT)
    ::InterlockedDecrement(&m_RefCount);
#else
    --m_RefCount;
#endif

    if( m_RefCount == 0 )
    {
        delete this;
        return 0;
    }
    else
    {
        return m_RefCount;
    }
}

/*****************************************************************************\

Function:
    CObject::GetRefCount

Description:
    Returns the current reference count.

Input:
    none

Output:
    long - reference count

\*****************************************************************************/
template<class CAllocatorType>
inline long CObject<CAllocatorType>::GetRefCount( void ) const
{
    return m_RefCount;
}

/*****************************************************************************\

Function:
    CObject::SafeAcquire

Description:
    Static function that calls the object's acquire function if the object
    exists

Input:
    CObject* ptr - pointer to object to acquire

Output:
    long - reference count

\*****************************************************************************/
template<class CAllocatorType>
inline long CObject<CAllocatorType>::SafeAcquire( CObject* ptr )
{
    if( ptr )
    {
        return ptr->Acquire();
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************\

Function:
    CObject::SafeRelease

Description:
    Static function that calls the object's release function if the object
    exists

Input:
    CObject* ptr - pointer to object to release

Output:
    long - reference count

\*****************************************************************************/
template<class CAllocatorType>
inline long CObject<CAllocatorType>::SafeRelease( CObject* ptr )
{
    if( ptr )
    {
        return ptr->Release();
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************\
 operator new
\*****************************************************************************/
template<class CAllocatorType>
inline void* CObject<CAllocatorType>::operator new( size_t size )
{
    ASSERT( size );

#ifdef ISTDLIB_MT
    void* ptr = CAllocatorType::AlignedAllocate( size, sizeof(DWORD) );
#else
    void* ptr = CAllocatorType::Allocate( size );
#endif

    ASSERT( ptr );
    return ptr;
}

/*****************************************************************************\
 operator new[]
\*****************************************************************************/
template<class CAllocatorType>
inline void* CObject<CAllocatorType>::operator new[]( size_t size )
{
    ASSERT( size );

#ifdef ISTDLIB_MT
    void* ptr = CAllocatorType::AlignedAllocate( size, sizeof(DWORD) );
#else
    void* ptr = CAllocatorType::Allocate( size );
#endif

    ASSERT( ptr );
    return ptr;
}

/*****************************************************************************\
 operator new with placement
\*****************************************************************************/
template<class CAllocatorType>
inline void* CObject<CAllocatorType>::operator new( size_t size, void* placement )
{
    ASSERT( size );
    ASSERT( placement );
    return placement;
}

/*****************************************************************************\
 operator delete
\*****************************************************************************/
template<class CAllocatorType>
inline void CObject<CAllocatorType>::operator delete(void* ptr)
{
    ASSERT( ptr );

#ifdef ISTDLIB_MT
    CAllocatorType::AlignedDeallocate( ptr );
#else
    CAllocatorType::Deallocate( ptr );
#endif
}

/*****************************************************************************\
 operator delete[]
\*****************************************************************************/
template<class CAllocatorType>
inline void CObject<CAllocatorType>::operator delete[]( void* ptr )
{
    ASSERT( ptr );

#ifdef ISTDLIB_MT
    CAllocatorType::AlignedDeallocate( ptr );
#else
    CAllocatorType::Deallocate( ptr );
#endif
}

/*****************************************************************************\
 operator delete from placement
\*****************************************************************************/
template<class CAllocatorType>
inline void CObject<CAllocatorType>::operator delete( void* ptr, void* placement )
{
    ASSERT( ptr );
    ASSERT( placement );
    ASSERT( ptr == placement );
}

} // iSTD

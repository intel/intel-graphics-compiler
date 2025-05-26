/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Includes for instrumentation
#if defined ( _DEBUG ) || defined ( _INTERNAL )
    #include <cstring>
    #include <stdlib.h>
    #include "IGC/common/igc_debug.h"
    #include "common/Stats.hpp"
#endif

#if defined( _WIN32 )
#include "3d/common/iStdLib/types.h"
#include <new>
#include <cstdint>

#include <intrin.h>
#include <wtypes.h>
#include <WinBase.h>

using namespace std;

#endif // _WIN32


#if ((defined(_DEBUG) || defined(_INTERNAL)) && defined(_WIN32)) //private heap for igc windows
#include <heapapi.h>
#include "igc_regkeys.hpp"
static HANDLE g_heap = NULL;
static std::atomic<unsigned int> g_alloc_cnt = 0;
static bool ReadingRegkey = true;
static bool PHEnabled = false;
#endif // _WIN32 || _WIN64

#include "Probe/Assertion.h"

#if ( defined ( _DEBUG ) || defined ( _INTERNAL ) )
/*****************************************************************************\

Class:
CAllocator

Description:
Default memory allocator class

\*****************************************************************************/
class CAllocator
{
public:
    static void*   Allocate(size_t size);
    static void    Deallocate(void* ptr);

    static void*   AlignedAllocate(size_t size, size_t alignment);
    static void    AlignedDeallocate(void* ptr);


protected:
    static void*   Malloc(size_t size);
    static void    Free(void* ptr);

    struct SAllocDesc
    {
        void*   alloc_ptr;
    };
};

/*****************************************************************************\

Function:
CAllocator::Allocate

Description:
Allocates memory from system memory pool

Input:
size_t size

Output:
void*

\*****************************************************************************/
inline void* CAllocator::Allocate(size_t size)
{
#if GET_MEM_STATS
    if(IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS))
    {
        void* ret = NULL;
        unsigned* instrPtr = NULL;

        ret = CAllocator::Malloc(size + 8);

        instrPtr = (unsigned*)ret;
        *instrPtr++ = reinterpret_cast<int&>(size);
        *instrPtr++ = 0xf00ba3;

#ifndef IGC_STANDALONE
        CMemoryReport::MallocMemInstrumentation(size);
#endif

        ret = instrPtr;

        return ret;
    }
    else
    {
        return CAllocator::Malloc(size);
    }
#else
    return CAllocator::Malloc(size);
#endif
}

/*****************************************************************************\

Function:
CAllocator::Deallocate

Description:
Deallocates memory from system memory pool

Input:
void* ptr

Output:
none

\*****************************************************************************/
inline void CAllocator::Deallocate(void* ptr)
{
#if GET_MEM_STATS
    if(IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS))
    {
        if(ptr)
        {
            bool blockValid = true;
            size_t size = 0;

            unsigned* instrPtr = (unsigned*)ptr;

            blockValid = (instrPtr[-1] == 0xf00ba3);
            if(blockValid)
            {
                size = instrPtr[-2];
                ptr = instrPtr - 2;
            }
#ifndef IGC_STANDALONE
            CMemoryReport::FreeMemInstrumentation(size);
#endif
            CAllocator::Free(ptr);
        }
    }
    else
    {
        CAllocator::Free(ptr);
    }
#else
    CAllocator::Free(ptr);
#endif
}

/*****************************************************************************\

Function:
CAllocator::AlignedAllocate

Description:
Allocates aligned memory from system memory pool

Input:
size_t size
size_t alignment

Output:
void*

Notes:

|<-------------------(alloc_size)------------------->|
----------------------------------------------------
|///////|               |                            |
|///////|  SAllocDesc   |<----------(size)---------->|
|///////|               |                            |
----------------------------------------------------
^                       ^
|                       |
alloc_ptr               ptr (aligned)

\*****************************************************************************/
inline void* CAllocator::AlignedAllocate(size_t size, size_t alignment)
{
    void* ptr = NULL;

    // Allocate enough space for the data, the alignment,
    // and storage for the descriptor
    size_t allocSize = size + alignment + sizeof(SAllocDesc);

    void* alloc_ptr = CAllocator::Malloc(allocSize);

    if(alloc_ptr)
    {
        // Ensure there is at least enough space to store the descriptor
        // before performing the alignment
        ptr = (BYTE*)alloc_ptr + sizeof(SAllocDesc);

        // Determine the number of bytes to offset for an aligned pointer
        const size_t offset = (alignment)
            ? alignment - ((size_t)ptr % alignment)
            : 0;

        // Align the pointer
        ptr = (BYTE*)ptr + offset;

        // Store the descriptor for the allocation inside the allocation
        // in the space before the aligned pointer
        SAllocDesc* alloc_desc = (SAllocDesc*)((BYTE*)ptr - sizeof(SAllocDesc));
        alloc_desc->alloc_ptr = alloc_ptr;
    }

    return ptr;
}

/*****************************************************************************\

Function:
CAllocator::AlignedDeallocate

Description:
Deallocates aligned memory from system memory pool

Input:
void* ptr

Output:
none

\*****************************************************************************/
inline void CAllocator::AlignedDeallocate(void* ptr)
{
    if(ptr)
    {
        // Extract the descriptor for the allocation
        SAllocDesc* alloc_desc = (SAllocDesc*)((BYTE*)ptr - sizeof(SAllocDesc));
        void* alloc_ptr = alloc_desc->alloc_ptr;

        CAllocator::Free(alloc_ptr);
    }
}
/*****************************************************************************\

Function:
CAllocator::Malloc

Description:
Abstraction for "malloc"

Input:
size_t size

Output:
void*

\*****************************************************************************/
inline void* CAllocator::Malloc(size_t size)
{
#if ((defined(_DEBUG) || defined(_INTERNAL)) && defined(_WIN32))//private heap for igc windows
    //read regkey for once
    if (ReadingRegkey)
    {
        HKEY uscKey;
        DWORD value = 0;
        LONG success = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\INTEL\\IGFX\\IGC",
            0,
            KEY_READ,
            &uscKey);

        if (ERROR_SUCCESS == success)
        {
            DWORD dwSize = sizeof(value);
            success = RegQueryValueExA(
                uscKey,
                "EnableIGCPrivateHeap",
                NULL,
                NULL,
                (LPBYTE)&value,
                &dwSize);

            RegCloseKey(uscKey);
            PHEnabled = value;
        }
        ReadingRegkey = false;
    }
    //private heap activated
    if (PHEnabled)
    {
        void* mem = NULL;
        if (!g_heap)
        {
            //create heap here
            g_heap = HeapCreate(0, 0, 0);
            IGC_ASSERT_EXIT_MESSAGE(g_heap != NULL, "Could not createglobal heap");
        }
        //allocate memory
        g_alloc_cnt++;
        mem = HeapAlloc(g_heap, 0, size);
        IGC_ASSERT_EXIT_MESSAGE(mem != NULL, "Could not allocate the required memory to Heap");
        return mem;
    }
#endif //end of debug internal

    void* const ptr = malloc(size);

#ifdef _DEBUG
    if (nullptr != ptr)
    {
        std::memset(ptr, 0xcc, size);
    }
#endif
#ifdef _WIN32
    IGC_ASSERT_EXIT_MESSAGE(nullptr != ptr, "Could not allocate the required memory to storage");
#endif
    return ptr;
}
/*****************************************************************************\

Function:
CAllocator::Free

Description:
Abstraction for "free"

Input:
void* ptr

Output:
none

\*****************************************************************************/
inline void CAllocator::Free(void* ptr)
{
#if ((defined(_DEBUG) || defined(_INTERNAL)) && defined(_WIN32)) //private heap malloc for igc windows
    if (PHEnabled)
    {
        if (ptr)
        {
            //free pointer here
            BOOL free_success = HeapFree(g_heap, 0, ptr);
            IGC_ASSERT_EXIT_MESSAGE(free_success, "Could not free require memory from heap");
            g_alloc_cnt--;
        }
        if (g_alloc_cnt <= 0)
        {
            //destroy heap here
            BOOL des_success = HeapDestroy(g_heap);
            g_heap = NULL;
            IGC_ASSERT_EXIT_MESSAGE(des_success, "Could not destroy heap");
        }
        return;
    }
#endif //end of windows and debug internal
    if(ptr)
    {
       free( ptr );
    }
}

#endif //defined( _WIN32 ) || ( defined ( _DEBUG ) || defined ( _INTERNAL ) )

/*****************************************************************************\
locally visible new & delete for Linux
\*****************************************************************************/
#if defined ( _DEBUG ) || defined ( _INTERNAL )
#if defined __GNUC__

#if !defined __clang__
/*
    The clang compiler is relatively strict with regard to warnings.
    This is a particular issue when building with -Werror.

    In the case addressed by this change, clang is being strict about
    redefinition of operators new, new[], delete, and delete[].

    The clang compiler will warn:
    - If a new declaration does not match a previous declaration
      - with respect to inline vs extern
      - with respect to exception specification (e.g., "noexcept")
      - with respect to parameter list (e.g., "std::size_t" vs "size_t").
    Warnings that may be seen are:
    - -Winline-new-delete
    - -Wimplicit-exception-spec-mismatch
    - -Wmicrosoft-exception-spec

    Because of the above, this special debug code (normally built for
    build_type == release-internal or debug) is disabled for clang for now.
    */

// TODO: Throw exception if the allocation fails.
inline void* operator new(size_t size)
{
    void* storage = CAllocator::Allocate(size);
    IGC_ASSERT_EXIT_MESSAGE(nullptr != storage, "Could not allocate the required memory to storage");
    return storage;
}

inline void operator delete(void* ptr) noexcept
{
    CAllocator::Deallocate(ptr);
}

// TODO: Throw exception if the allocation fails.
inline void* operator new[](size_t size)
{
    return ::operator new(size);
}

inline void operator delete[](void* ptr) noexcept
{
    CAllocator::Deallocate(ptr);
}
#endif // !defined __clang__
#endif//defined __GNUC__
#endif //defined ( _DEBUG ) || defined ( _INTERNAL )

#ifndef __GNUC__
#define __NOTAGNUC__
#endif // __GNUC__

#if ( ( defined ( _DEBUG ) || defined ( _INTERNAL ) ) && (defined __NOTAGNUC__ ) )
/*****************************************************************************\
 operator new
\*****************************************************************************/
void* __cdecl operator new( size_t size )
{
    void* storage = CAllocator::Allocate(size);
    IGC_ASSERT_EXIT_MESSAGE(nullptr != storage, "Could not allocate the required memory to storage");
    return storage;
}

/*****************************************************************************\
 operator delete
\*****************************************************************************/
void __cdecl operator delete( void* ptr )
{
    CAllocator::Deallocate( ptr );
}

/*****************************************************************************\
 operator new[]
\*****************************************************************************/
void* __cdecl operator new[]( size_t size )
{
    return ::operator new(size);
}

/*****************************************************************************\
 operator delete[]
\*****************************************************************************/
void __cdecl operator delete[]( void* ptr )
{
    CAllocator::Deallocate( ptr );
}
#endif // ( ( defined ( _DEBUG ) || defined ( _INTERNAL ) ) && (defined __NOTAGNUC__) )

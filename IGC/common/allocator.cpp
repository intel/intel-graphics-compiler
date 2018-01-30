/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#define ThreadCnt
#define He4k

//#define DisableForceInline
//#define OptimizationsOff
//#define EnableAsserts
//#define EnableCMATraces

// Includes for instrumentation
#if defined ( _DEBUG ) || defined ( _INTERNAL )
    #include <stdlib.h>
    #include "IGC/common/igc_debug.h"
    #include "common/Stats.hpp"
#endif

// Includes for CMA
#if defined( _WIN32 )
#include "3d/common/iStdLib/types.h"
#include <new>
#include <cstdint>
//#include <math.h>

#if defined( _WIN32 )
    #include <intrin.h>
#endif

#if defined He4k
    #include <wtypes.h>
    #include <WinBase.h>
#endif

namespace CMA
{

#if defined( _DEBUG ) && defined( _WIN32 )
    #define _MS_CRT_STDMUTEX_WA_
#endif

// minAlign ==
//    8  B for win32
//    16 B for win32 with WA
//
//    16 B for win64 (with and w/o WA)
#if defined( _MS_CRT_STDMUTEX_WA_ ) && !defined( _WIN64 )
    const size_t minAlign = 4 * sizeof( void * );
#else
    const size_t minAlign = 2 * sizeof( void * );
#endif

#if defined( _DEBUG ) && defined( _WIN32 )
    const size_t overlaySize = 2*minAlign;
#else
    const size_t overlaySize = minAlign;
#endif

    const uint32_t chunkSizeThreshold = 1024;
    const uint32_t blockListLenTresh  = 4096;

    const uint32_t cBlockBufSize  = 4 * 1024;


    class spinlock_t;
    class GenericPool;

#if defined He4k
    HANDLE heap1;
#endif


    struct ThreadBlockBufs
    {
        // 4k * 8 B = 32 kB
        char *              Bufs[ blockListLenTresh ];
        // bitset for 4000 has size of 4000 / 8 = 512 B.
        char                BufsBitset[ blockListLenTresh / 8 ];
        uint16_t            BufsCnt = 0;
        uint16_t            BufsStartInd = 0;
        char                BufPaddedBy8Bitset[ blockListLenTresh / 8 ];
    };

    struct SStructThreadLocalPtrs
    {
        spinlock_t *      pSpinlock;
        volatile int      mInitialized;
        GenericPool **    pPools;
        struct ThreadBlockBufs  msThBufs;
    };

    class CustomMA
    {
        // Static class members declarations.
    public:
        __declspec( thread ) static spinlock_t *     mpSpinlock;
    private:
        __declspec( thread ) static int_fast32_t     mInitialized;
        __declspec( thread ) static int_fast32_t     mEnabled;
        __declspec( thread ) static int_fast32_t     mThClosed;
        __declspec( thread ) static GenericPool **   mpPools;
    public:
        __declspec( thread ) static SStructThreadLocalPtrs * mpStructThreadLocalPtrs;
        __declspec( thread ) static uint16_t          mCMAThrdInd;
        
        // Methods.
    public:
        static int       __stdcall   Create( void );
        static void *    __stdcall   CustomAllocate( const size_t argSize );
        static void      __stdcall   CustomDeallocate( void * ptr );
        static void      __stdcall   DLLThreadAttach( void );
        static void      __stdcall   DLLProcessAttach( void );
        static void      __stdcall   DLLThreadDetach( void );
        static void      __stdcall   DLLProcessDetach( void );

#ifdef EnableCMATraces
        static void      __stdcall   CMAPrintStats( void );
#endif

        static void      __stdcall   ReleaseEmptyBlocks( void );
        
    private:
                                CustomMA();

        static void __stdcall   CustomDeallocateCritSect(
            GenericPool ** pChunkPools,
            const unsigned chunkSize,
            const unsigned chunkInd,
            const unsigned char allocType,
            unsigned char * const pChunk,
            bool fSameThDealloc
        );
    };

using namespace std;

// To enable CMA asserts uncomment the line below.
#ifdef EnableAsserts
    #undef CMA_assert
    #define CMA_assert( _Expression ) ( void ) ( ( !!( _Expression ) ) || ( __debugbreak(), 0 ) )
#else
    #undef CMA_assert
    #define CMA_assert( _Expression )     ( ( void ) 0 )
#endif // EnableAsserts


#ifdef OptimizationsOff
#pragma optimize( "", off )
#endif

    class CustomMA;

    #pragma pack( push, 1 )
    
    template < size_t N > struct ChunkWrappterTmplt
    {
        union
        {
            uint16_t  W0;
            struct
            {
                uint16_t  mAllocType    : 2;
                uint16_t  mFlagDefaultAlloctionAlignedBy8       : 1;
                uint16_t  mNotUsed1      : 1;
                uint16_t  mBlockInd     : 12;
            };
        };

        union
        {
            uint16_t  W1;
            struct
            {
                uint16_t  mPoolInd      : 7; // 32bit build has twice as much pools. So one more bit is needed for mPoolInd - in total 7 bits.
                uint16_t  mNotUsed2     : 1;
                uint16_t  mChunkInd     : 8;
            };
        };

        union
        {
            uint16_t W2;
            struct
            {
                uint16_t  mNotUsed3      : 10;
                uint16_t  mThrInd       : 6;
            };
        };

        union
        {
            uint16_t W3;
            struct
            {
                uint16_t  mFreeChunkCnt  : 8;
                uint16_t  mNextFreeChunkInd : 8;
            };
        };

        #if defined( _MS_CRT_STDMUTEX_WA_ )
            uint32_t  mToken;
        #endif
    };

    typedef ChunkWrappterTmplt< overlaySize > ChunkWrapper;

    #pragma pack( pop )

#if defined( _MS_CRT_STDMUTEX_WA_ )
    const uint32_t CMA_token = 0x01127370;
#endif

    const unsigned char Allocator_Custom_Dynamic    = 0x01; // chunk in dynamically allocated 4kB block
    const unsigned char Allocator_Default           = 0x03; // regular_malloc

#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
    // size must be multiplicity of minAlign
    unsigned int getPoolIndex( unsigned int size )
    {
        CMA_assert( size >= minAlign );
        CMA_assert( size <= chunkSizeThreshold );
        CMA_assert( ( size % minAlign ) == 0 );
        
        unsigned int ind = ( size / minAlign ) - 1;
        
        CMA_assert( ind < ( cBlockBufSize / minAlign ) );

        return ind;
    }

    unsigned int getSizeFromPoolInd( unsigned int ind )
    {
        CMA_assert( ind < ( cBlockBufSize / minAlign ) );
        
        unsigned int size = ( ind + 1 ) * minAlign;

        CMA_assert( size >= minAlign );
        CMA_assert( size <= chunkSizeThreshold );

        return size;
    }

    template < size_t  N > inline unsigned int getReservedChunks( const unsigned int size )
    {
        CMA_assert( 0 );
    }


    template <>
#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
    unsigned int getReservedChunks< 8 >( const unsigned int size )
    {
        // bufBlock is 4096 B
        // actual min size is 16B, so ther is 256 slots in buf;
        // 256 bitset takes 32 B, which is 2 elems;

        CMA_assert( ( size % 8 ) == 0 );

        unsigned int val = 0;

        if ( size >= 32 )
        {
            val = 1;
        }
        else
        {
            val = 2;
        }

        return val;
    }

    template <>
#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
    unsigned int getReservedChunks< 16 >( const unsigned int size )
    {
        CMA_assert( ( size % 16 ) == 0 );

        // bufBlock is 4096 B
        // actual min size is 32B, so ther is 128 slots in buf;
        // 128 bitset takes 16 B, which is less than 1 elem size;

        return 1;
    }
    
    inline int_fast16_t getTotalChunks16( const int_fast16_t size )
    {
        CMA_assert( ( size % minAlign ) == 0 );
        const float divisor = ( float ) cBlockBufSize;

        return ( int_fast16_t ) ( divisor  / ( float ) size );
    }

    inline unsigned int getTotalChunks( const unsigned int size )
    {
        CMA_assert( ( size % minAlign ) == 0 );
        return cBlockBufSize / size;
    }

    void inline bitSet( char * const pBitSet, unsigned int pos )
    {
        unsigned int dWord = pos / 32;
        unsigned int bit   = pos % 32;

        uint32_t * ptr = ( uint32_t * ) pBitSet;
        ptr[ dWord ] |= ( ( ( uint32_t ) 1 ) << bit );
    }

    void inline bitClear( char * const pBitSet, unsigned int pos )
    {
        unsigned int dWord = pos / 32;
        unsigned int bit   = pos % 32;

        uint32_t * ptr = ( uint32_t * ) pBitSet;
        ptr[ dWord ] &= ~( ( ( uint32_t ) 1 ) << bit );
    }

    uint32_t inline bitClearAndReturnDword( char * const pBitSet, unsigned int pos )
    {
        unsigned int dWord = pos / 32;
        unsigned int bit   = pos % 32;

        uint32_t * ptr = ( uint32_t * ) pBitSet;
        ptr[ dWord ] &= ~( ( ( uint32_t ) 1 ) << bit );
        return ptr[ dWord ];
    }

    int inline bitIsSet( char * const pBitSet, unsigned int pos )
    {
        unsigned int dWord = pos / 32;
        unsigned int bit   = pos % 32;

        uint32_t * ptr = ( uint32_t * ) pBitSet;
        return ( ( ptr[ dWord ] & ( ( ( uint32_t ) 1 ) << bit ) ) > 0 );
    }

    // Bit scan forward.
    // Seek first set bit (1).
#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
    DWORD bitScanForward( const DWORD mask )
    {
    #ifdef _WIN32
        DWORD index;
        _BitScanForward( &index, mask );
        return index;
    #elif  __linux__ 
        return static_cast<unsigned int>( __builtin_ffsl( mask ) - 1 );
    #else
        DWORD bit = 0;
        if( mask != 0 )
        {
            while( ( ( mask & 1 << bit) ) == 0 ) && bit < 32 )
            {
                ++bit;
            }
        }
        return bit;
    #endif
    }

    int bitIsAnySet( char * const pBitSet, const uint32_t startInd, const uint32_t endInd  )
    {
        int               found          = 0;

        const uint32_t    startDWord     = startInd / 32;
        const uint32_t    startBit       = startInd % 32;

        const uint32_t    endDWord       = endInd / 32;
        const uint32_t    endBit         = endInd % 32;

        uint32_t * const  ptr            = ( uint32_t * ) pBitSet;

        for( uint32_t currDWord = 0; currDWord <= endDWord; ++currDWord )
        {
            uint32_t mask = 0;
            uint32_t val = ptr[ currDWord ];

            if( val == 0 )
            {
                continue;
            }

            if( currDWord == startDWord )
            {
                mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
            }

            if( currDWord == endDWord )
            {
                mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
            }

            val = val & ~mask;

            if( val )
            {
                found = 1;
                break;
            }
        }
        
        return found;
    }

#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
    int bitGetFirstZeroBitPosIfAvailInline( char * const pBitSet, const uint32_t startInd, const uint32_t endInd, uint32_t & foundInd )
    {
        CMA_assert( startInd <= endInd );

        uint32_t * const ptr = ( uint32_t * ) pBitSet;

        int found = 0;
        uint32_t result = endInd;

        uint32_t startDWord     = startInd / 32;
        uint32_t startBit       = startInd % 32;

        uint32_t endDWord       = endInd / 32;
        uint32_t endBit         = endInd % 32;

        for( uint32_t currDWord = startDWord; currDWord <= endDWord; ++currDWord )
        {
            uint32_t mask = 0;
            uint32_t val = ptr[ currDWord ];

            if( val == 0xFFFFFFFF )
            {
                continue;
            }

            if( currDWord == startDWord )
            {
                mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
            }

            if( currDWord == endDWord )
            {
                mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
            }

            val |= mask;

            if( ~val )
            {
                result = ( uint32_t ) bitScanForward( ( DWORD ) ( ~val ) ) + 32 * currDWord;
                found = 1;
                break;
            }
        }

        if( !found && startInd > 0 )
        {
            endDWord = startInd / 32;
            endBit = startInd % 32;

            startDWord = 0;
            startBit = 0;

            for( uint32_t currDWord = startDWord; currDWord <= endDWord; ++currDWord )
            {
                uint32_t mask = 0;
                uint32_t val = ptr[ currDWord ];

                if( val == 0xFFFFFFFF )
                {
                    continue;
                }

                if( currDWord == startDWord )
                {
                    mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
                }

                if( currDWord == endDWord )
                {
                    mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
                }

                val |= mask;

                if( ~val )
                {
                    result = ( uint32_t ) bitScanForward( ( DWORD ) ( ~val ) ) + 32 * currDWord;
                    found = 1;
                    break;
                }
            }
        }

        if( found )
        {
            foundInd = result;
        }

        return found;
    }

    int bitGetFirstSetBitPosIfAvail( char * const pBitSet, const uint32_t startInd, const uint32_t endInd, uint32_t & foundInd )
    {
        CMA_assert( startInd <= endInd );
        uint32_t * const ptr = ( uint32_t * ) pBitSet;

        int found = 0;
        uint32_t result = endInd;

        uint32_t startDWord     = startInd / 32;
        uint32_t startBit       = startInd % 32;

        uint32_t endDWord       = endInd / 32;
        uint32_t endBit         = endInd % 32;

        for( uint32_t currDWord = startDWord; currDWord <= endDWord; ++currDWord )
        {
            uint32_t mask = 0;
            uint32_t val = ptr[ currDWord ];

            if( val == 0 )
            {
                continue;
            }

            if( currDWord == startDWord )
            {
                mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
            }

            if( currDWord == endDWord )
            {
                mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
            }

            val &= ~mask;

            if( val )
            {
                result = ( uint32_t ) bitScanForward( ( DWORD ) ( val ) ) + 32 * currDWord;
                found = 1;
                break;
            }
        }

        if( found )
        {
            foundInd = result;
        }

        return found;
    }

    int bitGetFirstZeroBitPosIfAvail( char * const pBitSet, const uint32_t startInd, const uint32_t endInd, uint32_t & foundInd )
    {
        CMA_assert( startInd <= endInd );

        uint32_t * const ptr = ( uint32_t * ) pBitSet;

        int found = 0;
        uint32_t result = endInd;

        uint32_t startDWord     = startInd / 32;
        uint32_t startBit       = startInd % 32;

        uint32_t endDWord       = endInd / 32;
        uint32_t endBit         = endInd % 32;

        for( uint32_t currDWord = startDWord; currDWord <= endDWord; ++currDWord )
        {
            uint32_t mask = 0;
            uint32_t val = ptr[ currDWord ];

            if( val == 0xFFFFFFFF )
            {
                continue;
            }

            if( currDWord == startDWord )
            {
                mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
            }

            if( currDWord == endDWord )
            {
                mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
            }

            val |= mask;

            if( ~val )
            {
                result = ( uint32_t ) bitScanForward( ( DWORD ) ( ~val ) ) + 32 * currDWord;
                found = 1;
                break;
            }
        }

        if( !found && startInd > 0 )
        {
            endDWord = startInd / 32;
            endBit = startInd % 32;

            startDWord = 0;
            startBit = 0;

            for( uint32_t currDWord = startDWord; currDWord <= endDWord; ++currDWord )
            {
                uint32_t mask = 0;
                uint32_t val = ptr[ currDWord ];

                if( val == 0xFFFFFFFF )
                {
                    continue;
                }

                if( currDWord == startDWord )
                {
                    mask |= ( ( ( ( uint32_t ) 1 ) << startBit ) - 1 );
                }

                if( currDWord == endDWord )
                {
                    mask |= ( ( ( uint32_t ) 0xFFFFFFFE ) << endBit );
                }

                val |= mask;

                if( ~val )
                {
                    result = ( uint32_t ) bitScanForward( ( DWORD ) ( ~val ) ) + 32 * currDWord;
                    found = 1;
                    break;
                }
            }
        }

        if( found )
        {
            foundInd = result;
        }

        return found;
    }

    /*****************************************************************************\
    Description:
        Fast spinlock.
        Read more at: http://software.intel.com/en-us/articles/implementing-scalable-atomic-locks-for-multi-core-intel-em64t-and-ia32-architectures
    \*****************************************************************************/
    class spinlock_t {
    public:
        spinlock_t() : m_lock( 0 ) {}
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
        void lock() {
            while(
    #if defined _WIN32
                    0 != _interlockedbittestandset( &m_lock, 0 ) )
    #else
                    __sync_lock_test_and_set( &m_lock, 1 ) )
    #endif
            {
                for(;;) 
                {
                    if(m_lock==0) break;
                    if(m_lock==0) break;
                    if(m_lock==0) break;
                    if(m_lock==0) break;
                }
            }
        }

    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
        void unlock()
        {
            m_lock=0;
        }

    private:
        volatile long m_lock;

    };


    /*****************************************************************************\
    Description:
        RAII wrapper for locking/unlocking spinlock
    \*****************************************************************************/
    class lock_t {
    public:
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
        lock_t( spinlock_t &spinlock )
        : m_spinlock(spinlock)
        {
            m_spinlock.lock();
        }

    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
        ~lock_t()
        {
            m_spinlock.unlock();
        }

    private:
        spinlock_t& m_spinlock;
        lock_t &operator= ( const lock_t& );
    };


    // Globals.

#if defined ThreadCnt
    const uint16_t cMaxThds = 64;

    uint16_t glThCnt = 0;
    uint16_t glMaxThCnt = 0;
    SStructThreadLocalPtrs * ttlp[ cMaxThds ] = { NULL };
    SStructThreadLocalPtrs StructTLPPlaceholder;
#endif


#if defined ThreadCnt
    spinlock_t      GlobalSpinlock;

#ifdef EnableCMATraces
    int_fast32_t    mPrintEnable;
#endif

#endif

    // Static class members definitions.
    __declspec( thread ) spinlock_t *                CustomMA::mpSpinlock;
    __declspec( thread ) int_fast32_t                CustomMA::mInitialized;
    __declspec( thread ) int_fast32_t                CustomMA::mEnabled;
    __declspec( thread ) int_fast32_t                CustomMA::mThClosed;
    __declspec( thread ) GenericPool **              CustomMA::mpPools;
    __declspec( thread ) SStructThreadLocalPtrs *    CustomMA::mpStructThreadLocalPtrs;
    __declspec( thread ) uint16_t                    CustomMA::mCMAThrdInd;
    

#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
void * CMAInternalMalloc( size_t alSize )
{
    if( alSize == cBlockBufSize ||  alSize == cBlockBufSize + 8 )
    {
#if defined He4k
		CMA_assert( heap1 != NULL );
#endif

        void * lptr = 
#if defined He4k
            ( char * ) HeapAlloc( heap1, 0, alSize );
#else
            malloc( alSize );
#endif

        return lptr;
    }
    else
    {
        void * lptr = malloc( alSize );

        return lptr;
    }
}

#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
void CMAInternalFree( void * ptr, size_t alSize )
{
    if( alSize == cBlockBufSize ||  alSize == cBlockBufSize + 8 )
    {
#if defined He4k
		CMA_assert( heap1 != NULL );

        HeapFree( heap1, 0, ptr );
#else
        free( ptr );
#endif
    }
    else
    {
        free( ptr );
    }
 
    return;
}

    class GenericPool
    {
    public:
#if defined EnableAsserts
        char                mPoolBlockBufsBitset[ blockListLenTresh / 8 ];
#endif
        char                mPoolNonFullBlockBufsBitset[ blockListLenTresh / 8 ];
        uint16_t            mNonFullBlCnt;
        uint16_t            mPoolBlCnt;
        uint16_t            mNonFullBlStartInd;
        char *              mpNonFullBlPtr;
        uint16_t            mPoolNextFreeChunkIndInNonFullBl;
        void *              mpPoolNextFreeChunkPtrInNonFullBl;

        const unsigned int  mElementSize;
        const uint16_t      mPoolBlockTotalChunks;
        struct ThreadBlockBufs * psThBufs;

    public:
        inline              GenericPool( unsigned int size, struct ThreadBlockBufs * psArgThBufs );
        void *              Allocate();


#ifndef EnableAsserts
    #if defined _WIN32
        __forceinline
    #else
        inline
    #endif
#endif
        void * getChunkPtrFromInd( unsigned ind, char * pBufBlock )
        {
            void * result = NULL;

            CMA_assert( ind < mPoolBlockTotalChunks );
            CMA_assert( pBufBlock != NULL );

            result = ( void * ) ( ( char * ) pBufBlock + ind * mElementSize );

            return result;
        }
    };

#ifdef EnableCMATraces
//#include <stdio.h>
#endif

    void CustomMA::DLLProcessAttach()
    {
        mEnabled = 1;

#ifdef EnableCMATraces
        if( mPrintEnable )
        {
            printf(">> CMA: DLLProcessAttach, ");
            CMAPrintStats();
            printf("\n");
        }
#endif

    }

    void CustomMA::DLLThreadAttach()
    {
        mEnabled = 1;

#ifdef EnableCMATraces
        if( mPrintEnable )
        {
            printf(">> CMA: DLLThreadAttach, ");
            CMAPrintStats();
            printf("\n");
        }
#endif

    }

    void CustomMA::DLLThreadDetach()
    {
        mEnabled = 0;
        mThClosed = 1;

#ifdef EnableCMATraces
        if( mPrintEnable )
        {
            printf(">> CMA: DLLThreadDetach, ");
            CMAPrintStats();
            printf("\n");
            printf("ReleaseEmptyBlocks() ...");
            ReleaseEmptyBlocks();
            printf(" done.\n");
            CMAPrintStats();
            printf("\n");
        }
#endif

        ReleaseEmptyBlocks();
    }

    void CustomMA::DLLProcessDetach()
    {
        mEnabled = 0;
        mThClosed = 1;

#ifdef EnableCMATraces
        if( mPrintEnable )
        {
            printf(">> CMA: DLLProcessDetach, ");
            CMAPrintStats();
            printf("\n");
            printf("ReleaseEmptyBlocks() ...");
            ReleaseEmptyBlocks();
            printf(" done.\n");
            CMAPrintStats();
            printf("\n");
        }
#endif

        ReleaseEmptyBlocks();
    }

#ifdef EnableCMATraces
    void CustomMA::CMAPrintStats()
    {
        if( CustomMA::mpStructThreadLocalPtrs )
        {
            printf("blkCnt %u", CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsCnt );
        }
        
        for( int ti = 0; ti < cMaxThds; ++ti )
        {
            if( ttlp[ ti ] )
            {
                printf("\n");
                printf("th%d: blkCnt %u, ", ti, ttlp[ ti ]->msThBufs.BufsCnt );
                
                uint16_t emptyBlkCnt = 0;
                uint16_t nonFullBlCnt = 0;
                uint32_t NFBChunkCnt = 0;

                if( ttlp[ ti ]->pPools )
                {
                    for( uint16_t pi = 0; pi < 4096 / minAlign; ++pi )
                    {
                        GenericPool * pool = ttlp[ ti ]->pPools[ pi ];
                        if( pool )
                        {
                            uint32_t bufInd = 0;
                            nonFullBlCnt += pool->mNonFullBlCnt;

                            uint32_t startInd = 0;
                            while( startInd < blockListLenTresh )
                            {
                                if( bitGetFirstSetBitPosIfAvail( ( char * ) pool->mPoolNonFullBlockBufsBitset, startInd, blockListLenTresh - 1, bufInd ) )
                                {
                                    CMA_assert( bufInd < blockListLenTresh );
                                    CMA_assert( bitIsSet( pool->mPoolBlockBufsBitset, bufInd ) );
                                    CMA_assert( bitIsSet( ttlp[ ti ]->msThBufs.BufsBitset, bufInd ) );

                                    char * pBuf = ttlp[ ti ]->msThBufs.Bufs[ bufInd ];
                                    ChunkWrapper * pFirstChunk = ( ChunkWrapper * ) ( pBuf + getReservedChunks< minAlign >( pool->mElementSize ) * pool->mElementSize );
                                
                                    uint16_t usedChunkCnt = pool->mPoolBlockTotalChunks - getReservedChunks< minAlign >( pool->mElementSize ) - pFirstChunk->mFreeChunkCnt;

                                    if( 0 == usedChunkCnt )
                                    {
                                        ++emptyBlkCnt;
                                    }
                                    else
                                    {
                                        NFBChunkCnt += usedChunkCnt;
                                    }

                                    startInd = bufInd + 1;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            
                printf(", empty blks %u, NFB %u, NFBChunkCnt %u", emptyBlkCnt, nonFullBlCnt, NFBChunkCnt );
            }
        }

        printf("\n");
    }
#endif

    void CustomMA::ReleaseEmptyBlocks()
    {
        uint32_t releaseSpinLock = 0;

	    if( CustomMA::mpSpinlock )
        {
            CMA_assert( CustomMA::mInitialized );
		    CMA_assert( CustomMA::mpSpinlock );
								
		    lock_t lock( *CustomMA::mpSpinlock );

            if( CustomMA::mpStructThreadLocalPtrs )
            {
                GenericPool ** pChunkPools = CustomMA::mpStructThreadLocalPtrs->pPools;

                uint32_t allPoolsReleased = 1;

                // iterate pools
                for( uint16_t pi = 0; pi < 4096 / minAlign; ++pi )
                {
                    GenericPool * pPool = pChunkPools[ pi ];
                    if( pPool )
                    {
                        uint32_t bufInd = 0;
                        uint32_t startInd = 0;
                        while( startInd < blockListLenTresh )
                        {
                            if( bitGetFirstSetBitPosIfAvail( ( char * ) pPool->mPoolNonFullBlockBufsBitset, startInd, blockListLenTresh - 1, bufInd ) )
                            {
                                CMA_assert( bufInd < blockListLenTresh );
                                CMA_assert( bitIsSet( pPool->mPoolBlockBufsBitset, bufInd ) );
                                CMA_assert( bitIsSet( ttlp[ mCMAThrdInd ]->msThBufs.BufsBitset, bufInd ) );

                                char * pBuf = CustomMA::mpStructThreadLocalPtrs->msThBufs.Bufs[ bufInd ];
                                ChunkWrapper * pFirstChunk = ( ChunkWrapper * ) ( pBuf + getReservedChunks< minAlign >( pPool->mElementSize ) * pPool->mElementSize );
                                
                                uint16_t usedChunkCnt = pPool->mPoolBlockTotalChunks - getReservedChunks< minAlign >( pPool->mElementSize ) - pFirstChunk->mFreeChunkCnt;

                                if( 0 == usedChunkCnt )
                                {
                                    // release block
                                    char *          pBitSet         = pBuf;

                                    bitClear( pPool->mPoolNonFullBlockBufsBitset, bufInd );
                                    --( pPool->mNonFullBlCnt );
                                    --( pPool->mPoolBlCnt );

                                    if( pPool->mpNonFullBlPtr && pPool->mNonFullBlStartInd == bufInd )
                                    {
                                        CMA_assert( pPool->mpNonFullBlPtr == pBitSet );
                                        pPool->mNonFullBlStartInd = 0;
                                        pPool->mpNonFullBlPtr = NULL;
                                        pPool->mpPoolNextFreeChunkPtrInNonFullBl = NULL;
                                        pPool->mPoolNextFreeChunkIndInNonFullBl = 0;
                                    }

                #if defined EnableAsserts
                                    bitClear( pPool->mPoolBlockBufsBitset, bufInd );
                #endif

                                    if( bitIsSet( CustomMA::mpStructThreadLocalPtrs->msThBufs.BufPaddedBy8Bitset, bufInd ) )
                                    {
                                        CMAInternalFree( pBitSet - 8, cBlockBufSize + 8 );
                                    }
                                    else
                                    {
                                        CMAInternalFree( pBitSet, cBlockBufSize );
                                    }

                                    // mark it as UNused
                                    bitClear( CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsBitset, bufInd );

                                    if( bufInd < CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsStartInd )
                                    {
                                        CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsStartInd = (uint16_t)bufInd;
                                    }

                                    --CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsCnt;
                                    CustomMA::mpStructThreadLocalPtrs->msThBufs.Bufs[ bufInd ] = NULL;
                                }

                                startInd = bufInd + 1;
                            }
                            else
                            {
                                break;
                            }
                        } // NFB iter

                        if( pPool->mPoolBlCnt == 0 )
                        {
                            mpPools[ pi ] = NULL;
                            pPool->~GenericPool();
                            CMAInternalFree( pPool, sizeof( GenericPool ) );
                            pPool = NULL;
                        }
                        else
                        {
                            allPoolsReleased = 0;
                        }
                    }
                } // iterate pools

                // all pools released
                if( allPoolsReleased )
                {
                    CMA_assert( 0 == CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsCnt );

                    if( 0 == CustomMA::mpStructThreadLocalPtrs->msThBufs.BufsCnt )
                    {
                        CMA_assert( mpStructThreadLocalPtrs == ttlp[ mCMAThrdInd ] && mpStructThreadLocalPtrs );
                        CMA_assert( mpStructThreadLocalPtrs->pPools     == mpPools );
                        CMA_assert( mpStructThreadLocalPtrs->pSpinlock  == mpSpinlock );

                        // release mpStructTLP
                        mpStructThreadLocalPtrs->~SStructThreadLocalPtrs();
                        CMAInternalFree( mpStructThreadLocalPtrs, sizeof ( SStructThreadLocalPtrs ) );
                        ttlp[ mCMAThrdInd ] = &StructTLPPlaceholder;
                        mpStructThreadLocalPtrs = &StructTLPPlaceholder;

                        CMA_assert( mpPools );

                        if( mpPools )
                        {
                            // release mpPools
                            CMAInternalFree ( mpPools, ( 4096 / minAlign ) * sizeof( void * ) );
                            mpPools = NULL;

                            // set other thread_local variables
                            /// ...
                            mCMAThrdInd = 0;    // stored in mpSTLP and TL;
                            mEnabled = 0;       // stored in mpSTLP and TL;

                            releaseSpinLock = 1;

                            /// mThClosed = 1;   /// effectively not used; stored only in thread_local
                            /// mInitialized     /// effectively not used; in mpSTLP and in thread_local;
                        }
                    } // release mpSTLP, mpPools
                }
            } // if mpSTLP
        } // if mpSipnLock

        // release mpSpinlock
        if( releaseSpinLock && mpSpinlock )
        {
            // mpSpinLock is
            //
            mpSpinlock->~spinlock_t();
            CMAInternalFree( mpSpinlock, sizeof ( spinlock_t ) );
            mpSpinlock = NULL;
        }

    } // ReleaseEmptyBlocks

#ifdef OptimizationsOff
#pragma optimize( "", on )
#endif

#ifdef OptimizationsOff
#pragma optimize( "", off )
#endif

    // returns 0 if succeed, otherwise returns 1
    int CustomMA::Create( void )
    {
        void * ptr = NULL;

        CMA_assert( overlaySize >= sizeof( ChunkWrapper ) );

        mpPools = ( GenericPool ** ) CMAInternalMalloc( ( 4096 / minAlign ) * sizeof( void * ) );
        if( mpPools )
        {
            memset( mpPools, 0, ( 4096 / minAlign ) * sizeof( void * )  );

            mpStructThreadLocalPtrs = ( SStructThreadLocalPtrs * ) CMAInternalMalloc( sizeof ( SStructThreadLocalPtrs ) );
            if( mpStructThreadLocalPtrs )
            {
                ptr = CMAInternalMalloc( sizeof ( spinlock_t ) );
                
                if( ptr )
                {
                    mpSpinlock = new ( ptr ) spinlock_t();

                    if( mpSpinlock )
                    {
                        mInitialized = 1;

                        mpStructThreadLocalPtrs->mInitialized = mInitialized;
                        mpStructThreadLocalPtrs->pPools = mpPools;
                        mpStructThreadLocalPtrs->pSpinlock = mpSpinlock;

                        memset( mpStructThreadLocalPtrs->msThBufs.BufsBitset, 0, blockListLenTresh / 8 );
                        memset( mpStructThreadLocalPtrs->msThBufs.BufPaddedBy8Bitset, 0, blockListLenTresh / 8 );

                        mpStructThreadLocalPtrs->msThBufs.BufsCnt = 0;
                        mpStructThreadLocalPtrs->msThBufs.BufsStartInd = 0;

#if defined ThreadCnt
                        lock_t lock( GlobalSpinlock );
#endif

#ifdef He4k
						if (!heap1)
						{
							heap1 = HeapCreate(0, 23 * 1024, 0);
						}
#endif

#if defined ThreadCnt
                        ttlp[ mCMAThrdInd ] = mpStructThreadLocalPtrs;

#endif

                    }
                }
            }
        }

        CMA_assert( ptr );
        CMA_assert( mpSpinlock );
        CMA_assert( mpStructThreadLocalPtrs );
        CMA_assert( mpPools );

        if( ptr && mpSpinlock && mpPools && mpStructThreadLocalPtrs )
        {

#ifdef EnableCMATraces
    #if defined ( _DEBUG ) || defined ( _INTERNAL )
                if( IGC_IS_FLAG_ENABLED( ShaderDumpPidDisable ) )
                //if( IGC_IS_FLAG_ENABLED( EnablaCMATraces ) )
                {
                    mPrintEnable = 1;
                }
    #endif
#endif

            return 0;
        }
        else
        {
        // cleanup if creation failed
#if defined ThreadCnt
            {
                lock_t lock( GlobalSpinlock );

                if( glThCnt > 0 )
                {
                    --glThCnt;
                }
                
                ttlp[ mCMAThrdInd ] = NULL;
                mCMAThrdInd = 0;
                mpStructThreadLocalPtrs = NULL;
                mpPools = NULL;
                mpSpinlock = NULL;
                mInitialized = 0;
            }
#endif

            return 1;
        }
    }

    GenericPool::GenericPool( unsigned int size, struct ThreadBlockBufs * psArgThBufs ):
        mNonFullBlCnt( 0 ), mPoolBlCnt( 0 ), mNonFullBlStartInd( 0 ), mpNonFullBlPtr( NULL ),
        mPoolNextFreeChunkIndInNonFullBl( 0 ),
        mpPoolNextFreeChunkPtrInNonFullBl( NULL ),
        mElementSize( size ),
        mPoolBlockTotalChunks( (const uint16_t)getTotalChunks16( size ) ),
        psThBufs( psArgThBufs )
    {
        CMA_assert( size );

        memset( mPoolNonFullBlockBufsBitset, 0, blockListLenTresh / 8 );

#if defined EnableAsserts
        memset( mPoolBlockBufsBitset, 0, blockListLenTresh / 8 );
#endif


    }

#ifndef DisableForceInline
    #ifndef EnableAsserts
        #if defined _WIN32
            __forceinline
        #else
            inline
        #endif
    #endif
#endif
    void * GenericPool::Allocate()
    {
        void *          pChunk          = NULL;
        uint16_t        argInd          = 0;
        uint32_t        bufInd          = 65535;
        char *          pBuf            = NULL;
                        
        size_t const    allocSize       = mElementSize;
        const uint16_t  cPoolInd        = (const uint16_t)getPoolIndex( this->mElementSize );
        GenericPool *   pPool           = this;

        const unsigned  reservedChunks  = getReservedChunks< minAlign >( allocSize );
        ChunkWrapper *  pFirstChunk     = NULL;

        CMA_assert( ( ( reservedChunks * allocSize ) % 4 ) == 0 );


        if( pPool->mpNonFullBlPtr && bitIsSet( pPool->mPoolNonFullBlockBufsBitset, pPool->mNonFullBlStartInd ) )
        {
            bufInd = pPool->mNonFullBlStartInd;
            pBuf = pPool->mpNonFullBlPtr;
            pFirstChunk = ( ChunkWrapper * ) ( pBuf + reservedChunks * allocSize );
        }
        else
        {
            // Get Non-Full Block Buffer
            bufInd = 65535;

            if( pPool->mNonFullBlCnt > 0 
                && bitGetFirstSetBitPosIfAvail( ( char * ) pPool->mPoolNonFullBlockBufsBitset, pPool->mNonFullBlStartInd, blockListLenTresh - 1, bufInd ) )
            {


                CMA_assert( bufInd < blockListLenTresh );
                CMA_assert( bitIsSet( pPool->mPoolBlockBufsBitset, bufInd ) );
                CMA_assert( bitIsSet( psThBufs->BufsBitset, bufInd ) );

                pBuf = psThBufs->Bufs[ bufInd ];
            
                if( pPool->mpNonFullBlPtr != pBuf )
                {
                    pPool->mpPoolNextFreeChunkPtrInNonFullBl = NULL;
                    pPool->mPoolNextFreeChunkIndInNonFullBl = 0;
                }

                if( bufInd > pPool->mNonFullBlStartInd || pPool->mpNonFullBlPtr == NULL )
                {
                    pPool->mNonFullBlStartInd = (uint16_t)bufInd;
                    pPool->mpNonFullBlPtr = pBuf;
                }

                pFirstChunk = ( ChunkWrapper * ) ( pBuf + reservedChunks * allocSize );
            }
            else
            // If there are no such buffs, allocate new one.
            {
                // allocate
                pBuf = ( char * ) CMAInternalMalloc( cBlockBufSize );
                bool bufPaddedBy8 = false;

                if( pBuf )
                {
                    if( 16 == minAlign )
                    {
                        uint32_t dynBlockAlign = ( uint32_t ) ( ( size_t ) pBuf % minAlign );
                        CMA_assert( dynBlockAlign == 0 || dynBlockAlign == 8 );
                        if( dynBlockAlign == 8 )
                        {
                            // free
                            CMAInternalFree( pBuf, cBlockBufSize );
                            pBuf = NULL;

                            // realloc
                            pBuf = ( char * ) CMAInternalMalloc( cBlockBufSize + 8 );
                            dynBlockAlign = ( uint32_t ) ( ( size_t ) pBuf % minAlign );
                            CMA_assert( dynBlockAlign == 0 || dynBlockAlign == 8 );

                            if( dynBlockAlign == 8 )
                            {
                                bufPaddedBy8 = true;
                                pBuf += 8;
                            }
                        } //  pBuf == 8
                    }

                    memset( pBuf, 0, reservedChunks * allocSize );

                    for( unsigned int i = 0; i < reservedChunks; ++i )
                    {
                        bitSet( pBuf, i );
                    }

                    pFirstChunk = ( ChunkWrapper * ) ( pBuf + reservedChunks * allocSize );
                    pFirstChunk->mFreeChunkCnt = pPool->mPoolBlockTotalChunks - reservedChunks;
                    pFirstChunk->mNextFreeChunkInd = 0;

                    // Get empty buffer index.
                    {
                        uint32_t ind = 0;
        
                        if( !bitIsSet( ( char * ) psThBufs->BufsBitset, 0 ) )
                        {
                            bufInd = 0;
                        }
                        else if( !bitIsSet( ( char * ) psThBufs->BufsBitset, psThBufs->BufsStartInd ) )
                        {
                            bufInd = psThBufs->BufsStartInd;
                        }
                        else if( bitGetFirstZeroBitPosIfAvail( ( char * ) psThBufs->BufsBitset, 0, ( blockListLenTresh ) - 1, ind ) )
                        {
                            bufInd = ind;
                        }
                        else
                        {
                            CMA_assert( 0 && "GetEmptyGlobalBufInd failed. No avail empty block slot." );
                        }

                        if( bufInd + 1 < blockListLenTresh )
                        {
                            psThBufs->BufsStartInd = bufInd + 1;
                        }

                        CMA_assert( bufInd < blockListLenTresh );

                        // mark it as used
                        bitSet( psThBufs->BufsBitset, bufInd );
                        ++psThBufs->BufsCnt;

                        CMA_assert( psThBufs->BufsCnt < blockListLenTresh + 1 );

                        if( bufPaddedBy8 )
                        {
                            bitSet( psThBufs->BufPaddedBy8Bitset, bufInd );
                        }
                        else
                        {
                            bitClear( psThBufs->BufPaddedBy8Bitset, bufInd );
                        }
                        psThBufs->Bufs[ bufInd ] = pBuf;
                    }

                    pFirstChunk->mThrInd          = CustomMA::mCMAThrdInd;
                    pFirstChunk->mBlockInd        = bufInd;

        #if defined EnableAsserts
                    // mark it as used in PoolBlockBufsBitset
                    bitSet( pPool->mPoolBlockBufsBitset, bufInd );
        #endif

                    // Mark it as Non-Full.
                    bitSet( pPool->mPoolNonFullBlockBufsBitset, bufInd );
                    ++pPool->mNonFullBlCnt;
                    ++pPool->mPoolBlCnt;
                    
                    if( pPool->mpNonFullBlPtr != pBuf )
                    {
                        pPool->mpPoolNextFreeChunkPtrInNonFullBl = NULL;
                        pPool->mPoolNextFreeChunkIndInNonFullBl = 0;
                    }

                    if( bufInd <= pPool->mNonFullBlStartInd || pPool->mpNonFullBlPtr == NULL )
                    {
                        pPool->mNonFullBlStartInd = (uint16_t)bufInd;
                        pPool->mpNonFullBlPtr = pBuf;
                    }
                } // if( pBuf )
            }
        }

        if( pBuf )
        {
            uint32_t chunkInd = 0;

            // Check in Pool if there is info saved about next free chunk.
            if( pPool->mpPoolNextFreeChunkPtrInNonFullBl )
            {
                const uint16_t savedNextFreeChunkInd = pPool->mPoolNextFreeChunkIndInNonFullBl;
                void * const pSavedNextFreeChunkPtr = pPool->mpPoolNextFreeChunkPtrInNonFullBl;

                CMA_assert( pPool->mpNonFullBlPtr == pBuf );
                CMA_assert( pPool->mpPoolNextFreeChunkPtrInNonFullBl <= pBuf + cBlockBufSize - mElementSize );
                CMA_assert( pPool->mPoolNextFreeChunkIndInNonFullBl == pFirstChunk->mNextFreeChunkInd );
                CMA_assert( pPool->getChunkPtrFromInd( savedNextFreeChunkInd, pBuf ) == pSavedNextFreeChunkPtr );
                CMA_assert( pFirstChunk->mFreeChunkCnt );
                CMA_assert( !bitIsSet( pBuf, pFirstChunk->mNextFreeChunkInd ) );

                pChunk = pSavedNextFreeChunkPtr;
                bitSet( ( char * ) pBuf, savedNextFreeChunkInd );
                argInd = savedNextFreeChunkInd;

                --pFirstChunk->mFreeChunkCnt;
            }
            else if( pFirstChunk->mNextFreeChunkInd > 0 )
            {
                const uint16_t savedNextFreeChunkInd = pFirstChunk->mNextFreeChunkInd;
                void * const pFreeChunkPtr = pPool->getChunkPtrFromInd( savedNextFreeChunkInd, pBuf );

                CMA_assert( !bitIsSet( pBuf, savedNextFreeChunkInd ) );

                pChunk = pFreeChunkPtr;
                bitSet( ( char * ) pBuf, savedNextFreeChunkInd );
                argInd = savedNextFreeChunkInd;

                --pFirstChunk->mFreeChunkCnt;
            }
            else
            {
                // Get first zero bit pos.

                uint32_t idx = 0;

                if( bitGetFirstZeroBitPosIfAvailInline( ( char * ) pBuf, 0, mPoolBlockTotalChunks - 1, idx )
                    && idx < pPool->mPoolBlockTotalChunks
                    && idx >= getReservedChunks< minAlign >( allocSize ) )
                {
                    chunkInd = idx;

                    CMA_assert( chunkInd < ( uint32_t ) pPool->mPoolBlockTotalChunks );
                    CMA_assert( chunkInd >= getReservedChunks< minAlign >( allocSize ) );

                    pChunk = pPool->getChunkPtrFromInd( chunkInd, pBuf );
                    bitSet( ( char * ) pBuf, chunkInd );
                    argInd = (uint16_t)chunkInd;

                    CMA_assert( pFirstChunk->mFreeChunkCnt );
                    CMA_assert( pFirstChunk->mFreeChunkCnt <= pPool->mPoolBlockTotalChunks - reservedChunks );

                    --pFirstChunk->mFreeChunkCnt;
                }
                else
                {
                    CMA_assert( 0 && "No empty chunks in buffer." ); // this should't happen
                    CMA_assert( pChunk == NULL ); // if it happens anyway, pChunk should be NULL;

                    return pChunk;
                }
            }

            CMA_assert( pChunk );

            // If there are no more avail empty chunks in buf, clear its bit in NonFull bitset.
            if( pFirstChunk->mFreeChunkCnt == 0 )
            {
                bitClear( pPool->mPoolNonFullBlockBufsBitset, bufInd );
                --pPool->mNonFullBlCnt;

                // In Pool, save info about next non-full buffer (ptr and index in NFBBlockBufBitset).
                if( pPool->mpNonFullBlPtr && pPool->mNonFullBlStartInd == bufInd )
                {
                    CMA_assert( pPool->mpNonFullBlPtr == pBuf );
                    pPool->mNonFullBlStartInd = 0;
                    pPool->mpNonFullBlPtr = NULL;
                }

                // In Pool, save info about next free Chunk (ptr and index in chunkBitset).
                // In FirstChunk in given Buf, save info about next free Chunk index (in chunkBitset).
                if( pPool->mpPoolNextFreeChunkPtrInNonFullBl )
                {
                    CMA_assert( pPool->mPoolNextFreeChunkIndInNonFullBl == argInd );
                    CMA_assert( pPool->mPoolNextFreeChunkIndInNonFullBl == pFirstChunk->mNextFreeChunkInd );
                    CMA_assert( pPool->mpPoolNextFreeChunkPtrInNonFullBl == pChunk );
                    pPool->mpPoolNextFreeChunkPtrInNonFullBl = NULL;
                    pPool->mPoolNextFreeChunkIndInNonFullBl = 0;
                    pFirstChunk->mNextFreeChunkInd = 0;
                }
            }
            // There are some empty chunks in this buf.
            else
            {
                CMA_assert( pPool->mpNonFullBlPtr );
                CMA_assert( pPool->mpNonFullBlPtr == pBuf );
                                    
                // Info about next non-full buffer (ptr and index in NFBBlockBufBitset), are already there.

                // In Pool, save info about next free Chunk (ptr and index in chunkBitset).
                // In FirstChunk in given Buf, save info about next free Chunk index (in chunkBitset).
                {
                    if( pPool->mpPoolNextFreeChunkPtrInNonFullBl )
                    {
                        CMA_assert( pPool->mPoolNextFreeChunkIndInNonFullBl == argInd );
                        CMA_assert( pPool->mPoolNextFreeChunkIndInNonFullBl == pFirstChunk->mNextFreeChunkInd );
                        CMA_assert( pPool->mpPoolNextFreeChunkPtrInNonFullBl == pChunk );
                    }

                    uint32_t nextFreeChunkInd = 0;
                    
                    {
                        uint32_t idx = 0;

                        if( bitGetFirstZeroBitPosIfAvailInline( ( char * ) pBuf, 0, mPoolBlockTotalChunks - 1, idx )
                            && idx < pPool->mPoolBlockTotalChunks
                            && idx >= getReservedChunks< minAlign >( allocSize ) )
                        {
                            nextFreeChunkInd = idx;

                            CMA_assert( pFirstChunk->mFreeChunkCnt );
                            CMA_assert( pFirstChunk->mFreeChunkCnt < pPool->mPoolBlockTotalChunks - reservedChunks );

                            CMA_assert( nextFreeChunkInd < ( uint32_t ) pPool->mPoolBlockTotalChunks );
                            CMA_assert( nextFreeChunkInd >= getReservedChunks< minAlign >( allocSize ) );

                            pPool->mPoolNextFreeChunkIndInNonFullBl = (uint16_t)nextFreeChunkInd;
                            pPool->mpPoolNextFreeChunkPtrInNonFullBl = getChunkPtrFromInd( nextFreeChunkInd, pBuf ); 

                            pFirstChunk->mNextFreeChunkInd = nextFreeChunkInd;
                        }
                        else
                        {
                            CMA_assert( 0 && "No empty chunks in buffer." ); // this should't happen
                        }
                    } // nextFreeChunkInd
                } // save info about next free Chunk
            } // There are some empty chunks in this buf.

            if( pChunk )
            {
                ( ( ChunkWrapper * ) pChunk )->mAllocType       = Allocator_Custom_Dynamic;
                ( ( ChunkWrapper * ) pChunk )->mPoolInd         = cPoolInd;
                ( ( ChunkWrapper * ) pChunk )->mChunkInd        = argInd;

    #if defined( _MS_CRT_STDMUTEX_WA_ )
                ( ( ChunkWrapper * ) pChunk )->mToken        = CMA_token;
    #endif
            }
    
            CMA_assert( pChunk );
        }

        CMA_assert( pChunk );

        return pChunk;
    }


    inline void* CustomMA::CustomAllocate( const size_t argSize )
    {
        uint_fast32_t doOSAlloc = 0;

        const size_t origSize = argSize > 0 ? argSize : minAlign;

        if( mEnabled && origSize <= ( chunkSizeThreshold - overlaySize ) )
        {

            spinlock_t * pLocSpinLock = CustomMA::mpSpinlock;


            if( !pLocSpinLock )
            {
                if( glThCnt < cMaxThds )
                {

#if defined ThreadCnt
                    bool locThBooked = false;

                    {
					    lock_t lock( GlobalSpinlock );

                        for( int i = 0; i < cMaxThds; ++i )
                        {
                            if( ttlp[ i ] == NULL )
                            {
                                // there must be sth != 0;
                                // address to actual object would be assigned in CustomMA::Create()
                                ttlp[ i ] = ( SStructThreadLocalPtrs * ) &StructTLPPlaceholder;

                                mCMAThrdInd = (uint16_t)i;
                                locThBooked = true;
                                ++glThCnt;
                                if( glMaxThCnt < glThCnt )
                                    glMaxThCnt = glThCnt;
                                break;
                            }
                        }
                    }

                    if( locThBooked )
                    {
#endif

                        CMA_assert( CustomMA::mpSpinlock == NULL );
                        CMA_assert( CustomMA::mInitialized == 0 );

                        // check if create succeds
                        if( CustomMA::Create() )
                        {
                            CustomMA::mInitialized = 0;
                            return NULL;					// another option is to try osAlloc rather than fail allocation call;
														    // it seems that practically it doesn't make any difference;
                        }
                        pLocSpinLock = CustomMA::mpSpinlock;

#if defined ThreadCnt
                    }
                    else
                    {
                        doOSAlloc = 1;
                    }
#endif

                }
                else
                {
                    doOSAlloc = 1;
                }
            }

            if( doOSAlloc == 0 )
            {

    #if defined ( _DEBUG ) || defined ( _INTERNAL )
     // Check if CMA disable is requested.
     if( IGC_IS_FLAG_ENABLED( DisableCustomMemAllocator ) )
     {
         doOSAlloc = 1;
     }
     else
     {
    #endif

            struct ThreadBlockBufs  * const psThBufs = & ( mpStructThreadLocalPtrs->msThBufs );

            size_t const allocSize = ( ( origSize / minAlign ) + ( ( origSize % minAlign ) > 0 ) ) * minAlign + overlaySize;

            // crit section till the end of this scope
            lock_t lock( *pLocSpinLock );

            unsigned const cPoolInd = getPoolIndex( ( unsigned int ) allocSize );

            // If there are no such pool yet, try to create it.
            if( mpPools[ cPoolInd ] == NULL )
            {
                // Check if there are any empty slots in GlobalBlockBufsBitset.
                if( psThBufs->BufsCnt < blockListLenTresh )
                {
                    void * pGP =  CMAInternalMalloc( sizeof( GenericPool ) );
                
                    if( !pGP )
                    {
                        CMA_assert( 0 );
                        return NULL;
                    }

                    GenericPool * pNewPool = new ( pGP ) GenericPool( ( unsigned int ) allocSize, psThBufs );
                    mpPools[ cPoolInd ] = pNewPool;

#ifdef EnableCMATraces
    #if defined ( _DEBUG ) || defined ( _INTERNAL )
                //if( IGC_IS_FLAG_ENABLED( EnablaCMATraces ) )
                if( IGC_IS_FLAG_ENABLED( ShaderDumpPidDisable ) )
                {
                    mPrintEnable = 1;
                }
    #endif
#endif
                }
                else
                {
                    doOSAlloc = 1;
                }
            }

            if( doOSAlloc == 0 )
            {
                GenericPool * pPool = mpPools[ cPoolInd ];
                char * ptr = NULL;

                CMA_assert( pPool );

                // Check if there are any non-full bufs in this pool.
                if( pPool->mNonFullBlCnt || psThBufs->BufsCnt < blockListLenTresh )
                {
                    ptr = ( char * ) pPool->Allocate();

                    CMA_assert( origSize <= allocSize );
                    CMA_assert( ( allocSize % minAlign ) == 0 );

                    if( ptr )
                    {
                        CMA_assert( ( ( size_t ) ( ptr + overlaySize ) % minAlign ) == 0 );

                        return ( void * ) ( ptr + overlaySize );
                    }
                    else
                    {
                        CMA_assert( 0 );    // this shouldn't happen
                        doOSAlloc = 1;      // fallback to OS alloc
                    }
                } // if( pPool->mNonFullBlCnt || psThBufs->BufsCnt < blockListLenTresh )
                else
                {
                    doOSAlloc = 1;
                }
            }

   #if defined ( _DEBUG ) || defined ( _INTERNAL )
     }
   #endif

            } // if( doOSAlloc == 0 )

        } // if( origSize <= ( chunkSizeThreshold - overlaySize ) )
        else
        {
            doOSAlloc = 1;
        }

        // OS_malloc
        if( doOSAlloc )
        {
            const size_t allocSize = origSize + overlaySize;

            char * ptr = ( char * ) malloc( allocSize );

            uint32_t dynPtrAlign = 0;

            if( 16 == minAlign )
            {
                dynPtrAlign = ( uint32_t ) ( ( size_t ) ptr % minAlign );
                CMA_assert( dynPtrAlign == 0 || dynPtrAlign == 8 );
                if( dynPtrAlign == 8 )
                {
                    free( ptr );

                    // realloc
                    ptr = ( char * ) malloc( allocSize + 8 );

                    dynPtrAlign = ( uint32_t ) ( ( size_t ) ptr % minAlign );
                    CMA_assert( dynPtrAlign == 0 || dynPtrAlign == 8 );
                }
            }

            if( ptr )
            {
                if( dynPtrAlign == 8 )
                {
                    ptr += 8;
                }

                CMA_assert( ( ( size_t ) ( ptr + overlaySize ) % minAlign ) == 0 );

                (( ChunkWrapper * ) ptr )->mAllocType   = Allocator_Default;
#if defined( _MS_CRT_STDMUTEX_WA_ )
                (( ChunkWrapper * ) ptr )->mToken       = CMA_token;
#endif

                CMA_assert( origSize <= allocSize );

                if( dynPtrAlign == 8 )
                {
                    ( ( ChunkWrapper * ) ptr )->mFlagDefaultAlloctionAlignedBy8 = 1;
                }
                else
                {
                    ( ( ChunkWrapper * ) ptr )->mFlagDefaultAlloctionAlignedBy8 = 0;
                }

                return ( void * ) ( ptr + overlaySize );
            }
            else
            {
                CMA_assert( 0 );
                return ptr;
            }
        }

        CMA_assert( 0 );
        return NULL;
    } // CustomAllocate

#ifndef DisableForceInline
    #ifndef EnableAsserts
        #if defined _WIN32
            __forceinline
        #else
            inline
        #endif
    #endif
#endif
    // -------------------------------------
    void CustomMA::CustomDeallocateCritSect(
        GenericPool ** pChunkPools,
        const unsigned chunkSize,
        const unsigned chunkInd,
        const unsigned char allocType,
        unsigned char * const pChunk,
        bool fSameThDealloc
)
    {
        GenericPool * pPool = NULL;

        CMA_assert( allocType == Allocator_Custom_Dynamic );

        if( pChunkPools )
        {
            pPool = pChunkPools[ getPoolIndex( chunkSize ) ];
        }

        CMA_assert( pPool );

        if( pPool )
        {
        #ifdef EnableAsserts
            uint32_t totalChunks = getTotalChunks( pPool->mElementSize );
        #endif
            CMA_assert( totalChunks );
            CMA_assert( totalChunks <= ( cBlockBufSize / minAlign ) );
            CMA_assert( chunkInd < totalChunks );
            CMA_assert( chunkInd >= getReservedChunks< minAlign >( pPool->mElementSize ) );


            CMA_assert( chunkSize == pPool->mElementSize );


            // calculate pBlock from pChunk and suze
            char *          pBitSet         = ( ( char * ) pChunk ) - ( chunkSize * chunkInd );
            const unsigned  reservedChunks  = getReservedChunks< minAlign >( chunkSize );
            ChunkWrapper *  pFirstChunk     = ( ChunkWrapper * ) ( pBitSet + reservedChunks * chunkSize );
            uint32_t        bufInd          = pFirstChunk->mBlockInd;

            SStructThreadLocalPtrs * const   pChunkStructThreadLocalPtrs = ttlp[ ( ( ChunkWrapper * ) ( pFirstChunk ) )->mThrInd ];
            CMA_assert( pChunkStructThreadLocalPtrs );


            CMA_assert( bitIsSet( pBitSet, chunkInd ) );

            bitClear( pBitSet, chunkInd );
            ++( pFirstChunk->mFreeChunkCnt );

            CMA_assert( pFirstChunk->mFreeChunkCnt <= pPool->mPoolBlockTotalChunks - reservedChunks );
            CMA_assert( bufInd < blockListLenTresh );

            // if buf was full, then it is not full any more
            if( !bitIsSet( pPool->mPoolNonFullBlockBufsBitset, bufInd ) )
            {
                bitSet( pPool->mPoolNonFullBlockBufsBitset, bufInd );
                ++pPool->mNonFullBlCnt;
                
                if( bufInd < pPool->mNonFullBlStartInd )
                {
                    pPool->mNonFullBlStartInd = (uint16_t)bufInd;
                    pPool->mpNonFullBlPtr = pBitSet;

                    pPool->mPoolNextFreeChunkIndInNonFullBl = (uint16_t)chunkInd;
                    pPool->mpPoolNextFreeChunkPtrInNonFullBl = pChunk;
                }

                pFirstChunk->mNextFreeChunkInd = chunkInd;
            }
            // buf wasn't full
            else
            {
                // if this was the last chunk in buf, and now buf is empty then free buf
                if( /*fSameThDealloc && */pPool->mNonFullBlCnt > 1 && pFirstChunk->mFreeChunkCnt == pPool->mPoolBlockTotalChunks - reservedChunks )
                {
                    bitClear( pPool->mPoolNonFullBlockBufsBitset, bufInd );
                    --( pPool->mNonFullBlCnt );
                    --( pPool->mPoolBlCnt );
                    
                    if( pPool->mpNonFullBlPtr && pPool->mNonFullBlStartInd == bufInd )
                    {
                        CMA_assert( pPool->mpNonFullBlPtr == pBitSet );
                        pPool->mNonFullBlStartInd = 0;
                        pPool->mpNonFullBlPtr = NULL;
                        pPool->mpPoolNextFreeChunkPtrInNonFullBl = NULL;
                        pPool->mPoolNextFreeChunkIndInNonFullBl = 0;
                    }

#if defined EnableAsserts
                    bitClear( pPool->mPoolBlockBufsBitset, bufInd );
#endif

                    if( bitIsSet( pChunkStructThreadLocalPtrs->msThBufs.BufPaddedBy8Bitset, bufInd ) )
                    {
                        CMAInternalFree( pBitSet - 8, cBlockBufSize + 8 );
                    }
                    else
                    {
                        CMAInternalFree( pBitSet, cBlockBufSize );
                    }

                    // mark it as UNused
                    bitClear( pChunkStructThreadLocalPtrs->msThBufs.BufsBitset, bufInd );

                    if( bufInd < pChunkStructThreadLocalPtrs->msThBufs.BufsStartInd )
                    {
                        pChunkStructThreadLocalPtrs->msThBufs.BufsStartInd = (uint16_t)bufInd;
                    }

                    --pChunkStructThreadLocalPtrs->msThBufs.BufsCnt;
                    pChunkStructThreadLocalPtrs->msThBufs.Bufs[ bufInd ] = NULL;
                }
                else
                {
                    if( pFirstChunk->mNextFreeChunkInd > chunkInd )
                    {
                        pFirstChunk->mNextFreeChunkInd = chunkInd;
                        
                        if( pPool->mpNonFullBlPtr == pBitSet )
                        {
                            pPool->mPoolNextFreeChunkIndInNonFullBl = (uint16_t)chunkInd;
                            pPool->mpPoolNextFreeChunkPtrInNonFullBl = pChunk;
                        }
                    }
                }
            }
        } // if pPool
    }

    // -----------------------------------------------------
    inline void CustomMA::CustomDeallocate( void * arg_ptr )
    {
        unsigned char * ptr = ( unsigned char * ) arg_ptr;

        if( ptr )
        {
#if defined( _MS_CRT_STDMUTEX_WA_ )
            const uint32_t      readToken =  ( ( ChunkWrapper * ) ( ptr - overlaySize ) )->mToken;

            if( readToken == CMA_token )
            {
#endif
                unsigned char * const   pChunk      = ptr - overlaySize;
                const unsigned char     allocType   = ( ( ChunkWrapper * ) ( pChunk ) )->mAllocType;

                if( allocType == Allocator_Custom_Dynamic )
                {
                    const unsigned chunkSize = getSizeFromPoolInd( ( ( ChunkWrapper * ) ( pChunk ) )->mPoolInd );
                    const unsigned chunkInd  = ( ( ChunkWrapper * ) ( pChunk ) )->mChunkInd;

                    if( ( chunkSize <= chunkSizeThreshold )
                        && ( ( chunkInd + 1 ) * chunkSize <= cBlockBufSize )
                        && ( chunkInd >= getReservedChunks< minAlign >( chunkSize ) ) )
                    {
                        // calculate pBlock from pChunk and size.
                        char *          pBitSet         = ( ( char * ) pChunk ) - ( chunkSize * chunkInd );
                        const unsigned  reservedChunks  = getReservedChunks< minAlign >( chunkSize );
                        ChunkWrapper *  pFirstChunk     = ( ChunkWrapper * ) ( pBitSet + reservedChunks * chunkSize );
                        GenericPool **  pChunkPools     = NULL;
                        spinlock_t *    pLocalSpinLock  = NULL;

                        CMA_assert( ( ( ChunkWrapper * ) ( pFirstChunk ) )->mThrInd < cMaxThds );

                        SStructThreadLocalPtrs *    pChunkStructThreadLocalPtrs     = ttlp[ ( ( ChunkWrapper * ) ( pFirstChunk ) )->mThrInd ];

                        if( pChunkStructThreadLocalPtrs )
                        {
                            pChunkPools = pChunkStructThreadLocalPtrs->pPools;

							if( ! CustomMA::mpSpinlock )
							{
								// non-cma enabled thread (th > 64)
								// or NOT YET cma enabled thread ( th <= 64 )
								// so only orig thread lock needed (orig thread - the thread that performed allocation)

								// non-orig th dea case 1

								CMA_assert( ! CustomMA::mInitialized );
								CMA_assert( CustomMA::mCMAThrdInd == 0 );
								pLocalSpinLock = pChunkStructThreadLocalPtrs->pSpinlock;
							}
							else
							{
								CMA_assert( CustomMA::mInitialized );
								CMA_assert( CustomMA::mCMAThrdInd < cMaxThds );

								// curr th cma enabled;
								// curr th lock already in  CuMA::mpSpLc

								if( ( ( ChunkWrapper * ) ( pFirstChunk ) )->mThrInd != mCMAThrdInd )
								{
									// non orig th dea case 2 - regular (both ths cma enabled);
									// get orig th lock;
									pLocalSpinLock = pChunkStructThreadLocalPtrs->pSpinlock;
								}
							}
                        }
						else
						{
							CMA_assert( 0 && "There should be always pChunkStructThreadLocalPtrs for th that allocated given chunk." );
						}

                        // other thread's spinlock
                        if( pLocalSpinLock )
                        {
                            if( CustomMA::mpSpinlock )
                            {
                                CMA_assert( CustomMA::mInitialized );
                            
								// both thds cma enabled;
								// respect locking order to avoid deadlock;

                                if( ( ( ChunkWrapper * ) ( pFirstChunk ) )->mThrInd < mCMAThrdInd )
                                {
                                    lock_t lock( *pLocalSpinLock );
                                    lock_t lock2( *CustomMA::mpSpinlock );

                                    CustomMA::CustomDeallocateCritSect( pChunkPools, chunkSize, chunkInd, allocType, pChunk, false );
                                }
                                else
                                {
                                    lock_t lock2( *CustomMA::mpSpinlock );
                                    lock_t lock( *pLocalSpinLock );

                                    CustomMA::CustomDeallocateCritSect( pChunkPools, chunkSize, chunkInd, allocType, pChunk, false );
                                }
                            }
                            else
                            {
								// only orig th lock;
								// curr th non-cma enabled (or not yet);

                                lock_t lock( *pLocalSpinLock );

                                CustomMA::CustomDeallocateCritSect( pChunkPools, chunkSize, chunkInd, allocType, pChunk, false );
                            }
                        }
                        else
                        {
							// dealloc of chunk orignally allocated by the curr thread;

							if( CustomMA::mpSpinlock )
                            {
                                CMA_assert( CustomMA::mInitialized );
								CMA_assert( CustomMA::mpSpinlock );
								
								lock_t lock2( *CustomMA::mpSpinlock );
                                CustomMA::CustomDeallocateCritSect( pChunkPools, chunkSize, chunkInd, allocType, pChunk, true );
                            }
                            else
                            {
								CMA_assert( 0 && "Shouldn't happen." );
                            }
                        } // !pLocalSpinLock

                    } // chunkSize, chunkInd
                    else
                    {
                        CMA_assert( chunkSize <= chunkSizeThreshold );
                        CMA_assert( chunkInd < getTotalChunks( chunkSize ) );
                        CMA_assert( chunkInd >= getReservedChunks< minAlign >( chunkSize ) );
						CMA_assert( 0 && "Shouldn't happen." );
                    }

                } // Allocator_Custom_Dynamic
                else if( allocType == Allocator_Default )
                {
                    if( ( ( ChunkWrapper * ) pChunk )->mFlagDefaultAlloctionAlignedBy8 )
                    {
                        free( pChunk - 8 );
                    }
                    else
                    {
                        free( pChunk );
                    }
                }
                else
                {
                    CMA_assert( 0 );
                }
#if defined( _MS_CRT_STDMUTEX_WA_ )
            } // if( readToken == CMA_token )
            else
            {
                    free( ptr );
            }
#endif
        } // if ( ptr )
    } // CustomDeallocate

#ifdef OptimizationsOff
#pragma optimize( "", on )
#endif

} // end namespace cma

using namespace CMA;

#ifdef OptimizationsOff
#pragma optimize( "", off )
#endif

void CMADLLThreadAttach()
{
    CustomMA::DLLThreadAttach();
}

void CMADLLProcessAttach()
{
    CustomMA::DLLProcessAttach();
}

void CMADLLThreadDetach()
{
    CustomMA::DLLThreadDetach();
}


void CMADLLProcessDetach()
{
    CustomMA::DLLProcessDetach();
}


#ifdef OptimizationsOff
#pragma optimize( "", on )
#endif

#endif


#if defined( _WIN32 ) || ( defined ( _DEBUG ) || defined ( _INTERNAL ) )
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
    void* ptr = NULL;

    // CMA is currently enabled only for IGC module (excluding IGC_STANDALONE tool).
    // CAllocator seems always be enabled for all platforms.
    // CMA check-in doesn't change that.

#if ( defined( _WIN32 ) && !( defined IGC_STANDALONE ) )
    ptr = CMA::CustomMA::CustomAllocate( size );
#else
    ptr = malloc( size );
#endif

#ifdef _DEBUG
    if(ptr)
    {
#ifdef _IGC_
        // does not introduce dependencies, is into memset by compiler
        for(unsigned int t = 0; t<size; ++t)
            static_cast< unsigned char* >(ptr)[t] = 0xcc;
#else
        ::memset(ptr, 0xcc, size);
#endif
    }
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
    if(ptr)
    {
#if ( defined( _WIN32 ) && !( defined IGC_STANDALONE ) )
    CMA::CustomMA::CustomDeallocate( ptr );
#else
    free( ptr );
#endif
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
#if defined(ANDROID)
inline void* operator new(size_t size)
#else
inline void* operator new(size_t size)
#endif
{
    void* storage = CAllocator::Allocate(size);
    assert(storage && "Could not allocate the required memory to storage");
    return storage;
}

inline void operator delete(void* ptr)
{
    assert( ptr && "ptr cannot be null");
    CAllocator::Deallocate(ptr);
}

// TODO: Throw exception if the allocation fails.
#if defined(ANDROID)
inline void* operator new[](size_t size)
#else
inline void* operator new[](size_t size)
#endif
{
    return ::operator new(size);
}

inline void operator delete[](void* ptr)
{
    assert( ptr && "ptr cannot be null");
    CAllocator::Deallocate(ptr);
}
#endif // !defined __clang__
#endif//defined __GNUC__
#endif //defined ( _DEBUG ) || defined ( _INTERNAL )

#ifndef __GNUC__
#define __NOTAGNUC__ 
#endif // __GNUC__

#if defined( _WIN32 ) || ( ( defined ( _DEBUG ) || defined ( _INTERNAL ) ) && (defined __NOTAGNUC__ ) )
/*****************************************************************************\
 operator new
\*****************************************************************************/
void* __cdecl operator new( size_t size )
{
    void* storage = CAllocator::Allocate(size);
#if defined ( _DEBUG ) || defined ( _INTERNAL )
    assert(storage && "Could not allocate the required memory to storage");
#endif
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
#endif // defined( _WIN32 ) || ( ( defined ( _DEBUG ) || defined ( _INTERNAL ) ) && (defined __NOTAGNUC__) )

#if defined( _WIN32 )
void* __cdecl operator new( size_t size, const std::nothrow_t& ) throw()
{
    return CAllocator::Allocate( size );
}

void __cdecl operator delete( void* ptr, const std::nothrow_t& ) throw()
{
    CAllocator::Deallocate( ptr );
}

void* __cdecl operator new[]( size_t size, const std::nothrow_t& ) throw()
{
    return CAllocator::Allocate( size );
}

void __cdecl operator delete[]( void* ptr, const std::nothrow_t& ) throw()
{
    CAllocator::Deallocate( ptr );
}
#endif // defined( _WIN32 )

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include "Debug.h"
#include "utility.h"
#include <string.h>
#include "CpuUtil.h"

#if !defined ( _MSC_VER )
#include "inc/common/secure_mem.h"
#endif

#if defined(_WIN32)
    #include <basetsd.h>
    #if defined ( _WIN64 ) && defined ( _In_ )
        // NOTE: <math.h> is not necessary here.
        // This is only an ugly workaround for a VS2008 bug that causes the compilation
        // issue on 64-bit DEBUG configuration.
        // Including "math.h" before "intrin.h" helps to get rid of the following warning:
        // warning C4985: 'ceil': attributes not present on previous declaration.
        #include <math.h>
    #endif

    #include <intrin.h>

    #define USE_X86
    #define USE_SSE4_1
#elif defined(__i386__) || defined(__x86_64__)
    #include <x86intrin.h>
    #define USE_X86

    #if defined(__SSE4_1__)
        #define USE_SSE4_1
    #endif // defined(__SSE4_1__)
#endif

#if defined(USE_SSE4_1)
typedef __m128              DQWORD;         // 128-bits,   16-bytes
#endif

typedef DWORD               PREFETCH[8];    //             32-bytes
typedef DWORD               CACHELINE[8];   //             32-bytes
typedef WORD                DHWORD[32];     // 512-bits,   64-bytes

namespace iSTD
{

enum
{
    DWORD_SHIFT         = 2,
    BYTE_TAIL           = 3,
    INSTR_128_SHIFT     = 4,
    CACHE_LINE_SHIFT    = 6,
    DUAL_CACHE_SHIFT    = 7,
    TAIL_SIZE           = 15,
    INSTR_WIDTH_128     = 16,
    INSTR_WIDTH_256     = 32,
    CACHE_LINE_SIZE     = 64,
    TIERED_TAIL         = 127,
    DUAL_CACHE_SIZE     = 128,
    MIN_ERMSB_ALIGNED   = 4096,
    MIN_STREAM_SIZE     = 524288,
};

#ifdef _WIN64
#   define USE_INLINE_ASM 0
#else
#   if defined _MSC_VER
#       define USE_INLINE_ASM 1
#   else
#       define USE_INLINE_ASM 0
#   endif
#endif

/*****************************************************************************\
Function Prototypes
\*****************************************************************************/
inline void Prefetch( const void* );
inline void PrefetchBuffer( const void*, const size_t );
inline void CachelineFlush( const void* );

template <size_t size>
inline void MemCopy( void*, const void* );
inline void MemCopy( void*, const void*, const size_t );
inline void MemCopyWC( void*, const void*, const size_t );
inline void MemCopySwapBytes( void*, const void*, const size_t, const unsigned int);
inline void ScalarSwapBytes( void**, const void**, const size_t, const unsigned int);

inline void SafeMemSet( void*, const int, const size_t );
inline int  SafeMemCompare( const void*, const void*, const size_t );
inline void SafeMemMove( void*, const void*, const size_t );

#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
inline void  __fastcall FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqa(void* dst, const void* src );
inline void  __fastcall FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqu(void* dst, const void* src );
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
inline void FastMemCopyFromWC( void* dst, const void* src, const size_t bytes, CPU_INSTRUCTION_LEVEL cpuInstructionLevel);

inline void FastCpuBlt( BYTE*, const DWORD, BYTE*, const DWORD, const DWORD, DWORD );

inline void FindWordBufferMinMax( WORD*, const DWORD, WORD&, WORD& );
inline void FindDWordBufferMinMax( DWORD*, const DWORD, DWORD&, DWORD& );
inline void FindWordBufferMinMaxRestart( WORD*, const DWORD, const WORD, WORD&, WORD&, CPU_INSTRUCTION_LEVEL cpuInstructionLevel );
inline void FindDWordBufferMinMaxRestart( DWORD*, const DWORD, const DWORD, DWORD&, DWORD&, CPU_INSTRUCTION_LEVEL cpuInstructionLevel );

inline void FindWordBufferMinMaxCopy( WORD*, WORD*, const DWORD, WORD&, WORD& );
inline void FindDWordBufferMinMaxCopy( DWORD*, DWORD*, const DWORD, DWORD&, DWORD& );
inline void FindWordBufferMinMaxRestartCopy( WORD*, WORD*, const DWORD, const WORD, WORD&, WORD&, CPU_INSTRUCTION_LEVEL cpuInstructionLevel );
inline void FindDWordBufferMinMaxRestartCopy( DWORD*, DWORD*, const DWORD, const DWORD, DWORD&, DWORD&, CPU_INSTRUCTION_LEVEL cpuInstructionLevel );

/*****************************************************************************\
Inline Function:
    Prefetch

Description:
    executes __asm prefetchnta
\*****************************************************************************/
inline void Prefetch( const void* ptr )
{
#if defined(USE_X86)
    _mm_prefetch( (const char*)ptr, _MM_HINT_NTA );
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(ptr, 0, 0);
#endif
}

/*****************************************************************************\
Inline Function:
    PrefetchBuffer

Description:
    executes __asm prefetchnta
\*****************************************************************************/
inline void PrefetchBuffer( const void* pBuffer, const size_t bytes )
{
    const size_t cachelines = bytes / sizeof(PREFETCH);

    for( size_t i = 0; i <= cachelines; i++ )
    {
#if defined(USE_X86)
        _mm_prefetch( (const char*)pBuffer + i * sizeof(PREFETCH),
            _MM_HINT_NTA );
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch((const char*)pBuffer + i * sizeof(PREFETCH), 0, 0);
#endif
    }
}

/*****************************************************************************\
Inline Function:
    CachelineFlush

Description:
    executes __asm clflush
\*****************************************************************************/
inline void CachelineFlush( const void* ptr )
{
#if defined(USE_X86)
    _mm_clflush( (char*)ptr );
#endif
}

/*****************************************************************************\
Inline Function:
    MemCopy

Description:
    Templated Exception Handler Memory Copy function
\*****************************************************************************/
template <size_t size>
inline void MemCopy( void* dst, const void* src )
{
    MemCopy(dst, src, size);
}

template <>
inline void MemCopy<1>( void* dst, const void* src )
{
    const BYTE* pSrc = reinterpret_cast<const BYTE*>(src);
    BYTE*       pDst = reinterpret_cast<BYTE*>(dst);
    *pDst = *pSrc;
}

template <>
inline void MemCopy<2>( void* dst, const void* src )
{
    const WORD* pSrc = reinterpret_cast<const WORD*>(src);
    WORD*       pDst = reinterpret_cast<WORD*>(dst);
    *pDst = *pSrc;
}

template <>
inline void MemCopy<4>( void* dst, const void* src )
{
    const UINT32*   pSrc = reinterpret_cast<const UINT32*>(src);
    UINT32*         pDst = reinterpret_cast<UINT32*>(dst);
    *pDst = *pSrc;
}

template <>
inline void MemCopy<8>( void* dst, const void* src )
{
    const UINT64*   pSrc = reinterpret_cast<const UINT64*>(src);
    UINT64*         pDst = reinterpret_cast<UINT64*>(dst);
    *pDst = *pSrc;
}

#if defined(USE_SSE4_1)
template <>
inline void MemCopy<16>( void* dst, const void* src )
{
    const __m128i*  pMMSrc  = reinterpret_cast<const __m128i*>(src);
    __m128i*        pMMDst  = reinterpret_cast<__m128i*>(dst);
    __m128i         xmm0    = _mm_loadu_si128(pMMSrc);
    _mm_storeu_si128(pMMDst, xmm0);
}

template <>
inline void MemCopy<28>( void* dst, const void* src )
{
    const __m128i*  pMMSrc  = reinterpret_cast<const __m128i*>( src );
    __m128i*        pMMDst  = reinterpret_cast<__m128i*>( dst );
    __m128i         xmm0    = _mm_loadu_si128( pMMSrc );
    _mm_storeu_si128( pMMDst, xmm0 );

    pMMSrc += 1;
    pMMDst += 1;

    const UINT64*   pSrc64 = reinterpret_cast<const UINT64*>( pMMSrc );
    UINT64*         pDst64 = reinterpret_cast<UINT64*>( pMMDst );
    *pDst64 = *pSrc64;

    pDst64 += 1;
    pSrc64 += 1;

    const UINT32*   pSrc32 = reinterpret_cast<const UINT32*>( pSrc64 );
    UINT32*         pDst32 = reinterpret_cast<UINT32*>( pDst64 );
    *pDst32 = *pSrc32;
}
#endif // defined(USE_SSE4_1)

/*****************************************************************************\
Inline Function:
    MemCopy

Description:
    Exception Handler Memory Copy function
\*****************************************************************************/
inline void MemCopy( void* dst, const void* src, const size_t bytes )
{
#if defined ( USE_SSE4_1 )
    UINT8*            pDst8 = reinterpret_cast<UINT8*>( dst );
    const UINT8*    pSrc8 = reinterpret_cast<const UINT8*>( src );
    size_t            bytesRemaining = bytes;

    // handle invalid cases
    if( bytesRemaining == 0 )
        return;

    // handle sizes <= 4 bytes
    if( bytesRemaining <= 4 )
    {
        if( bytesRemaining == 1 )
        {
            // copy 1 bytes
            *pDst8 = *pSrc8;
            return;
        }

        if( bytesRemaining == 2 )
        {
            // copy 2 bytes
            *reinterpret_cast<UINT16*>( pDst8 ) = *reinterpret_cast<const UINT16*>( pSrc8 );
            return;
        }

        if( bytesRemaining == 3 )
        {
            // copy 3 bytes
            *reinterpret_cast<UINT16*>( pDst8 ) = *reinterpret_cast<const UINT16*>( pSrc8 );
            *( pDst8 + 2 ) = *( pSrc8 + 2 );
            return;
        }

        *reinterpret_cast<UINT32*>( pDst8 ) = *reinterpret_cast<const UINT32*>( pSrc8 );
        return;
    }

    // align destination to 4 byte boundary if size is > 8 bytes
    if( bytesRemaining > 8 &&
        reinterpret_cast<UINT_PTR>( pDst8 ) & 0x3 )
    {
        // check for shift by 1
        if( reinterpret_cast<UINT_PTR>( pDst8 ) & 0x1 )
        {
            *pDst8 = *pSrc8;

            bytesRemaining -= 1;
            pDst8 += 1;
            pSrc8 += 1;
        }

        // check for shift by 2
        if( reinterpret_cast<UINT_PTR>( pDst8 ) & 0x2 )
        {
            *reinterpret_cast<UINT16*>( pDst8 ) = *reinterpret_cast<const UINT16*>( pSrc8 );

            bytesRemaining -= 2;
            pDst8 += 2;
            pSrc8 += 2;
        }
    }

    // handle sizes <= 64 bytes as series of 4 byte moves
    if( bytesRemaining <= CACHE_LINE_SIZE )
    {
        const size_t ptrAdvance = bytesRemaining & ~0x3; // TODO: Need to see if we can mimic the jump table

        pDst8 += ptrAdvance;
        pSrc8 += ptrAdvance;

        switch( bytesRemaining / 4 )
        {
            case 16:
                *reinterpret_cast<UINT32*>( pDst8 - 64 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 64 );
            case 15:
                *reinterpret_cast<UINT32*>( pDst8 - 60 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 60 );
            case 14:
                *reinterpret_cast<UINT32*>( pDst8 - 56 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 56 );
            case 13:
                *reinterpret_cast<UINT32*>( pDst8 - 52 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 52 );
            case 12:
                *reinterpret_cast<UINT32*>( pDst8 - 48 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 48 );
            case 11:
                *reinterpret_cast<UINT32*>( pDst8 - 44 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 44 );
            case 10:
                *reinterpret_cast<UINT32*>( pDst8 - 40 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 40 );
            case 9:
                *reinterpret_cast<UINT32*>( pDst8 - 36 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 36 );
            case 8:
                *reinterpret_cast<UINT32*>( pDst8 - 32 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 32 );
            case 7:
                *reinterpret_cast<UINT32*>( pDst8 - 28 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 28 );
            case 6:
                *reinterpret_cast<UINT32*>( pDst8 - 24 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 24 );
            case 5:
                *reinterpret_cast<UINT32*>( pDst8 - 20 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 20 );
            case 4:
                *reinterpret_cast<UINT32*>( pDst8 - 16 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 16 );
            case 3:
                *reinterpret_cast<UINT32*>( pDst8 - 12 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 12 );
            case 2:
                *reinterpret_cast<UINT32*>( pDst8 - 8 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 8 );
            case 1:
                *reinterpret_cast<UINT32*>( pDst8 - 4 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 4 );
        }

        // tail may have up to 3 bytes off
        if( bytesRemaining & 0x1 )
        {
            *pDst8 = *pSrc8;

            bytesRemaining -= 1;
            pDst8 += 1;
            pSrc8 += 1;
        }

        if( bytesRemaining & 0x2 )
        {
            *reinterpret_cast<UINT16*>( pDst8 ) = *reinterpret_cast<const UINT16*>( pSrc8 );

            bytesRemaining -= 2;
            pDst8 += 2;
            pSrc8 += 2;
        }
    }

    // size is > 64 bytes use SSE2
    else
    {
        __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; // xmm registers

        // align the destination to 16 bytes if necessary
        const size_t alignDst16 = reinterpret_cast<UINT_PTR>( pDst8 ) & TAIL_SIZE;
        if( alignDst16 != 0 )
        {
            const size_t alignSize = 0x10 - alignDst16;

            // already aligned to 4 bytes previously, so remainder must be a multiple of 4
            pDst8 += alignSize;
            pSrc8 += alignSize;

            switch( alignSize / 4 )
            {
                case 3:
                    *reinterpret_cast<UINT32*>( pDst8 - 12 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 12 );
                case 2:
                    *reinterpret_cast<UINT32*>( pDst8 - 8 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 8 );
                case 1:
                    *reinterpret_cast<UINT32*>( pDst8 - 4 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 4 );
            }

            bytesRemaining -= alignSize;
        }

        // if the size is greater than 1/2 largest cache
        if( bytesRemaining > MIN_STREAM_SIZE )
        {
            while( bytesRemaining >= 128 )
            {
                pDst8 += 128;
                pSrc8 += 128;
                bytesRemaining -= 128;

                xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 128 ));
                xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                xmm7 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));

                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 128 ), xmm0 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm1 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm2 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm3 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm4 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm5 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm6 );
                _mm_stream_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm7);
            }

            // copy up to 128 bytes
            const size_t ptrAdvance = bytesRemaining & ~0xF;

            pDst8 += ptrAdvance;
            pSrc8 += ptrAdvance;

            switch( bytesRemaining / 16 )
            {
                case 7:
                    xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm0 );
                case 6:
                    xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm1 );
                case 5:
                    xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm2 );
                case 4:
                    xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm3 );
                case 3:
                    xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm4 );
                case 2:
                    xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm5 );
                case 1:
                    xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm6 );
            }

            bytesRemaining -= ptrAdvance;
        }

        // size is less than 1/2 the largest cache, copy either fully aligned or partially aligned
        else
        {
            const size_t alignSrc16 = reinterpret_cast<UINT_PTR>( pSrc8 ) & 0xF;

            // copy with source un-aligned
            if( alignSrc16 != 0 )
            {
                while( bytesRemaining >= 128 )
                {
                    pDst8 += 128;
                    pSrc8 += 128;
                    bytesRemaining -= 128;

                    xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 128 ));
                    xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                    xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                    xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                    xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                    xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                    xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                    xmm7 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));

                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 128 ), xmm0 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm1 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm2 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm3 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm4 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm5 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm6 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm7 );
                }

                // copy up to 128 bytes
                const size_t ptrAdvance = bytesRemaining & ~0xF;

                pDst8 += ptrAdvance;
                pSrc8 += ptrAdvance;

                switch( bytesRemaining / 16 )
                {
                    case 7:
                        xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm0 );
                    case 6:
                        xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm1 );
                    case 5:
                        xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm2 );
                    case 4:
                        xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm3 );
                    case 3:
                        xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm4 );
                    case 2:
                        xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm5 );
                    case 1:
                        xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm6 );
                }

                bytesRemaining -= ptrAdvance;
            }

            // copy with source aligned
            else
            {
                while( bytesRemaining >= 128 )
                {
                    pDst8 += 128;
                    pSrc8 += 128;
                    bytesRemaining -= 128;

                    xmm0 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 128 ));
                    xmm1 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                    xmm2 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                    xmm3 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                    xmm4 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                    xmm5 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                    xmm6 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                    xmm7 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));

                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 128 ), xmm0 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm1 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm2 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm3 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm4 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm5 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm6 );
                    _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm7 );
                }

                // copy up to 128 bytes
                const size_t ptrAdvance = bytesRemaining & ~0xF;

                pDst8 += ptrAdvance;
                pSrc8 += ptrAdvance;

                switch( bytesRemaining / 16 )
                {
                    case 7:
                        xmm0 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 112 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 112 ), xmm0 );
                    case 6:
                        xmm1 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 96 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 96 ), xmm1 );
                    case 5:
                        xmm2 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 80 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 80 ), xmm2 );
                    case 4:
                        xmm3 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 64 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 64 ), xmm3 );
                    case 3:
                        xmm4 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 48 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 48 ), xmm4 );
                    case 2:
                        xmm5 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 32 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 32 ), xmm5 );
                    case 1:
                        xmm6 = _mm_load_si128( reinterpret_cast<const __m128i*>( pSrc8 - 16 ));
                        _mm_store_si128( reinterpret_cast<__m128i*>( pDst8 - 16 ), xmm6 );
                }

                bytesRemaining -= ptrAdvance;
            }
        }

        // copy the tail up to 15 bytes
        if( bytesRemaining )
        {
            const size_t ptrAdvance = bytesRemaining & ~0x3;

            pDst8 += ptrAdvance;
            pSrc8 += ptrAdvance;

            // copy last up to 12 bytes
            switch( bytesRemaining / 4 )
            {
                case 3:
                    *reinterpret_cast<UINT32*>( pDst8 - 12 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 12 );
                case 2:
                    *reinterpret_cast<UINT32*>( pDst8 - 8 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 8 );
                case 1:
                    *reinterpret_cast<UINT32*>( pDst8 - 4 ) = *reinterpret_cast<const UINT32*>( pSrc8 - 4 );
            }

            // copy last up to 3 bytes
            if( bytesRemaining & 0x1 )
            {
                *pDst8 = *pSrc8;

                bytesRemaining -= 1;
                pDst8 += 1;
                pSrc8 += 1;
            }

            if( bytesRemaining & 0x2 )
            {
                *reinterpret_cast<UINT16*>( pDst8 ) = *reinterpret_cast<const UINT16*>( pSrc8 );

                bytesRemaining -= 2;
                pDst8 += 2;
                pSrc8 += 2;
            }
        }
    }
#else // !defined ( USE_SSE4_1 )
    // Linux projects do not support standard types or memcpy_s
    ::memcpy_s(dst, bytes, src, bytes);
#endif
}

/*****************************************************************************\
Inline Function:
    MemCopyWC

Description:
    Memory copy to a destination that is un-cacheable, i.e host to gpu.

Input:
    dst - pointer to write-combined destination buffer
    src - pointer to source buffer
    bytes - number of bytes to copy
\*****************************************************************************/
inline void MemCopyWC( void* dst, const void* src, const size_t bytes )
{
#if defined ( USE_SSE4_1 )
    const __m128i           s_SSE2CmpMask   = _mm_setr_epi8( 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 );
    const __m128i*          pMMSrc          = reinterpret_cast<const __m128i*>(src);
    __m128i*                pMMDest         = reinterpret_cast<__m128i*>(dst);
    size_t                  count           = bytes;
    size_t                  cnt             = 0;
    __m128i                 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

    // if size > 16 align destination and move non-temporally
    if (count >= INSTR_WIDTH_128)
    {
        // align destination to 16 if necessary
        UINT32 align = (UINT32)((UINT_PTR)pMMDest & TAIL_SIZE);
        if (align != 0)
        {
            // move alignment through a masked non-temporal move
            const char* pSrc    = reinterpret_cast<const char*>(pMMSrc);
            char*       pDst    = reinterpret_cast<char*>(pMMDest);

            align               = INSTR_WIDTH_128 - align;
            char    shiftCnt    = (char)(INSTR_WIDTH_128 - align - 1);
            __m128i shiftMask   = _mm_set1_epi8(shiftCnt);
            __m128i mask        = _mm_cmpgt_epi8(s_SSE2CmpMask, shiftMask);
            __m128i val         = _mm_loadu_si128(pMMSrc);
            _mm_maskmoveu_si128(val, mask, pDst);

            pSrc += align;
            pDst += align;

            pMMSrc  = reinterpret_cast<const __m128i*>(pSrc);
            pMMDest = reinterpret_cast<__m128i*>(pDst);
        }

        count -= align; // take off the alignment from size

        // check source alignment
        if ((UINT_PTR)pMMSrc & TAIL_SIZE)
        {
            // copy un-aligned by tiers
            cnt = count >> DUAL_CACHE_SHIFT;
            for (UINT32 i = 0; i < cnt; i += 1)
            {
                xmm0 = _mm_loadu_si128(pMMSrc);
                xmm1 = _mm_loadu_si128(pMMSrc + 1);
                xmm2 = _mm_loadu_si128(pMMSrc + 2);
                xmm3 = _mm_loadu_si128(pMMSrc + 3);
                xmm4 = _mm_loadu_si128(pMMSrc + 4);
                xmm5 = _mm_loadu_si128(pMMSrc + 5);
                xmm6 = _mm_loadu_si128(pMMSrc + 6);
                xmm7 = _mm_loadu_si128(pMMSrc + 7);
                pMMSrc += 8;

                _mm_stream_si128(pMMDest, xmm0);
                _mm_stream_si128(pMMDest + 1, xmm1);
                _mm_stream_si128(pMMDest + 2, xmm2);
                _mm_stream_si128(pMMDest + 3, xmm3);
                _mm_stream_si128(pMMDest + 4, xmm4);
                _mm_stream_si128(pMMDest + 5, xmm5);
                _mm_stream_si128(pMMDest + 6, xmm6);
                _mm_stream_si128(pMMDest + 7, xmm7);
                pMMDest += 8;
            }

            count &= TIERED_TAIL;
            if (count != 0)
            {
                cnt = count >> INSTR_128_SHIFT;
                for (UINT32 i = 0; i < cnt; i += 1)
                {
                    xmm0 = _mm_loadu_si128(pMMSrc);
                    pMMSrc += 1;
                    _mm_stream_si128(pMMDest, xmm0);
                    pMMDest += 1;
                }
            }
        }
        else
        {
            // copy aligned by tiers
            cnt = count >> DUAL_CACHE_SHIFT;
            for (UINT32 i = 0; i < cnt; i += 1)
            {
                xmm0 = _mm_load_si128(pMMSrc);
                xmm1 = _mm_load_si128(pMMSrc + 1);
                xmm2 = _mm_load_si128(pMMSrc + 2);
                xmm3 = _mm_load_si128(pMMSrc + 3);
                xmm4 = _mm_load_si128(pMMSrc + 4);
                xmm5 = _mm_load_si128(pMMSrc + 5);
                xmm6 = _mm_load_si128(pMMSrc + 6);
                xmm7 = _mm_load_si128(pMMSrc + 7);
                pMMSrc += 8;

                _mm_stream_si128(pMMDest, xmm0);
                _mm_stream_si128(pMMDest + 1, xmm1);
                _mm_stream_si128(pMMDest + 2, xmm2);
                _mm_stream_si128(pMMDest + 3, xmm3);
                _mm_stream_si128(pMMDest + 4, xmm4);
                _mm_stream_si128(pMMDest + 5, xmm5);
                _mm_stream_si128(pMMDest + 6, xmm6);
                _mm_stream_si128(pMMDest + 7, xmm7);
                pMMDest += 8;
            }

            count &= TIERED_TAIL;
            if (count != 0)
            {
                cnt = count >> INSTR_128_SHIFT;
                for (UINT32 i = 0; i < cnt; i += 1)
                {
                    xmm0 = _mm_load_si128(pMMSrc);
                    pMMSrc += 1;
                    _mm_stream_si128(pMMDest, xmm0);
                    pMMDest += 1;
                }
            }
        }
    }

    // handle tail copy as a fallthrough
    count &= TAIL_SIZE;
    if (count != 0)
    {
        cnt                 = count >> DWORD_SHIFT;
        DWORD*          pDst = reinterpret_cast<DWORD*>(pMMDest);
        const DWORD*    pSrc = reinterpret_cast<const DWORD*>(pMMSrc);

        for (UINT32 i = 0; i < cnt; i += 1)
        {
            *pDst    = *pSrc;
            pDst     += 1;
            pSrc     += 1;
        }

        cnt                 = count & BYTE_TAIL;
        BYTE*       bDst    = reinterpret_cast<BYTE*>(pDst);
        const BYTE* bSrc    = reinterpret_cast<const BYTE*>(pSrc);

        for (UINT32 i = 0; i < cnt; i += 1)
        {
            *bDst   = *bSrc;
            bDst    += 1;
            bSrc    += 1;
        }
    }
#else // #if defined ( _MSC_VER )
    // Linux projects do not support standard types or memcpy_s
    ::memcpy_s(dst, bytes, src, bytes);
#endif
}

/*****************************************************************************\
Inline Function:
    ScalarSwapBytes

Description:
    Helper function for MemCopySwapBytes
\*****************************************************************************/
inline void ScalarSwapBytes(
    void** dst,
    const void** src,
    const size_t byteCount,
    const unsigned int swapbytes)
{
    switch (swapbytes)
    {
    case 2:
        {
            WORD*          wDst = reinterpret_cast<WORD*>(*dst);
            const WORD*    wSrc = reinterpret_cast<const WORD*>(*src);

            for (UINT32 i = 0; i < byteCount / 2; i += 1)
            {
                WORD tmp = *wSrc;
                *wDst    = (tmp >> 8) | (tmp << 8);
                wDst     += 1;
                wSrc     += 1;
            }

            *src  = reinterpret_cast<const void*>(wSrc);
            *dst = reinterpret_cast<void*>(wDst);
        }
        break;
    case 4:
        {
            DWORD*          dwDst = reinterpret_cast<DWORD*>(*dst);
            const DWORD*    dwSrc = reinterpret_cast<const DWORD*>(*src);

            for (UINT32 i = 0; i < byteCount / 4; i += 1)
            {
                DWORD tmp = *dwSrc;
                *dwDst    = (tmp >> 24) | (tmp << 24) |
                            ((tmp & 0x0000FF00) << 8) |
                            ((tmp & 0x00FF0000) >> 8);
                dwDst     += 1;
                dwSrc     += 1;
            }

            *src  = reinterpret_cast<const void*>(dwSrc);
            *dst = reinterpret_cast<void*>(dwDst);
        }
        break;
    default:
        // should not occur
        BYTE*               bDst = reinterpret_cast<BYTE*>(*dst);
        const BYTE*         bSrc = reinterpret_cast<const BYTE*>(*src);

        ::memcpy_s(bDst, byteCount, bSrc, byteCount);

        *src = reinterpret_cast<const void*>(bSrc + byteCount);
        *dst = reinterpret_cast<void*>(bDst + byteCount);
    }
}

/*****************************************************************************\
Inline Function:
    MemCopySwapBytes

Description:
    Memory copy with swapped byte order, 2 and 4 byte elements only

Input:
    dst - pointer to write-combined destination buffer
    src - pointer to source buffer
    bytes - number of bytes to copy
    swapbytes - granularity of elements to swap
\*****************************************************************************/
inline void MemCopySwapBytes(
    void* dst,
    const void* src,
    const size_t bytes,
    const unsigned int swapbytes)
{
    // only handle 2 and 4 bytes swapping
    if (swapbytes != 2 && swapbytes != 4)
    {
        MemCopy(dst, src, bytes);
        return;
    }

#if defined(USE_SSE4_1)
    const __m128i*      pMMSrc  = reinterpret_cast<const __m128i*>(src);
    __m128i*            pMMDest = reinterpret_cast<__m128i*>(dst);
    size_t              count   = bytes;
    size_t              cnt     = 0;
    __m128i             xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

    // 2 byte shuffle
    const __m128i       wordMask = _mm_setr_epi8(
                            0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06,
                            0x09, 0x08, 0x0b, 0x0a, 0x0d, 0x0c, 0x0f, 0x0e);

    // 4 byte shuffle
    const __m128i       dwordMask = _mm_setr_epi8(
                            0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04,
                            0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c);

    // when size is < 16 rely, must use scalar swap
    if (count >= INSTR_WIDTH_128)
    {
        const __m128i shuffleMask = (swapbytes == 2) ? wordMask : dwordMask;

         // handle un-aligned tiered copy up to 2 cache lines
        if (count < 2 * CACHE_LINE_SIZE)
        {
            cnt = count >> INSTR_128_SHIFT;
            for (UINT32 i = 0; i < cnt; i += 1)
            {
                xmm0 = _mm_loadu_si128(pMMSrc);
                pMMSrc += 1;
                xmm0 = _mm_shuffle_epi8(xmm0, shuffleMask);
                _mm_storeu_si128(pMMDest, xmm0);
                pMMDest += 1;
            }
        }
        // handle aligned copy for > 2 cache lines
        else
        {
            // align destination to 16 if necessary
            UINT32 align = (UINT32)((UINT_PTR)pMMDest & TAIL_SIZE);
            if (align != 0)
            {
                align = INSTR_WIDTH_128 - align;
                cnt = align >> DWORD_SHIFT;
                ScalarSwapBytes((void**)&pMMDest, (const void**)&pMMSrc, cnt * sizeof(DWORD), swapbytes);
                cnt = align & BYTE_TAIL;

                // only words should remain, not bytes
                if (cnt > 0)
                {
                    ASSERT(cnt % 2 == 0);
                    ASSERT(swapbytes == 2);
                    ScalarSwapBytes((void**)&pMMDest, (const void**)&pMMSrc, cnt, swapbytes);
                }
            }

            count -= align; // take off the alignment from size

            // check source alignment
            if ((UINT_PTR)pMMSrc & TAIL_SIZE)
            {
                // copy un-aligned by tiers
                cnt = count >> DUAL_CACHE_SHIFT;
                for (UINT32 i = 0; i < cnt; i += 1)
                {
                    xmm0 = _mm_loadu_si128(pMMSrc);
                    xmm1 = _mm_loadu_si128(pMMSrc + 1);
                    xmm2 = _mm_loadu_si128(pMMSrc + 2);
                    xmm3 = _mm_loadu_si128(pMMSrc + 3);
                    xmm4 = _mm_loadu_si128(pMMSrc + 4);
                    xmm5 = _mm_loadu_si128(pMMSrc + 5);
                    xmm6 = _mm_loadu_si128(pMMSrc + 6);
                    xmm7 = _mm_loadu_si128(pMMSrc + 7);
                    pMMSrc += 8;

                    xmm0 = _mm_shuffle_epi8(xmm0, shuffleMask);
                    xmm1 = _mm_shuffle_epi8(xmm1, shuffleMask);
                    xmm2 = _mm_shuffle_epi8(xmm2, shuffleMask);
                    xmm3 = _mm_shuffle_epi8(xmm3, shuffleMask);
                    xmm4 = _mm_shuffle_epi8(xmm4, shuffleMask);
                    xmm5 = _mm_shuffle_epi8(xmm5, shuffleMask);
                    xmm6 = _mm_shuffle_epi8(xmm6, shuffleMask);
                    xmm7 = _mm_shuffle_epi8(xmm7, shuffleMask);

                    _mm_store_si128(pMMDest, xmm0);
                    _mm_store_si128(pMMDest + 1, xmm1);
                    _mm_store_si128(pMMDest + 2, xmm2);
                    _mm_store_si128(pMMDest + 3, xmm3);
                    _mm_store_si128(pMMDest + 4, xmm4);
                    _mm_store_si128(pMMDest + 5, xmm5);
                    _mm_store_si128(pMMDest + 6, xmm6);
                    _mm_store_si128(pMMDest + 7, xmm7);
                    pMMDest += 8;
                }

                count &= TIERED_TAIL;
                if (count != 0)
                {
                    cnt = count >> INSTR_128_SHIFT;
                    for (UINT32 i = 0; i < cnt; i += 1)
                    {
                        xmm0 = _mm_loadu_si128(pMMSrc);
                        pMMSrc += 1;
                        xmm0 = _mm_shuffle_epi8(xmm0, shuffleMask);
                        _mm_store_si128(pMMDest, xmm0);
                        pMMDest += 1;
                    }
                }
            }
            else
            {
                // copy aligned by tiers
                cnt = count >> DUAL_CACHE_SHIFT;
                for (UINT32 i = 0; i < cnt; i += 1)
                {
                    xmm0 = _mm_load_si128(pMMSrc);
                    xmm1 = _mm_load_si128(pMMSrc + 1);
                    xmm2 = _mm_load_si128(pMMSrc + 2);
                    xmm3 = _mm_load_si128(pMMSrc + 3);
                    xmm4 = _mm_load_si128(pMMSrc + 4);
                    xmm5 = _mm_load_si128(pMMSrc + 5);
                    xmm6 = _mm_load_si128(pMMSrc + 6);
                    xmm7 = _mm_load_si128(pMMSrc + 7);
                    pMMSrc += 8;

                    xmm0 = _mm_shuffle_epi8(xmm0, shuffleMask);
                    xmm1 = _mm_shuffle_epi8(xmm1, shuffleMask);
                    xmm2 = _mm_shuffle_epi8(xmm2, shuffleMask);
                    xmm3 = _mm_shuffle_epi8(xmm3, shuffleMask);
                    xmm4 = _mm_shuffle_epi8(xmm4, shuffleMask);
                    xmm5 = _mm_shuffle_epi8(xmm5, shuffleMask);
                    xmm6 = _mm_shuffle_epi8(xmm6, shuffleMask);
                    xmm7 = _mm_shuffle_epi8(xmm7, shuffleMask);

                    _mm_store_si128(pMMDest, xmm0);
                    _mm_store_si128(pMMDest + 1, xmm1);
                    _mm_store_si128(pMMDest + 2, xmm2);
                    _mm_store_si128(pMMDest + 3, xmm3);
                    _mm_store_si128(pMMDest + 4, xmm4);
                    _mm_store_si128(pMMDest + 5, xmm5);
                    _mm_store_si128(pMMDest + 6, xmm6);
                    _mm_store_si128(pMMDest + 7, xmm7);
                    pMMDest += 8;
                }

                count &= TIERED_TAIL;
                if (count != 0)
                {
                    cnt = count >> INSTR_128_SHIFT;
                    for (UINT32 i = 0; i < cnt; i += 1)
                    {
                        xmm0 = _mm_load_si128(pMMSrc);
                        pMMSrc += 1;
                        xmm0 = _mm_shuffle_epi8(xmm0, shuffleMask);
                        _mm_store_si128(pMMDest, xmm0);
                        pMMDest += 1;
                    }
                }
            }
        }

        // handle tail copy as a fallthrough
        count &= TAIL_SIZE;
        if (count != 0)
        {
            cnt = count >> DWORD_SHIFT;
            ScalarSwapBytes((void**)&pMMDest, (const void**)&pMMSrc, cnt * sizeof(DWORD), swapbytes);
            cnt = count & BYTE_TAIL;

            // only words should remain, not bytes
            if (cnt > 0)
            {
                ASSERT(cnt % 2 == 0);
                ASSERT(swapbytes == 2);
                ScalarSwapBytes((void**)&pMMDest, (const void**)&pMMSrc, cnt, swapbytes);
            }
        }
        return;
    }
#endif // defined(USE_SSE4_1)
    ScalarSwapBytes(&dst, &src, bytes, swapbytes);
}

/*****************************************************************************\
Inline Function:
    SafeMemSet

Description:
    Exception Handler Memory Set function
\*****************************************************************************/
inline void SafeMemSet( void* dst, const int data, const size_t bytes )
{
#if defined(_DEBUG) && defined(ISTDLIB_KMD)
    __try
#endif
    {
        ::memset( dst, data, bytes );
    }
#if defined(_DEBUG) && defined(ISTDLIB_KMD)
    // catch exceptions here so they are easily debugged
    __except(1)
    {
        ASSERT(0);
    }
#endif
}

/*****************************************************************************\
Inline Function:
    SafeMemCompare

Description:
    Exception Handler Memory Compare function
\*****************************************************************************/
inline int SafeMemCompare( const void* dst, const void* src, const size_t bytes )
{
#if defined(_DEBUG) && defined(ISTDLIB_KMD)
    __try
#endif
    {
        return ::memcmp( dst, src, bytes );
    }
#if defined(_DEBUG) && defined(ISTDLIB_KMD)
    // catch exceptions here so they are easily debugged
    __except(1)
    {
        ASSERT(0);
        return -1;
    }
#endif
}

/*****************************************************************************\
Inline Function:
    SafeMemMove

Description:
    copies "bytes" of data from src to dst.
    dst is not corrupted if src and dst blocks of data overlap.

Input:
    dst   - pointer to destination buffer
    src   - pointer to source buffer
    bytes - number of bytes to copy
\*****************************************************************************/
inline void SafeMemMove( void *dst, const void *src, const size_t bytes )
{
    if( dst!=src )
    {
        if( src>dst && bytes )
        {
            size_t t = 0;
            do
            {
                static_cast< unsigned char* >( dst )[t] = static_cast< const unsigned char* >( src )[t];
            }
            while( ++t != bytes );
        }
        else
        {
            size_t t = bytes-1;
            do
            {
                static_cast< unsigned char* >( dst )[t] = static_cast< const unsigned char* >( src )[t];
            }
            while( t-- != 0 );
        }
    }
}

#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
/*****************************************************************************\
MACROS:
    EMIT_R_MR
    Example:  movntdqa xmm1, xmmword ptr [eax]

    EMIT_R_MR_OFFSET
    Example: movntdqa xmm1, xmmword ptr [eax + 0x10]

Description:
    Used to encode SSE4.1 instructions with parametrs
\*****************************************************************************/
#define EMIT_R_MR(OPCODE, X, Y )   \
    OPCODE                         \
    __asm _emit (0x00 + X*8 + Y)

#define EMIT_R_MR_OFFSET(OPCODE, X, Y, OFFSET)  \
    OPCODE                                      \
    __asm _emit (0x80 + X*8 + Y)                \
    __asm _emit (OFFSET&0xFF)                   \
    __asm _emit ((OFFSET>>8)&0xFF)              \
    __asm _emit ((OFFSET>>16)&0xFF)             \
    __asm _emit ((OFFSET>>24)&0xFF)

/*****************************************************************************\
MACROS:
    REG_XXX

Description:
    Define CPU General Purpose and XMM Register Indices
    These MACROS are to be replaced with instrinics available with .NET 2008
\*****************************************************************************/
#if defined( _MSC_VER )
#define REG_EAX  0x00
#define REG_ECX  0x01
#define REG_EDX  0x02
#define REG_EBX  0x03
#define REG_ESP  0x04
#define REG_EBP  0x05
#define REG_ESI  0x06
#define REG_EDI  0x07
#define REG_XMM0 0x00
#define REG_XMM1 0x01
#define REG_XMM2 0x02
#define REG_XMM3 0x03
#define REG_XMM4 0x04
#define REG_XMM5 0x05
#define REG_XMM6 0x06
#define REG_XMM7 0x07
#endif //#if defined( _MSC_VER )

/*****************************************************************************\
MACROS:
    MOVNTDQA_OP
    MOVNTDQA_R_MR
    MOVNTDQA_R_MRB

Description:
    Used to emit SSE4_1 movntdqa (streaming load) instructions
        SRC - XMM Register, destination data is to be stored
        DST - General Purpose Register containing source address
        OFFSET - Offset to be added to the source address
\*****************************************************************************/
#define MOVNTDQA_OP     \
    _asm _emit 0x66     \
    _asm _emit 0x0F     \
    _asm _emit 0x38     \
    _asm _emit 0x2A

#define MOVNTDQA_R_MR(DST, SRC)                 \
    EMIT_R_MR(MOVNTDQA_OP, DST, SRC)

#define MOVNTDQA_R_MR_OFFSET(DST, SRC, OFFSET)  \
    EMIT_R_MR_OFFSET(MOVNTDQA_OP, DST, SRC, OFFSET)

/*****************************************************************************\
Inline Function:
    FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqa

Description: Fast copy from USWC memory to cacheable system memory

Input:
    dst - 16-byte aligned pointer to (cacheable) destination buffer
    src - 16-byte(req)/64-byte(optimal) aligned pointer to (USWC) source buffer
\*****************************************************************************/
__forceinline void __fastcall FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqa( void* dst, const void* src )
{

    __asm
    {
        ;Store the orginal source start address
        mov edx, src

        ;Store the dest address
        mov ecx, dst

        align 16

        ; Load data from source buffer
        ; Streaming loads from the same cache line should be grouped together
        ; and not be interleaved with: a) Writes or non-streaming loads or
        ; b) Streaming loads from other cache lines (strided accesses)

        ; movntdqa xmm0, xmmword ptr [edx]
        MOVNTDQA_R_MR(REG_XMM0, REG_EDX)

        ; movntdqa xmm1, xmmword ptr [edx+16]
        MOVNTDQA_R_MR_OFFSET(REG_XMM1, REG_EDX, 16)

        ; movntdqa xmm2, xmmword ptr [edx+32]
        MOVNTDQA_R_MR_OFFSET(REG_XMM2, REG_EDX, 32)

        ; movntdqa xmm3, xmmword ptr [edx+48]
        MOVNTDQA_R_MR_OFFSET(REG_XMM3, REG_EDX, 48)

        ; Save data in destination buffer.
        movdqa xmmword ptr [ecx], xmm0
        movdqa xmmword ptr [ecx+16], xmm1
        movdqa xmmword ptr [ecx+32], xmm2
        movdqa xmmword ptr [ecx+48], xmm3
    }

} // FastMemCopy_SSE4_1_movntdqa_movdqa()

/*****************************************************************************\
Inline Function:
    FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqu

Description: Fast copy from USWC memory (DHWORD in size) to cacheable system memory

Input:
    dst - 16-byte (unaligned) pointer to (cacheable) destination buffer
    src - 16-byte(req)/64-byte(optimal) aligned pointer to (USWC) source buffer
\*****************************************************************************/
__forceinline void  __fastcall FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqu(void* dst, const void* src )
{
    __asm
    {
        ;Store the orginal source start address
        mov edx, src

        ;Store the dest address
        mov ecx, dst

        align 16

        ; Load data from source buffer
        ; Streaming loads from the same cache line should be grouped together
        ; and not be interleaved with: a) Writes or non-streaming loads or
        ; b) Streaming loads from other cache lines (strided accesses)

        ; movntdqa xmm0, xmmword ptr [edx]
        MOVNTDQA_R_MR(REG_XMM0, REG_EDX)

        ; movntdqa xmm1, xmmword ptr [edx+16]
        MOVNTDQA_R_MR_OFFSET(REG_XMM1, REG_EDX, 16)

        ; movntdqa xmm2, xmmword ptr [edx+32]
        MOVNTDQA_R_MR_OFFSET(REG_XMM2, REG_EDX, 32)

        ; movntdqa xmm3, xmmword ptr [edx+48]
        MOVNTDQA_R_MR_OFFSET(REG_XMM3, REG_EDX, 48)

        ; Copy data in destination buffer.
        movdqu xmmword ptr [ecx], xmm0
        movdqu xmmword ptr [ecx+16], xmm1
        movdqu xmmword ptr [ecx+32], xmm2
        movdqu xmmword ptr [ecx+48], xmm3
    }
} // FastMemCopy_SSE4_1_movntdqa_movdqu()
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1


inline void FastMemCopyFromWC( void* dst, const void* src, const size_t bytes, CPU_INSTRUCTION_LEVEL cpuInstructionLevel )
{
    // Cache pointers to memory
    BYTE* p_dst = (BYTE*)dst;
    BYTE* p_src = (BYTE*)src;

    size_t count = bytes;

#if defined(USE_SSE4_1)
    if( cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1 )
    {

        if( count >= sizeof(DHWORD) )
        {
            //Streaming Load must be 16-byte aligned but should
            //be 64-byte aligned for optimal performance
            const size_t doubleHexWordAlignBytes =
                GetAlignmentOffset( p_src, sizeof(DHWORD) );

            // Copy portion of the source memory that is not aligned
            if( doubleHexWordAlignBytes )
            {
                MemCopy( p_dst, p_src, doubleHexWordAlignBytes );

                p_dst += doubleHexWordAlignBytes;
                p_src += doubleHexWordAlignBytes;
                count -= doubleHexWordAlignBytes;
            }

            ASSERT( IsAligned( p_src, sizeof(DHWORD) ) == true );

            // Get the number of bytes to be copied (rounded down to nearets DHWORD)
            const size_t DoubleHexWordsToCopy = count / sizeof(DHWORD);

            if( DoubleHexWordsToCopy )
            {
                // Determine if the destination address is aligned
                const bool isDstDoubleQuadWordAligned =
                    IsAligned( p_dst, sizeof(DQWORD) );

#if !defined(USE_INLINE_ASM) || USE_INLINE_ASM == 0
                __m128i* pMMSrc = (__m128i*)(p_src);
                __m128i* pMMDest = reinterpret_cast<__m128i*>(p_dst);
                __m128i  xmm0, xmm1, xmm2, xmm3;
#endif // !defined(USE_INLINE_ASM) || USE_INLINE_ASM == 0

                if( isDstDoubleQuadWordAligned )
                {
#if defined(__GNUC__)
                    // Sync the WC memory data before issuing the MOVNTDQA instruction.
                    _mm_mfence();
#endif
                    for( size_t i=0; i<DoubleHexWordsToCopy; i++ )
                    {

#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
                        FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqa( p_dst, p_src );
#else // !(defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1)
                        xmm0 = _mm_stream_load_si128(pMMSrc);
                        xmm1 = _mm_stream_load_si128(pMMSrc + 1);
                        xmm2 = _mm_stream_load_si128(pMMSrc + 2);
                        xmm3 = _mm_stream_load_si128(pMMSrc + 3);
                        pMMSrc += 4;

                        _mm_store_si128(pMMDest, xmm0);
                        _mm_store_si128(pMMDest + 1, xmm1);
                        _mm_store_si128(pMMDest + 2, xmm2);
                        _mm_store_si128(pMMDest + 3, xmm3);
                        pMMDest += 4;
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1

                        p_dst += sizeof(DHWORD);
                        p_src += sizeof(DHWORD);
                        count -= sizeof(DHWORD);
                    }
                }
                else
                {
#if defined(__GNUC__)
                    // Sync the WC memory data before issuing the MOVNTDQA instruction.
                    _mm_mfence();
#endif
                    for( size_t i=0; i<DoubleHexWordsToCopy; i++ )
                    {

#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
                        FastBlockCopyFromUSWC_SSE4_1_movntdqa_movdqu( p_dst, p_src );
#else // !(defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1)
                        xmm0 = _mm_stream_load_si128(pMMSrc);
                        xmm1 = _mm_stream_load_si128(pMMSrc + 1);
                        xmm2 = _mm_stream_load_si128(pMMSrc + 2);
                        xmm3 = _mm_stream_load_si128(pMMSrc + 3);
                        pMMSrc += 4;

                        _mm_storeu_si128(pMMDest, xmm0);
                        _mm_storeu_si128(pMMDest + 1, xmm1);
                        _mm_storeu_si128(pMMDest + 2, xmm2);
                        _mm_storeu_si128(pMMDest + 3, xmm3);
                        pMMDest += 4;
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1

                        p_dst += sizeof(DHWORD);
                        p_src += sizeof(DHWORD);
                        count -= sizeof(DHWORD);
                    }
                }
            }
        }
    }
#endif // defined(USE_SSE4_1)
    // Copy remaining BYTE(s)
    if( count )
    {
        MemCopy( p_dst, p_src, count );
    }
}

/*****************************************************************************\
Inline Function:
    FastCpuBlt

Description:
    Intel C++ Compiler CPU Blit function

Parameters:
    BYTE* dst - destination pointer
    const DWORD dstPitch - pitch to increment destination pointer per count
    BYTE* src - source pointer
    const DWORD srcPitch - pitch to increment source pointer per count
    const DWORD stride - stride of data to copy per count, in bytes
    DWORD count - number of iterations to copy data

\*****************************************************************************/
inline void FastCpuBlt(
    BYTE* dst,
    const DWORD dstPitch,
    BYTE* src,
    const DWORD srcPitch,
    const DWORD stride,
    DWORD count )
{
    do
    {
        MemCopy( dst, src, stride );

        dst += dstPitch;
        src += srcPitch;
    }
    while( --count > 0 );
}

/*****************************************************************************\
Inline Function:
    FastCpuSet

Description:
    Intel C++ Compiler CPU Blit function

Parameters:
    BYTE* dst - destination pointer
    const DWORD dstPitch - pitch to increment destination pointer per count
    BYTE* src - source pointer
    const DWORD srcPitch - pitch to increment source pointer per count
    const DWORD stride - stride of data to copy per count, in bytes
    DWORD count - number of iterations to copy data

\*****************************************************************************/
inline void FastCpuSet(
    BYTE* dst,
    const DWORD dstPitch,
    const DWORD value,
    const DWORD stride,
    DWORD count )
{
    do
    {
        SafeMemSet( dst, value, stride );

        dst += dstPitch;
    }
    while( --count > 0 );
}

/*****************************************************************************\
Inline Function:
    FastCpuBltFromUSWC

Description:
    Intel C++ Compiler CPU Blit function from non-temporal to temporal memory
    This function is optimized using SSE4 instructions which use accelerated write-combined
    loads that bypass the cache.

Parameters:
    BYTE* dst - destination pointer (temporal)
    const DWORD dstPitch - pitch to increment destination pointer per count
    BYTE* src - source pointer (non-temporal)
    const DWORD srcPitch - pitch to increment source pointer per count
    const DWORD stride - stride of data to copy per count, in bytes
    DWORD count - number of iterations to copy data
    CPU_INSTRUCTION_LEVEL level - cpu instruction level (SSE support level)

\*****************************************************************************/
#if defined ( _MSC_VER )
inline void FastCpuBltFromUSWC(
    BYTE* dst,
    const DWORD dstPitch,
    BYTE* src,
    const DWORD srcPitch,
    const DWORD stride,
    DWORD count,
    CPU_INSTRUCTION_LEVEL level)
{
#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
    //back up the XMM registers just in case
     __declspec( align(16) ) BYTE backUpRegisters[16*4];

     void *tempPtr = (void *) backUpRegisters;

    __asm mov ecx, tempPtr
    __asm movdqa xmmword ptr [ecx + 16*0], xmm0
    __asm movdqa xmmword ptr [ecx + 16*1], xmm1
    __asm movdqa xmmword ptr [ecx + 16*2], xmm2
    __asm movdqa xmmword ptr [ecx + 16*3], xmm3
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1

    do
    {
        iSTD::FastMemCopyFromWC( dst, src, stride, level );

        dst += dstPitch;
        src += srcPitch;
    }
    while( --count > 0 );
#if defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
    __asm mov ecx, tempPtr
    __asm movdqa xmm0, xmmword ptr [ecx + 16*0]
    __asm movdqa xmm1, xmmword ptr [ecx + 16*1]
    __asm movdqa xmm2, xmmword ptr [ecx + 16*2]
    __asm movdqa xmm3, xmmword ptr [ecx + 16*3]
#endif // defined(USE_INLINE_ASM) && USE_INLINE_ASM == 1
}
#endif


/*****************************************************************************\
Inline Function:
    FindWordBufferMinMax

Description:
    Finds the min and max unsigned 16-bit values in the buffer

Input:
    WORD* pBuffer - pointer to 16-bit buffer
    const DWORD bytes - size of buffer in bytes

Output:
    WORD &min - minimum 16-bit value
    WORD &max - maximum 16-bit value

\*****************************************************************************/
inline void FindWordBufferMinMax(
    WORD* pBuffer,
    const DWORD bytes,
    WORD &min,
    WORD &max )
{
    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    WORD wValue = 0;
    WORD wMinValue = 0xffff;
    WORD wMaxValue = 0x0000;

    size_t count = bytes / sizeof(WORD);

#if defined(USE_SSE4_1)
    size_t i = 0;

    if( IsAligned( pBuffer, sizeof(WORD) ) )
    {
        const size_t DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const size_t WordsPerPrefetch            = sizeof(PREFETCH) / sizeof(WORD);
        const size_t WordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= WordsPerDoubleQuadWord )
        {
            const size_t doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(WORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pBuffer++;

                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= WordsPerDoubleQuadWord )
            {
                __m128i mValue128i;

                // Need to convert unsigned values to signed values
                // since min/max is signed op
                __m128i mSignedScale128i = _mm_set1_epi16((WORD)0x8000);

                // Signed min/max initialization
                __m128i mMinValue128i    = _mm_set1_epi16(wMinValue-(WORD)0x8000);
                __m128i mMaxValue128i    = _mm_set1_epi16(wMaxValue-(WORD)0x8000);

                while( count >= WordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= WordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mValue128i = *(__m128i*)pBuffer;
                        pBuffer += WordsPerDoubleQuadWord;

                        // Make values signed
                        mValue128i = _mm_sub_epi16( mValue128i,
                            mSignedScale128i );

                        // Determine parallel min/max
                        mMinValue128i = _mm_min_epi16( mMinValue128i,
                            mValue128i );
                        mMaxValue128i = _mm_max_epi16( mMaxValue128i,
                            mValue128i );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= WordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= WordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mValue128i = *(__m128i*)pBuffer;
                    pBuffer += WordsPerDoubleQuadWord;

                    // Make values signed
                    mValue128i = _mm_sub_epi16( mValue128i,
                        mSignedScale128i );

                    // Determine parallel min/max
                    mMinValue128i = _mm_min_epi16( mMinValue128i,
                        mValue128i );
                    mMaxValue128i = _mm_max_epi16( mMaxValue128i,
                        mValue128i );
                }

                // Determine wMinValue

                // Make values unsigned
                mMinValue128i = _mm_add_epi16( mMinValue128i,
                    mSignedScale128i );

                // Extract each value in double-quadword to find minimum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 0 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 1 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 2 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 3 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 4 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 5 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 6 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 7 );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Make values unsigned
                mMaxValue128i = _mm_add_epi16( mMaxValue128i,
                    mSignedScale128i );

                // Extract each value in double-quadword to find maximum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 0 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 1 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 2 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 3 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 4 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 5 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 6 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 7 );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= WordsPerDoubleQuadWord )
        } // if( count >= WordsPerDoubleQuadWord )
    }
#ifndef _WIN64
    else // if( IsAligned( pBuffer, sizeof(WORD) ) )
    {
        const size_t QuadWordsPerCacheline   = sizeof(CACHELINE) / sizeof(QWORD);
        const size_t WordsPerCacheline       = sizeof(CACHELINE) / sizeof(WORD);
        const size_t WordsPerQuadWord        = sizeof(QWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(CACHELINE) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(CACHELINE) );

        if( count >= WordsPerQuadWord )
        {
            __m64   mValue64;

            // Need to convert unsigned values to signed values
            // since min/max is signed op
            __m64   mSignedScale64  = _mm_set1_pi16((WORD)0x8000);

            // Signed min/max initialization
            __m64   mMinValue64     = _mm_set1_pi16(wMinValue-(WORD)0x8000);
            __m64   mMaxValue64     = _mm_set1_pi16(wMaxValue-(WORD)0x8000);

            // Find min/max per cacheline of values
            while( count >= WordsPerCacheline )
            {
                Prefetch( (BYTE*)pBuffer + sizeof(CACHELINE) );

                // Process cacheline values per pass
                count -= WordsPerCacheline;

                for( i = 0; i < QuadWordsPerCacheline; i++ )
                {
                    // Get quadword values
                    mValue64 = *(__m64*)pBuffer;
                    pBuffer += WordsPerQuadWord;

                    // Make values signed
                    mValue64 = _mm_sub_pi16( mValue64, mSignedScale64 );

                    // Determine parallel min/max
                    mMinValue64 = _mm_min_pi16( mMinValue64, mValue64 );
                    mMaxValue64 = _mm_max_pi16( mMaxValue64, mValue64 );
                }
            }

            // Process quadword values per pass for remainder
            while( count >= WordsPerQuadWord )
            {
                // Process quadword values per pass
                count -= WordsPerQuadWord;

                // Get quadword values
                mValue64 = *(__m64*)pBuffer;
                pBuffer += WordsPerQuadWord;

                // Make values signed
                mValue64 = _mm_sub_pi16( mValue64, mSignedScale64 );

                // Determine parallel min/max
                mMinValue64 = _mm_min_pi16( mMinValue64, mValue64 );
                mMaxValue64 = _mm_max_pi16( mMaxValue64, mValue64 );
            }

            // Determine wMinValue

            // Make values unsigned
            mMinValue64 = _mm_add_pi16( mMinValue64, mSignedScale64 );

            // Extract each value in quadword to find minimum
            // for( i = 0; i < WordsPerQuadWord; i++ )
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 0 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 1 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 2 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 3 );
            wMinValue = Min( wMinValue, wValue );

            // Determine wMaxValue

            // Make values unsigned
            mMaxValue64 = _mm_add_pi16( mMaxValue64, mSignedScale64 );

            // Extract each value in quadword to find maximum
            // for( i = 0; i < WordsPerQuadWord; i++ )
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 0 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 1 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 2 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 3 );
            wMaxValue = Max( wMaxValue, wValue );

            _mm_empty();

        } // if( count >= WordsPerQuadWord )
    }
#endif // _WIN64
#endif // defined(USE_SSE4_1)

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pBuffer++;

        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


/*****************************************************************************\
Inline Function:
    FindWordBufferMinMaxRestart

Description:
    Finds the min and max unsigned 32-bit values in the buffer
    Excludes a restart value from min or max values

Input:
    WORD* pBuffer - pointer to 32-bit buffer
    const DWORD bytes - size of buffer in bytes
    const WORD restart - restart index to ignore
    cpuInstructionLevel - indicates if SSE_4.1 is available

Output:
    WORD &min - minimum 32-bit value
    WORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindWordBufferMinMaxRestart(
    WORD* pBuffer,
    const DWORD bytes,
    const WORD restart,
    WORD &min,
    WORD &max,
    CPU_INSTRUCTION_LEVEL cpuInstructionLevel )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    WORD wValue = 0;
    WORD wMinValue = 0xffff;
    WORD wMaxValue = 0x0000;

    size_t count = bytes / sizeof(WORD);

#ifdef USE_SSE4_1

    size_t i = 0;

    if( IsAligned( pBuffer, sizeof(WORD) ) &&
        cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1 )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD WordsPerPrefetch            = sizeof(PREFETCH) / sizeof(WORD);
        const DWORD WordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= WordsPerDoubleQuadWord )
        {
            const size_t doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(WORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pBuffer++;

                    if (wValue == restart) {
                        continue;
                    }
                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= WordsPerDoubleQuadWord )
            {
                __m128i mInput, mRestarts, mMask;
                __m128i mAll_ones = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMinValue128i = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMaxValue128i = _mm_setzero_si128();

                // Initialize register used for testing for restart index.
                mRestarts = _mm_setr_epi16(restart, restart, restart, restart, restart, restart, restart, restart);

                while( count >= WordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= WordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mInput = *(__m128i*)pBuffer;
                        pBuffer += WordsPerDoubleQuadWord;

                        // Make mask of non-restart_index fields
                        mMask = _mm_andnot_si128(_mm_cmpeq_epi16(mInput, mRestarts), mAll_ones);

                        // Copy minimum and maximum fields for non-restarts
                        mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu16(mMinValue128i, mInput), mMask );
                        mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu16(mMaxValue128i, mInput), mMask );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= WordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= WordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mInput = *(__m128i*)pBuffer;
                    pBuffer += WordsPerDoubleQuadWord;

                    // Make mask of non-restart_index fields
                    mMask = _mm_andnot_si128(_mm_cmpeq_epi16(mInput, mRestarts), mAll_ones);

                    // Copy minimum and maximum fields for non-restarts
                    mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu16(mMinValue128i, mInput), mMask );
                    mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu16(mMaxValue128i, mInput), mMask );
                }

                // Determine wMinValue

                // Extract each value in double-quadword to find minimum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 0 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 1 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 2 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 3 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 4 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 5 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 6 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 7 );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Extract each value in double-quadword to find maximum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 0 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 1 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 2 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 3 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 4 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 5 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 6 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 7 );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= WordsPerDoubleQuadWord )
        } // if( count >= WordsPerDoubleQuadWord )
    }

#endif // USE_SSE4_1

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pBuffer++;

        if (wValue == restart) {
            continue;
        }
        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


/*****************************************************************************\
Inline Function:
    FindDWordBufferMinMax

Description:
    Finds the min and max unsigned 32-bit values in the buffer

Input:
    DWORD* pBuffer - pointer to 32-bit buffer
    const DWORD bytes - size of buffer in bytes

Output:
    DWORD &min - minimum 32-bit value
    DWORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindDWordBufferMinMax(
    DWORD* pBuffer,
    const DWORD bytes,
    DWORD &min,
    DWORD &max )
{
    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    DWORD wValue = 0;
    DWORD wMinValue = 0xffffffff;
    DWORD wMaxValue = 0x00000000;

    DWORD count = bytes / sizeof(DWORD);

#if defined(USE_SSE4_1)
    DWORD i = 0;

    if( IsAligned( pBuffer, sizeof(DWORD) ) )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD DWordsPerPrefetch            = sizeof(PREFETCH) / sizeof(DWORD);
        const DWORD DWordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(DWORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= DWordsPerDoubleQuadWord )
        {
            const DWORD doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(DWORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pBuffer++;

                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= DWordsPerPrefetch )
            {
                __m128i mValue128i;
                __m128 mValue128;

                // Signed min/max initialization
                // need extra QWORD bits for SSE2 FP conversion
                __m128  mMinValue128 = _mm_set1_ps( (float)( (QWORD)wMinValue  ) );
                __m128  mMaxValue128 = _mm_set1_ps( (float)( wMaxValue  ) );

                while( count >= DWordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= DWordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mValue128i = *(__m128i*)pBuffer;
                        pBuffer += DWordsPerDoubleQuadWord;

                        // Convert to FP
                        mValue128 = _mm_cvtepi32_ps( mValue128i );

                        // Determine parallel min/max
                        mMinValue128 = _mm_min_ps( mMinValue128,
                            mValue128 );
                        mMaxValue128 = _mm_max_ps( mMaxValue128,
                            mValue128 );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= DWordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= DWordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mValue128i = *(__m128i*)pBuffer;
                    pBuffer += DWordsPerDoubleQuadWord;

                    // Convert to FP
                    mValue128 = _mm_cvtepi32_ps( mValue128i );

                    // Determine parallel min/max
                    mMinValue128 = _mm_min_ps( mMinValue128,
                        mValue128 );
                    mMaxValue128 = _mm_max_ps( mMaxValue128,
                        mValue128 );
                }

                // Determine wMinValue

                // Convert back to DWORD
                __m128i mMinValue128i = _mm_cvtps_epi32( mMinValue128 );

                // Extract each value in double-quadword to find minimum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMinValue128i );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 4 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 8 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 12 ) );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Convert back to DWORD
                __m128i mMaxValue128i = _mm_cvtps_epi32( mMaxValue128 );

                // Extract each value in double-quadword to find maximum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMaxValue128i );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 4 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 8 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 3 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 12 ) );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= DWordsPerDoubleQuadWord )
        } // if( count >= DWordsPerDoubleQuadWord )
    }
#endif // defined(USE_SSE4_1)

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pBuffer++;

        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


/*****************************************************************************\
Inline Function:
    FindDWordBufferMinMaxRestart

Description:
    Finds the min and max unsigned 32-bit values in the buffer
    Excludes a restart value from min or max values

Input:
    DWORD* pBuffer - pointer to 32-bit buffer
    const DWORD bytes - size of buffer in bytes
    const DWORD restart - restart index to ignore
    cpuInstructionLevel - indicates if SSE_4.1 is available

Output:
    DWORD &min - minimum 32-bit value
    DWORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindDWordBufferMinMaxRestart(
    DWORD* pBuffer,
    const DWORD bytes,
    const DWORD restart,
    DWORD &min,
    DWORD &max,
    CPU_INSTRUCTION_LEVEL cpuInstructionLevel )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    DWORD wValue = 0;
    DWORD wMinValue = 0xffffffff;
    DWORD wMaxValue = 0x00000000;

    DWORD count = bytes / sizeof(DWORD);

#ifdef USE_SSE4_1

    DWORD i = 0;

    if( IsAligned( pBuffer, sizeof(DWORD) ) &&
        cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1 )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD DWordsPerPrefetch            = sizeof(PREFETCH) / sizeof(DWORD);
        const DWORD DWordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(DWORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= DWordsPerDoubleQuadWord )
        {
            const DWORD doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(DWORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pBuffer++;

                    if (wValue == restart) {
                        continue;
                    }
                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= DWordsPerPrefetch )
            {
                __m128i mInput, mRestarts, mMask;
                __m128i mAll_ones = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMinValue128i = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMaxValue128i = _mm_setzero_si128();

                // Initialize register used for testing for restart index.
                mRestarts = _mm_setr_epi32(restart, restart, restart, restart);

                while( count >= DWordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= DWordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                      // Get double-quadword values
                        mInput = *(__m128i*)pBuffer;
                        pBuffer += DWordsPerDoubleQuadWord;
                       // Make mask of non-restart_index fields
                      mMask = _mm_andnot_si128(_mm_cmpeq_epi32(mInput, mRestarts), mAll_ones);

                        // Copy minimum and maximum fields for non-restarts
                        mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu32(mMinValue128i, mInput), mMask );
                        mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu32(mMaxValue128i, mInput), mMask );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= DWordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= DWordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mInput = *(__m128i*)pBuffer;
                    pBuffer += DWordsPerDoubleQuadWord;

                    // Make mask of non-restart_index fields
                    mMask = _mm_andnot_si128(_mm_cmpeq_epi32(mInput, mRestarts), mAll_ones);

                    // Copy minimum and maximum fields for non-restarts
                    mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu32(mMinValue128i, mInput), mMask );
                    mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu32(mMaxValue128i, mInput), mMask );
                }

                // Determine wMinValue

                // Extract each value in double-quadword to find minimum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMinValue128i );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 4 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 8 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 12 ) );
                wMinValue = Min( wMinValue, wValue );
                // Determine wMaxValue
                // Extract each value in double-quadword to find maximum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMaxValue128i );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 4 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 8 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 3 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 12 ) );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= DWordsPerPrefetch )
        } // if( count >= DWordsPerDoubleQuadWord )
    }

#endif // USE_SSE4_1

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pBuffer++;

        if (wValue == restart) {
            continue;
        }
        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}



/*****************************************************************************\
 Inline Function:
    FindWordBufferMinMaxCopy

Description:
    Finds the min and max unsigned 16-bit values in the buffer
    Copies data from pBuffer to pDest at the same time

Input:
    WORD* pDest - pointer to 16-bit buffer to copy into
    WORD* pBuffer - pointer to 16-bit index buffer
    const DWORD bytes - size of buffer in bytes

Output:
    WORD &min - minimum 16-bit value
    WORD &max - maximum 16-bit value

\*****************************************************************************/
inline void FindWordBufferMinMaxCopy(
    WORD* pDest,
    WORD* pBuffer,
    const DWORD bytes,
    WORD &min,
    WORD &max )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    WORD wValue = 0;
    WORD wMinValue = 0xffff;
    WORD wMaxValue = 0x0000;

    size_t count = bytes / sizeof(WORD);

#if defined(USE_SSE4_1)
    size_t i = 0;

    if( IsAligned( pBuffer, sizeof(WORD) ) )
    {
        const size_t DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const size_t WordsPerPrefetch            = sizeof(PREFETCH) / sizeof(WORD);
        const size_t WordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= WordsPerDoubleQuadWord )
        {
            const size_t doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(WORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pDest++ = *pBuffer++;

                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= WordsPerDoubleQuadWord )
            {
                __m128i mValue128i;

                // Need to convert unsigned values to signed values
                // since min/max is signed op
                __m128i mSignedScale128i = _mm_set1_epi16((WORD)0x8000);

                // Signed min/max initialization
                __m128i mMinValue128i    = _mm_set1_epi16(wMinValue-(WORD)0x8000);
                __m128i mMaxValue128i    = _mm_set1_epi16(wMaxValue-(WORD)0x8000);

                while( count >= WordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= WordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mValue128i = *(__m128i*)pBuffer;
                        _mm_storeu_si128((__m128i*)pDest, mValue128i);
                        pBuffer += WordsPerDoubleQuadWord;
                        pDest += WordsPerDoubleQuadWord;

                        // Make values signed
                        mValue128i = _mm_sub_epi16( mValue128i,
                            mSignedScale128i );

                        // Determine parallel min/max
                        mMinValue128i = _mm_min_epi16( mMinValue128i,
                            mValue128i );
                        mMaxValue128i = _mm_max_epi16( mMaxValue128i,
                            mValue128i );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= WordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= WordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mValue128i = *(__m128i*)pBuffer;
                    _mm_storeu_si128((__m128i*)pDest, mValue128i);
                    pBuffer += WordsPerDoubleQuadWord;
                    pDest += WordsPerDoubleQuadWord;

                    // Make values signed
                    mValue128i = _mm_sub_epi16( mValue128i,
                        mSignedScale128i );

                    // Determine parallel min/max
                    mMinValue128i = _mm_min_epi16( mMinValue128i,
                        mValue128i );
                    mMaxValue128i = _mm_max_epi16( mMaxValue128i,
                        mValue128i );
                }

                // Determine wMinValue

                // Make values unsigned
                mMinValue128i = _mm_add_epi16( mMinValue128i,
                    mSignedScale128i );

                // Extract each value in double-quadword to find minimum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 0 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 1 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 2 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 3 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 4 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 5 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 6 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 7 );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Make values unsigned
                mMaxValue128i = _mm_add_epi16( mMaxValue128i,
                    mSignedScale128i );

                // Extract each value in double-quadword to find maximum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 0 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 1 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 2 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 3 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 4 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 5 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 6 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 7 );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= WordsPerDoubleQuadWord )
        } // if( count >= WordsPerDoubleQuadWord )
    }
#ifndef _WIN64
    else // if( IsAligned( pBuffer, sizeof(WORD) ) )
    {
        const size_t QuadWordsPerCacheline   = sizeof(CACHELINE) / sizeof(QWORD);
        const size_t WordsPerCacheline       = sizeof(CACHELINE) / sizeof(WORD);
        const size_t WordsPerQuadWord        = sizeof(QWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(CACHELINE) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(CACHELINE) );

        if( count >= WordsPerQuadWord )
        {
            __m64   mValue64;

            // Need to convert unsigned values to signed values
            // since min/max is signed op
            __m64   mSignedScale64  = _mm_set1_pi16((WORD)0x8000);

            // Signed min/max initialization
            __m64   mMinValue64     = _mm_set1_pi16(wMinValue-(WORD)0x8000);
            __m64   mMaxValue64     = _mm_set1_pi16(wMaxValue-(WORD)0x8000);

            // Find min/max per cacheline of values
            while( count >= WordsPerCacheline )
            {
                Prefetch( (BYTE*)pBuffer + sizeof(CACHELINE) );

                // Process cacheline values per pass
                count -= WordsPerCacheline;

                for( i = 0; i < QuadWordsPerCacheline; i++ )
                {
                    // Get quadword values
                    mValue64 = *(__m64*)pBuffer;
                    *(__m64*)pDest = mValue64;
                    pBuffer += WordsPerQuadWord;
                    pDest += WordsPerQuadWord;

                    // Make values signed
                    mValue64 = _mm_sub_pi16( mValue64, mSignedScale64 );

                    // Determine parallel min/max
                    mMinValue64 = _mm_min_pi16( mMinValue64, mValue64 );
                    mMaxValue64 = _mm_max_pi16( mMaxValue64, mValue64 );
                }
            }

            // Process quadword values per pass for remainder
            while( count >= WordsPerQuadWord )
            {
                // Process quadword values per pass
                count -= WordsPerQuadWord;

                // Get quadword values
                mValue64 = *(__m64*)pBuffer;
                *(__m64*)pDest = mValue64;
                pBuffer += WordsPerQuadWord;
                pDest += WordsPerQuadWord;

                // Make values signed
                mValue64 = _mm_sub_pi16( mValue64, mSignedScale64 );

                // Determine parallel min/max
                mMinValue64 = _mm_min_pi16( mMinValue64, mValue64 );
                mMaxValue64 = _mm_max_pi16( mMaxValue64, mValue64 );
            }

            // Determine wMinValue

            // Make values unsigned
            mMinValue64 = _mm_add_pi16( mMinValue64, mSignedScale64 );

            // Extract each value in quadword to find minimum
            // for( i = 0; i < WordsPerQuadWord; i++ )
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 0 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 1 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 2 );
            wMinValue = Min( wMinValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMinValue64, 3 );
            wMinValue = Min( wMinValue, wValue );

            // Determine wMaxValue

            // Make values unsigned
            mMaxValue64 = _mm_add_pi16( mMaxValue64, mSignedScale64 );

            // Extract each value in quadword to find maximum
            // for( i = 0; i < WordsPerQuadWord; i++ )
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 0 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 1 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 2 );
            wMaxValue = Max( wMaxValue, wValue );
            wValue = (WORD)_mm_extract_pi16( mMaxValue64, 3 );
            wMaxValue = Max( wMaxValue, wValue );

            _mm_empty();

        } // if( count >= WordsPerQuadWord )
    }
#endif // _WIN64
#endif // defined(USE_SSE4_1)

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pDest++ = *pBuffer++;

        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}

/*****************************************************************************\
Inline Function:
    FindDWordBufferMinMaxCopy

Description:
    Finds the min and max unsigned 32-bit values in the buffer
    Copies data from pBuffer to pDest at the same time

Input:
    DWORD* pDest - pointer to 32-bit buffer to copy into
    DWORD* pBuffer - pointer to 32-bit buffer
    const DWORD bytes - size of buffer in bytes

Output:
    WORD &min - minimum 32-bit value
    WORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindDWordBufferMinMaxCopy(
    DWORD* pDest,
    DWORD* pBuffer,
    const DWORD bytes,
    DWORD &min,
    DWORD &max )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    DWORD wValue = 0;
    DWORD wMinValue = 0xffffffff;
    DWORD wMaxValue = 0x00000000;

    DWORD count = bytes / sizeof(DWORD);

#if defined(USE_SSE4_1)
    DWORD i = 0;

    if( IsAligned( pBuffer, sizeof(DWORD) ) )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD DWordsPerPrefetch            = sizeof(PREFETCH) / sizeof(DWORD);
        const DWORD DWordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(DWORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= DWordsPerDoubleQuadWord )
        {
            const DWORD doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(DWORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pDest++ = *pBuffer++;

                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= DWordsPerDoubleQuadWord )
            {
                __m128i mValue128i;
                __m128 mValue128;

                // Signed min/max initialization
                // need extra QWORD bits for SSE2 FP conversion
                __m128  mMinValue128 = _mm_set1_ps( (float)( (QWORD)wMinValue  ) );
                __m128  mMaxValue128 = _mm_set1_ps( (float)( wMaxValue  ) );

                while( count >= DWordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= DWordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mValue128i = *(__m128i*)pBuffer;
                        _mm_storeu_si128((__m128i*)pDest, mValue128i);
                        pBuffer += DWordsPerDoubleQuadWord;
                        pDest += DWordsPerDoubleQuadWord;

                        // Convert to FP
                        mValue128 = _mm_cvtepi32_ps( mValue128i );

                        // Determine parallel min/max
                        mMinValue128 = _mm_min_ps( mMinValue128,
                            mValue128 );
                        mMaxValue128 = _mm_max_ps( mMaxValue128,
                            mValue128 );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= DWordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= DWordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mValue128i = *(__m128i*)pBuffer;
                    _mm_storeu_si128((__m128i*)pDest, mValue128i);
                    pBuffer += DWordsPerDoubleQuadWord;
                    pDest += DWordsPerDoubleQuadWord;

                    // Convert to FP
                    mValue128 = _mm_cvtepi32_ps( mValue128i );

                    // Determine parallel min/max
                    mMinValue128 = _mm_min_ps( mMinValue128,
                        mValue128 );
                    mMaxValue128 = _mm_max_ps( mMaxValue128,
                        mValue128 );
                }

                // Determine wMinValue

                // Convert back to DWORD
                __m128i mMinValue128i = _mm_cvtps_epi32( mMinValue128 );

                // Extract each value in double-quadword to find minimum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMinValue128i );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 4 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 8 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 12 ) );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Convert back to DWORD
                __m128i mMaxValue128i = _mm_cvtps_epi32( mMaxValue128 );

                // Extract each value in double-quadword to find maximum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMaxValue128i );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 4 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 8 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 3 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 12 ) );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= DWordsPerDoubleQuadWord )
        } // if( count >= DWordsPerDoubleQuadWord )
    }
#endif // defined(USE_SSE4_1)

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pDest++ = *pBuffer++;

        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


/*****************************************************************************\
Inline Function:
    FindWordBufferMinMaxRestartCoy

Description:
    Finds the min and max unsigned 32-bit values in the buffer
    Excludes a restart value from min or max values
    Copies data from pBuffer to pDest at the same time

Input:
    WORD* pDest - pointer to 32-bit buffer to copy into
    WORD* pBuffer - pointer to 32-bit buffer
    const DWORD bytes - size of buffer in bytes
    const WORD restart - restart index to ignore
    cpuInstructionLevel - indicates if SSE_4.1 is available

Output:
    WORD &min - minimum 32-bit value
    WORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindWordBufferMinMaxRestartCopy(
    WORD* pDest,
    WORD* pBuffer,
    const DWORD bytes,
    const WORD restart,
    WORD &min,
    WORD &max,
    CPU_INSTRUCTION_LEVEL cpuInstructionLevel )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    WORD wValue = 0;
    WORD wMinValue = 0xffff;
    WORD wMaxValue = 0x0000;

    size_t count = bytes / sizeof(WORD);

#ifdef USE_SSE4_1

    size_t i = 0;

    if( IsAligned( pBuffer, sizeof(WORD) ) &&
        cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1 )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD WordsPerPrefetch            = sizeof(PREFETCH) / sizeof(WORD);
        const DWORD WordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(WORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= WordsPerDoubleQuadWord )
        {
            const size_t doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(WORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pDest++ = *pBuffer++;

                    if (wValue == restart) {
                        continue;
                    }
                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= WordsPerDoubleQuadWord )
            {
                __m128i mInput, mRestarts, mMask;
                __m128i mAll_ones = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMinValue128i = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMaxValue128i = _mm_setzero_si128();

                // Initialize register used for testing for restart index.
                mRestarts = _mm_setr_epi16(restart, restart, restart, restart, restart, restart, restart, restart);

                while( count >= WordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= WordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mInput = *(__m128i*)pBuffer;
                        _mm_storeu_si128((__m128i*)pDest, mInput);
                        pBuffer += WordsPerDoubleQuadWord;
                        pDest += WordsPerDoubleQuadWord;

                        // Make mask of non-restart_index fields
                        mMask = _mm_andnot_si128(_mm_cmpeq_epi16(mInput, mRestarts), mAll_ones);

                        // Copy minimum and maximum fields for non-restarts
                        mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu16(mMinValue128i, mInput), mMask );
                        mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu16(mMaxValue128i, mInput), mMask );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= WordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= WordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mInput = *(__m128i*)pBuffer;
                    _mm_storeu_si128((__m128i*)pDest, mInput);
                    pBuffer += WordsPerDoubleQuadWord;
                    pDest += WordsPerDoubleQuadWord;

                    // Make mask of non-restart_index fields
                    mMask = _mm_andnot_si128(_mm_cmpeq_epi16(mInput, mRestarts), mAll_ones);

                    // Copy minimum and maximum fields for non-restarts
                    mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu16(mMinValue128i, mInput), mMask );
                    mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu16(mMaxValue128i, mInput), mMask );
                }

                // Determine wMinValue

                // Extract each value in double-quadword to find minimum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 0 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 1 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 2 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 3 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 4 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 5 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 6 );
                wMinValue = Min( wMinValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMinValue128i, 7 );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Extract each value in double-quadword to find maximum
                // for( i = 0; i < WordsPerDoubleQuadWord; i++ )
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 0 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 1 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 2 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 3 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 4 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 5 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 6 );
                wMaxValue = Max( wMaxValue, wValue );
                wValue = (WORD)_mm_extract_epi16( mMaxValue128i, 7 );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= WordsPerDoubleQuadWord )
        } // if( count >= WordsPerDoubleQuadWord )
    }

#endif // USE_SSE4_1

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pDest++ = *pBuffer++;

        if (wValue == restart) {
            continue;
        }
        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


/*****************************************************************************\
Inline Function:
    FindDWordBufferMinMaxRestartCopy

Description:
    Finds the min and max unsigned 32-bit values in the buffer
    Excludes a restart value from min or max values
    Copies data from pBuffer to pDest at the same time

Input:
    DWORD* pDest - pointer to 32-bit buffer to copy into
    DWORD* pBuffer - pointer to 32-bit index buffer
    const DWORD bytes - size of buffer in bytes
    const DWORD restart - restart index to ignore
    cpuInstructionLevel - indicates if SSE_4.1 is available

Output:
    DWORD &min - minimum 32-bit value
    DWORD &max - maximum 32-bit value

\*****************************************************************************/
inline void FindDWordBufferMinMaxRestartCopy(
    DWORD* pDest,
    DWORD* pBuffer,
    const DWORD bytes,
    const DWORD restart,
    DWORD &min,
    DWORD &max,
    CPU_INSTRUCTION_LEVEL cpuInstructionLevel )
{
//    PrefetchBuffer( (BYTE*)pBuffer, bytes );

    DWORD wValue = 0;
    DWORD wMinValue = 0xffffffff;
    DWORD wMaxValue = 0x00000000;

    DWORD count = bytes / sizeof(DWORD);

#ifdef USE_SSE4_1

    DWORD i = 0;

    if( IsAligned( pBuffer, sizeof(DWORD) ) &&
        cpuInstructionLevel >= CPU_INSTRUCTION_LEVEL_SSE4_1 )
    {
        const DWORD DoubleQuadWordsPerPrefetch  = sizeof(PREFETCH) / sizeof(DQWORD);
        const DWORD DWordsPerPrefetch            = sizeof(PREFETCH) / sizeof(DWORD);
        const DWORD DWordsPerDoubleQuadWord      = sizeof(DQWORD) / sizeof(DWORD);

        Prefetch( (BYTE*)pBuffer + sizeof(PREFETCH) );
        Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

        // Find min/max per cacheline of values
        if( count >= DWordsPerDoubleQuadWord )
        {
            const DWORD doubleQuadwordAlignWords =
                GetAlignmentOffset( pBuffer, sizeof(DQWORD) ) / sizeof(DWORD);

            // If pBuffer is not double-quadword aligned then process
            // until aligned
            if( doubleQuadwordAlignWords )
            {
                for( i = 0; i < doubleQuadwordAlignWords; i++ )
                {
                    wValue = *pDest++ = *pBuffer++;

                    if (wValue == restart) {
                        continue;
                    }
                    wMinValue = Min( wMinValue, wValue );
                    wMaxValue = Max( wMaxValue, wValue );
                }

                count -= doubleQuadwordAlignWords;
            }

            // Find min/max per cacheline of values
            if( count >= DWordsPerPrefetch )
            {
                __m128i mInput, mRestarts, mMask;
                __m128i mAll_ones = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMinValue128i = _mm_setr_epi32(0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu);
                __m128i mMaxValue128i = _mm_setzero_si128();

                // Initialize register used for testing for restart index.
                mRestarts = _mm_setr_epi32(restart, restart, restart, restart);

                while( count >= DWordsPerPrefetch )
                {
                    Prefetch( (BYTE*)pBuffer + 2 * sizeof(PREFETCH) );

                    // Process cacheline values per pass
                    count -= DWordsPerPrefetch;

                    for( i = 0; i < DoubleQuadWordsPerPrefetch; i++ )
                    {
                        // Get double-quadword values
                        mInput = *(__m128i*)pBuffer;
                        _mm_storeu_si128((__m128i*)pDest, mInput);
                        pBuffer += DWordsPerDoubleQuadWord;
                        pDest += DWordsPerDoubleQuadWord;

                        // Make mask of non-restart_index fields
                        mMask = _mm_andnot_si128(_mm_cmpeq_epi32(mInput, mRestarts), mAll_ones);

                        // Copy minimum and maximum fields for non-restarts
                        mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu32(mMinValue128i, mInput), mMask );
                        mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu32(mMaxValue128i, mInput), mMask );
                    }
                }

                // Process double-quadword values per pass for remainder
                while( count >= DWordsPerDoubleQuadWord )
                {
                    // Process double-quadword values per pass
                    count -= DWordsPerDoubleQuadWord;

                    // Get double-quadword values
                    mInput = *(__m128i*)pBuffer;
                    _mm_storeu_si128((__m128i*)pDest, mInput);
                    pBuffer += DWordsPerDoubleQuadWord;
                    pDest += DWordsPerDoubleQuadWord;

                    // Make mask of non-restart_index fields
                    mMask = _mm_andnot_si128(_mm_cmpeq_epi32(mInput, mRestarts), mAll_ones);

                    // Copy minimum and maximum fields for non-restarts
                    mMinValue128i = _mm_blendv_epi8(mMinValue128i, _mm_min_epu32(mMinValue128i, mInput), mMask );
                    mMaxValue128i = _mm_blendv_epi8(mMaxValue128i, _mm_max_epu32(mMaxValue128i, mInput), mMask );
                }

                // Determine wMinValue

                // Extract each value in double-quadword to find minimum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMinValue128i );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 4 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 8 ) );
                wMinValue = Min( wMinValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMinValue128i, 12 ) );
                wMinValue = Min( wMinValue, wValue );

                // Determine wMaxValue

                // Extract each value in double-quadword to find maximum
                // Grab element 0 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32( mMaxValue128i );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 1 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 4 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 2 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 8 ) );
                wMaxValue = Max( wMaxValue, wValue );
                // Grab element 3 from m128i reg:   3 | 2 | 1 | 0
                wValue = (DWORD)_mm_cvtsi128_si32(
                        _mm_srli_si128( mMaxValue128i, 12 ) );
                wMaxValue = Max( wMaxValue, wValue );

            } // if( count >= DWordsPerPrefetch )
        } // if( count >= DWordsPerDoubleQuadWord )
    }

#endif // USE_SSE4_1

    // Find min/max per value
    while( count > 0 )
    {
        count -= 1;

        wValue = *pDest++ = *pBuffer++;

        if (wValue == restart) {
            continue;
        }
        wMinValue = Min( wMinValue, wValue );
        wMaxValue = Max( wMaxValue, wValue );
    }

    min = wMinValue;
    max = wMaxValue;
}


} // iSTD

#if defined(USE_X86)
# undef USE_X86
#endif // defined(USE_X86)

#if defined(USE_SSE4_1)
# undef USE_SSE4_1
#endif // defined(USE_SSE4_1)

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
#ifndef __IGC_H
#define __IGC_H

#if defined(_WIN32)
    #define IGC_API_CALL_TYPE __stdcall
    #ifdef IGC_EXPORTS
        #define IGC_API_CALL __declspec(dllexport) IGC_API_CALL_TYPE
    #else
        #define IGC_API_CALL __declspec(dllimport) IGC_API_CALL_TYPE
    #endif
#else
    #define IGC_API_CALL_TYPE
    #define IGC_API_CALL __attribute__ ((visibility("default"))) IGC_API_CALL_TYPE
#endif

#if defined(_DEBUG) || defined(_INTERNAL)
# define IGC_DEBUG
#endif

#include <string>
#include "../IGC/Compiler/CodeGenPublicEnums.h"
// TODO: add all external declarations so that external projects only need
// to include this file only.

// Definition of integer Registry-key Values
// (may merge this into igc_flags.def's macro)
enum {
    // ForcePixelShaderSIMDMode
    FLAG_PS_SIMD_MODE_DEFAULT = 0,                 // default is SIMD8 compilation + heuristics to determine if we want to compile SIMD32/SIMD16
    FLAG_PS_SIMD_MODE_FORCE_SIMD8 = 1,             // Force SIMD8 compilation
    FLAG_PS_SIMD_MODE_FORCE_SIMD16 = 2,            // Force SIMD16 compilation
    FLAG_PS_SIMD_MODE_FORCE_SIMD32 = 4,            // Force SIMD32 compilation
};

enum {
    BIT_CG_SIMD8     = 0b0000000000000001,
    BIT_CG_SIMD16    = 0b0000000000000010,
    BIT_CG_SIMD32    = 0b0000000000000100,
    BIT_CG_SPILL8    = 0b0000000000001000,
    BIT_CG_SPILL16   = 0b0000000000010000,
    BIT_CG_SPILL32   = 0b0000000000100000,
    BIT_CG_RETRY     = 0b0000000001000000,
    BIT_CG_DO_SIMD32 = 0b0000000010000000,
    BIT_CG_DO_SIMD16 = 0b0000000100000000,
};

typedef unsigned short CG_CTX_STATS_t;

// shader stat for opt customization
typedef struct {
    uint32_t         m_tempCount;
    uint32_t         m_sampler;
    uint32_t         m_inputCount;
    uint32_t         m_dxbcCount;
    uint32_t         m_ConstantBufferCount;
    IGC::Float_DenormMode m_floatDenormMode16;
    IGC::Float_DenormMode m_floatDenormMode32;
    IGC::Float_DenormMode m_floatDenormMode64;
} SHADER_STATS_t;

typedef struct {
    CG_CTX_STATS_t  m_stats;              // record what simd has been generated
    void*           m_pixelShaderGen;     // Generated pixel shader output
    char*           m_savedBitcodeCharArray; // Serialized Bitcode
    unsigned int    m_savedBitcodeCharArraySize;
    void*           m_savedInstrTypes;
    SHADER_STATS_t  m_savedShaderStats;
} CG_CTX_t;

#define IsRetry(stats)               (stats & (BIT_CG_RETRY))
#define DoSimd16(stats)              (stats & (BIT_CG_DO_SIMD16))
#define DoSimd32(stats)              (stats & (BIT_CG_DO_SIMD32))
#define HasSimd(MODE, stats)         (stats & (BIT_CG_SIMD##MODE))
#define HasSimdSpill(MODE, stats)   ((stats & (BIT_CG_SIMD##MODE)) &&  (stats & (BIT_CG_SPILL##MODE)))
#define HasSimdNoSpill(MODE, stats) ((stats & (BIT_CG_SIMD##MODE)) && !(stats & (BIT_CG_SPILL##MODE)))
#define SetRetry(stats)              (stats = (stats | (BIT_CG_RETRY)))
#define SetSimd16(stats)             (stats = (stats | (BIT_CG_DO_SIMD16)))
#define SetSimd32(stats)             (stats = (stats | (BIT_CG_DO_SIMD32)))
#define SetSimdSpill(MODE, stats)    (stats = (stats | (BIT_CG_SIMD##MODE) |  (BIT_CG_SPILL##MODE)))
#define SetSimdNoSpill(MODE, stats)  (stats = (stats | (BIT_CG_SIMD##MODE) & ~(BIT_CG_SPILL##MODE)))

typedef enum {
    FLAG_CG_ALL_SIMDS = 0,
    FLAG_CG_STAGE1_FAST_COMPILE = 1,
    FLAG_CG_STAGE1_BEST_PERF = 2,
} CG_FLAG_t;

#define IsStage2Available(ctx_ptr) (ctx_ptr != nullptr)

#define IsStage2RestSIMDs(prev_ctx_ptr) (prev_ctx_ptr != nullptr)
#define IsStage1FastCompile(flag, prev_ctx_ptr) (!IsStage2RestSIMDs(prev_ctx_ptr) && flag == FLAG_CG_STAGE1_FAST_COMPILE)
#define IsStage1BestPerf(flag, prev_ctx_ptr)    (!IsStage2RestSIMDs(prev_ctx_ptr) && flag == FLAG_CG_STAGE1_BEST_PERF)
#define IsAllSIMDs(flag, prev_ctx_ptr)          (!IsStage2RestSIMDs(prev_ctx_ptr) && flag == FLAG_CG_ALL_SIMDS)
#define IsStage1(pCtx)   (IsStage1BestPerf(pCtx->m_CgFlag, pCtx->m_StagingCtx) || \
                          IsStage1FastCompile(pCtx->m_CgFlag, pCtx->m_StagingCtx))
#define HasSavedIR(pCtx) (pCtx && IsStage2RestSIMDs(pCtx->m_StagingCtx) && \
                          pCtx->m_StagingCtx->m_savedBitcodeCharArraySize > 0)

#define DoSimd32Stage2(prev_ctx_ptr) (IsStage2RestSIMDs(prev_ctx_ptr) && DoSimd32(prev_ctx_ptr->m_stats))
#define DoSimd16Stage2(prev_ctx_ptr) (IsStage2RestSIMDs(prev_ctx_ptr) && DoSimd16(prev_ctx_ptr->m_stats))

#define ContinueFastCompileStage1(flag, prev_ctx_ptr, stats) ( \
    IsStage1FastCompile(flag, prev_ctx_ptr) && \
    (IsRetry(stats) || DoSimd16(stats)))

// If the staged compilation enabled, we don't need compile continuation when SIMD8 is spilled.
#define ContinueBestPerfStage1(flag, prev_ctx_ptr, stats) ( \
    IsStage1BestPerf(flag, prev_ctx_ptr) && \
     (( IGC_IS_FLAG_ENABLED(ExtraRetrySIMD16) && !HasSimdSpill(8, stats)) || \
      (!IGC_IS_FLAG_ENABLED(ExtraRetrySIMD16) && (!HasSimd(8, stats) || DoSimd32(stats)))))

// We don't need compile continuation if no staged compilation enabled denoted by RegKeys.
#define HasCompileContinuation(Ail, flag, prev_ctx_ptr, stats) ( \
    (IGC_IS_FLAG_ENABLED(StagedCompilation) || Ail) && \
    (ContinueFastCompileStage1(flag, prev_ctx_ptr, stats) || \
     ContinueBestPerfStage1(flag, prev_ctx_ptr, stats)))

// Return true when simd MODE has been generated previously
#define AvoidDupStage2(MODE, flag, prev_ctx_ptr)      (IsStage2RestSIMDs(prev_ctx_ptr) && HasSimdNoSpill(MODE, prev_ctx_ptr->m_stats))

// Fast CG always returns simd 8
#define ValidFastModes(flag, prev_ctx_ptr, stats)     (IsStage1FastCompile(flag, prev_ctx_ptr) && HasSimd(8, stats) && !HasSimd(16, stats) && !HasSimd(32, stats))
// Best CG returns simd 8 or 16
#define ValidBestModes(flag, prev_ctx_ptr, stats)     (IsStage1BestPerf(flag, prev_ctx_ptr) && (HasSimd(8, stats) || HasSimd(16, stats)) && !HasSimd(32, stats))
// ALL_SIMDS CG must have simd 8 in any case
#define ValidAllSimdsModes(flag, prev_ctx_ptr, stats) (IsAllSIMDs(flag, prev_ctx_ptr) && HasSimd(8, stats))

// Rest Stage2 CG would not generate duplicated simd 32 or 16 modes, and
// When simd 8 spills in Stage1 for FAST_COMPILE, we may generate simd again;
// otherwise it must be generated either from Stage1 or Stage2
#define ValidStage2Modes(prev_ctx_ptr, stats) ( \
    IsStage2RestSIMDs(prev_ctx_ptr) && \
    (!HasSimd(32, prev_ctx_ptr->m_stats) || !HasSimd(32, stats)) && \
    (!HasSimd(16, prev_ctx_ptr->m_stats) || !HasSimd(16, stats)) && \
    ((HasSimd(8,  prev_ctx_ptr->m_stats) ^ HasSimd(8,  stats)) || HasSimdSpill(8,  prev_ctx_ptr->m_stats)) \
    )

#define ValidGeneratedModes(flag, prev_ctx_ptr, stats) ( \
    ValidFastModes(flag, prev_ctx_ptr, stats) || \
    ValidBestModes(flag, prev_ctx_ptr, stats) || \
    ValidAllSimdsModes(flag, prev_ctx_ptr, stats) || \
    ValidStage2Modes(prev_ctx_ptr, stats) \
    )

#endif // __IGC_H


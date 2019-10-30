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
    BIT_CG_SIMD8   = 0b00000001,
    BIT_CG_SIMD16  = 0b00000010,
    BIT_CG_SIMD32  = 0b00000100,
    BIT_CG_SPILL8  = 0b00001000,
    BIT_CG_SPILL16 = 0b00010000,
    BIT_CG_SPILL32 = 0b00100000,
    BIT_CG_RETRY   = 0b01000000,
};

typedef char CG_CTX_t;

#define IsRetry(ctx)               (ctx & (BIT_CG_RETRY))
#define HasSimd(MODE, ctx)         (ctx & (BIT_CG_SIMD##MODE))
#define HasSimdSpill(MODE, ctx)   ((ctx & (BIT_CG_SIMD##MODE)) &&  (ctx & (BIT_CG_SPILL##MODE)))
#define HasSimdNoSpill(MODE, ctx) ((ctx & (BIT_CG_SIMD##MODE)) && !(ctx & (BIT_CG_SPILL##MODE)))
#define SetRetry(ctx)              (ctx = (ctx | (BIT_CG_RETRY)))
#define SetSimdSpill(MODE, ctx)    (ctx = (ctx | (BIT_CG_SIMD##MODE) |  (BIT_CG_SPILL##MODE)))
#define SetSimdNoSpill(MODE, ctx)  (ctx = (ctx | (BIT_CG_SIMD##MODE) & ~(BIT_CG_SPILL##MODE)))

typedef enum {
    FLAG_CG_ALL_SIMDS = 0,
    FLAG_CG_STAGE1_FAST_COMPILE = 1,
    FLAG_CG_STAGE1_BEST_PERF = 2,
    FLAG_CG_STAGE2_REST_SIMDS = 3,
} CG_FLAG_t;

#define UnsetStagingCtx(ctx) (ctx = 0)
#define IsStage2Available(ctx) (ctx != 0)

// We don't need compile continuation if no staged compilation enabled denoted by RegKeys.
// If the staged compilation enabled, we don't need compile continuation when SIMD8 is spilled.
#define HasCompileContinuation(flag, ctx) ( \
    IGC_IS_FLAG_ENABLED(StagedCompilation) && \
    ((flag == FLAG_CG_STAGE1_FAST_COMPILE) || \
     ((flag == FLAG_CG_STAGE1_BEST_PERF) && \
      !HasSimdSpill(8, ctx))) \
    )

// Return true when simd MODE has been generated previously
#define AvoidDupStage2(MODE, flag, prev_ctx)  (flag == FLAG_CG_STAGE2_REST_SIMDS && HasSimd(MODE, prev_ctx))

// Fast CG always returns simd 8
#define ValidFastModes(flag, ctx)       (flag == FLAG_CG_STAGE1_FAST_COMPILE && HasSimd(8, ctx) && !HasSimd(16, ctx) && !HasSimd(32, ctx))
// Best CG returns simd 8 or 16
#define ValidBestModes(flag, ctx)       (flag == FLAG_CG_STAGE1_BEST_PERF && (HasSimd(8, ctx) || HasSimd(16, ctx)) && !HasSimd(32, ctx))
// ALL_SIMDS CG must have simd 8 in any case
#define ValidAllSimdsModes(flag, ctx)   (flag == FLAG_CG_ALL_SIMDS && HasSimd(8, ctx))
// Remaining Stage2 CG must have simd 8 in any case
#define ValidFullStage2Modes(flag, ctx) (flag == FLAG_CG_STAGE2_REST_SIMDS && HasSimd(8, ctx))

// Rest Stage2 CG would not generate duplicated simd 32 mode, and
// simd 8 and simd 16 must be generated either from Stage1 or Stage2
#define ValidStage2Modes(flag, prev_ctx, ctx) ( \
    flag == FLAG_CG_STAGE2_REST_SIMDS && \
    (!HasSimd(32, prev_ctx) || !HasSimd(32, ctx)) && \
    (HasSimd(8,  prev_ctx) ^ HasSimd(8,  ctx)) && \
    (HasSimd(16, prev_ctx) ^ HasSimd(16, ctx)) \
    )

#define ValidGeneratedModes(flag, prev_ctx, ctx) ( \
    ValidFastModes(flag, ctx) || \
    ValidBestModes(flag, ctx) || \
    ValidAllSimdsModes(flag, ctx) || \
    ValidStage2Modes(flag, prev_ctx, ctx) \
    )

#endif // __IGC_H


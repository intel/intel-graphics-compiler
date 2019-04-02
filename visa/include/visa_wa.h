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
/**
** File Name     : visa_wa.h
**
** Abstract      : Declaration of all SW workarounds implemented in vISA finalizer.
**
**  ---------------------------------------------------------------------- */

#ifndef _VISA_WA_H_
#define _VISA_WA_H_

#define VISA_WA_DECLARE( wa, wa_comment, wa_bugType ) unsigned int wa : 1;
#define VISA_WA_INIT( wa ) wa = 0;
#define VISA_WA_ENABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = true;                  \
}
#define VISA_WA_DISABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = false;                  \
}
#define VISA_WA_CHECK(pWaTable, w)  ((pWaTable)->w)

enum VISA_WA_BUG_TYPE
{
    VISA_WA_BUG_TYPE_UNKNOWN    = 0,
    VISA_WA_BUG_TYPE_CORRUPTION = 1,
    VISA_WA_BUG_TYPE_HANG       = 2,
    VISA_WA_BUG_TYPE_PERF       = 4,
    VISA_WA_BUG_TYPE_FUNCTIONAL = 8,
    VISA_WA_BUG_TYPE_SPEC       = 16,
    VISA_WA_BUG_TYPE_FAIL       = 32
};

typedef struct _VISA_WA_TABLE
{

    VISA_WA_DECLARE(
        WaHeaderRequiredOnSimd16Sample16bit,
        "SIMD16 send to sampler using 16-bit format must use header.",
        VISA_WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
        WaSendsSrc1SizeLimitWhenEOT,
        "Work around to limit size of src1 up to 2 GRFs, on Sends/Sendsc instructions when they are EOT.",
        WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
        WaDisallow64BitImmMov,
        "Mov with 64 bit immediate is not allowed.",
        WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
        WaByteDstAlignRelaxedRule,
        "Relaxed alignmen rule for byte destiantion is not allowed for BDW SteppingA.",
        WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
        WaSIMD16SIMD32CallDstAlign,
        "SIMD16/SIMD32 call destiantion must have .0 offset for SKL SteppingA.",
        WA_BUG_TYPE_UNKNOWN)
        
    VISA_WA_DECLARE(
        WaThreadSwitchAfterCall,
        "BDW, CHV, SKL, BXT: Follow every call by a dummy non-JEU and non-send instruction with a switch for both cases whether a subroutine is taken or not.",
        WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
        WaSendWARRestriction,
        "Two send instructions with WAR dependency is not allowed on BDW SteppingA.",
        WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaSrc1ImmHfNotAllowed,
    "Immediate source1 of type HF is not allowed for SKL C0 && BXT A0",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaDstSubRegNumNotAllowedWithLowPrecPacked,
    "Destination of type HF with sub register offet > 0 not allowed in math instruction SKL A0 && CHV A0",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaDisableMixedModeLog,
    "Mixed mode is not allowed for LOG instruction on SKL A0, B0.",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaDisableMixedModeFdiv,
    "Mixed mode is not allowed for DIV instruction on SKL A0, B0.",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaResetN0BeforeGatewayMessage,
    "Gateway is sending spurious clear to notify register resulting in a EU hang.",
    WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    WaDisableMixedModePow,
    "Mixed mode is not allowed for POW instruction on SKL A0, B0.",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaFloatMixedModeSelNotAllowedWithPackedDestination,
    "HF destination is not allowed for CHV.",
    WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WADisableWriteCommitForPageFault,
    "No write commit for page fault mode",
    VISA_WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaDisableSIMD16On3SrcInstr,
    "Split SIMD16 3src instructions with HF in to SIMD8",
    VISA_WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaSendSEnableIndirectMsgDesc,
    "Enable indirect msgDesc for sends in GPGPU mode with preemption",
    VISA_WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaClearArfDependenciesBeforeEot,
    "Clear ARF dependencies before a send instruction",
    VISA_WA_BUG_TYPE_UNKNOWN)

    VISA_WA_DECLARE(
    WaDisableSendsSrc0DstOverlap,
    "sends instruction hangs if there is overlap of src0 and destination registers. So disable Sends as a WA",
    VISA_WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    WaMixModeSelInstDstNotPacked,
    "Disable hf dst for mix mode sel instruction, even if it is not packed.",
    VISA_WA_BUG_TYPE_FAIL)

    VISA_WA_DECLARE(
    WaDisableSendSrcDstOverlap,
    "When pagefault is enabled, the source and destination operands must not overlap.",
    VISA_WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    WaDisableSendsPreemption,
    "Disable preemption for sends with non-null src1 to work-around HW hang bug.",
    VISA_WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    WaClearTDRRegBeforeEOTForNonPS,
    "Clear tdr register before every EOT in non-PS shader kernels.",
    VISA_WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    WaNoSimd16TernarySrc0Imm,
    "do not allow src0 immediate for ternary simd16 inst.",
    VISA_WA_BUG_TYPE_FAIL)

    VISA_WA_DECLARE(
    Wa_1406306137,
    "Do not apply atomic instruction option on sends.",
    VISA_WA_BUG_TYPE_HANG)

    VISA_WA_DECLARE(
    Wa_2201674230,
    "Limit the number of live sends to work-around HW hang bug.",
    VISA_WA_BUG_TYPE_HANG)

	VISA_WA_DECLARE(
    Wa_1406950495,
    "Do not read ce0 to work-around HW bug.",
    VISA_WA_BUG_TYPE_HANG)

	VISA_WA_DECLARE(
    Wa_1606931601,
    "Split simd16 to two simd8 if src0 is scalar and src1 and src2 have bundle conflict.",
    VISA_WA_BUG_TYPE_UNKNOWN)
	
	VISA_WA_DECLARE(
    WaSwapForSrc1Replicate,
    "swap src1 with src2 or src0 when src1 is replicate, there is no scalar mux in src1.",
    VISA_WA_BUG_TYPE_UNKNOWN)

    _VISA_WA_TABLE()
    {
        VISA_WA_INIT(WaHeaderRequiredOnSimd16Sample16bit)
        VISA_WA_INIT(WaSendsSrc1SizeLimitWhenEOT)
        VISA_WA_INIT(WaDisallow64BitImmMov)
        VISA_WA_INIT(WaByteDstAlignRelaxedRule)
        VISA_WA_INIT(WaSIMD16SIMD32CallDstAlign)
        VISA_WA_INIT(WaThreadSwitchAfterCall)
        VISA_WA_INIT(WaSendWARRestriction)
        VISA_WA_INIT(WaSrc1ImmHfNotAllowed)
        VISA_WA_INIT(WaDstSubRegNumNotAllowedWithLowPrecPacked)
        VISA_WA_INIT(WaDisableMixedModeLog)
        VISA_WA_INIT(WaDisableMixedModeFdiv)
        VISA_WA_INIT(WaResetN0BeforeGatewayMessage)
        VISA_WA_INIT(WaDisableMixedModePow)
        VISA_WA_INIT(WaFloatMixedModeSelNotAllowedWithPackedDestination)
        VISA_WA_INIT(WADisableWriteCommitForPageFault)
        VISA_WA_INIT(WaDisableSIMD16On3SrcInstr)
        VISA_WA_INIT(WaSendSEnableIndirectMsgDesc)
        VISA_WA_INIT(WaClearArfDependenciesBeforeEot)
        VISA_WA_INIT(WaDisableSendsSrc0DstOverlap)
        VISA_WA_INIT(WaMixModeSelInstDstNotPacked)
        VISA_WA_INIT(WaDisableSendSrcDstOverlap)
        VISA_WA_INIT(WaDisableSendsPreemption)
        VISA_WA_INIT(WaClearTDRRegBeforeEOTForNonPS)
        VISA_WA_INIT(WaNoSimd16TernarySrc0Imm)
        VISA_WA_INIT(Wa_1406306137)
        VISA_WA_INIT(Wa_2201674230)
        VISA_WA_INIT(Wa_1406950495)
        VISA_WA_INIT(Wa_1606931601)
        VISA_WA_INIT(WaSwapForSrc1Replicate)
    }
} VISA_WA_TABLE, *PVISA_WA_TABLE;

#endif

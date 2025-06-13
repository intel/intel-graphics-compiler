/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorOCL/TranslationBlock.h"
#include <optional>
#include <vector>
#include <string>

// We should probably replace all of this with proper option parsing,
// like RS does
namespace IGC {
class InternalOptions
{
public:
    InternalOptions(const TC::STB_TranslateInputArgs* pInputArgs)
    {
        if (pInputArgs == nullptr)
            return;

        if (pInputArgs->pInternalOptions != nullptr)
        {
            parseOptions(pInputArgs->pInternalOptions);
        }
    }

    bool KernelDebugEnable                          = false;
    bool IncludeSIPCSR                              = false;
    bool IncludeSIPKernelDebug                      = false;
    bool IntelGreaterThan4GBBufferRequired          = false;
    bool IntelDisableA64WA                          = false;
    bool IntelForceEnableA64WA                      = false;
    bool Use32BitPtrArith                           = false;
    bool IncludeSIPKernelDebugWithLocalMemory       = false;

    // stateless to stateful optimization
    bool IntelHasPositivePointerOffset              = false;
    bool IntelHasBufferOffsetArg                    = false;
    bool IntelBufferOffsetArgOptional               = true;

    bool replaceGlobalOffsetsByZero                 = false;
    bool IntelEnablePreRAScheduling                 = true;
    bool PromoteStatelessToBindless                 = false;
    bool PreferBindlessImages                       = false;
    bool UseBindlessMode                            = false;
    bool UseBindlessPrintf                          = false;
    bool UseBindlessLegacyMode                      = true;
    bool ExcludeIRFromZEBinary                      = false;
    bool EmitZeBinVISASections                      = false;
    bool NoSpill                                    = false;
    bool DisableNoMaskWA                            = false;
    bool IgnoreBFRounding                           = false;   // If true, ignore BFloat rounding when folding bf operations
    bool CompileOneKernelAtTime                     = false;

    // Generic address related
    bool ForceGlobalMemoryAllocation                = false;

    // -1 : initial value that means it is not set from cmdline
    // 0-5: valid values set from the cmdline
    int16_t VectorCoalescingControl                 = -1;

    bool Intel128GRFPerThread                       = false;
    bool Intel256GRFPerThread                       = false;
    bool IntelNumThreadPerEU                        = false;
    int32_t numThreadsPerEU                         = -1;

    bool IntelExpGRFSize                            = false;
    uint32_t expGRFSize                             = 0;
     bool IntelEnableAutoLargeGRF = false;

     // IntelForceInt32DivRemEmu is used only if fp64 is supported natively.
     // IntelForceInt32DivRemEmu wins if both are set and can be applied.
     bool IntelForceInt32DivRemEmu                   = false;
     bool IntelForceInt32DivRemEmuSP                 = false;
     bool IntelForceDisable4GBBuffer                 = false;
     // user-controled option to disable EU Fusion
     bool DisableEUFusion                            = false;
     // Function Control (same as IGC key FunctionControl)
     int FunctionControl                             = -1;
     // Fail comilation if spills are present in compiled kernel
     bool FailOnSpill                                = false;
     // This option enables FP64 emulation for platforms that
     // cannot HW support for double operations
     bool EnableFP64GenEmu                           = false;
     // Cache default. -1 menans not set (thus not used by igc);
     // Valid values are defined as enum type LSC_L1_L3_CC in
     //   visa\include\visa_igc_common_header.h, which are from
     //   macro definitions in igc\common\igc_regkeys_enums_defs.h
     int StoreCacheDefault                           = -1;
     int LoadCacheDefault                            = -1;
     // Force high-accuracy math functions from BiFModule
     bool UseHighAccuracyMathFuncs                   = false;

     bool AllowRelocAdd                              = true;
     // LdStCombine
     //   EnableLdStCombine:
     //      0: disable LdStCombine
     //      1: enable LdStCombine for LSC
     //      2: enable LdStCombine for LSC and Non-LSC
     //      otherwise: ignored
     //   MaxStoreBytes:
     //   MaxLoadBytes:
     //     4, 8, 16, 32 : set max bytes for combining
     //     otherwise: ignored.
     int LdStCombine                                 = -1; // default
     uint32_t MaxStoreBytes                          = 0;  // default
     uint32_t MaxLoadBytes                           = 0;  // default

     uint32_t IntelPrivateMemoryMinimalSizePerThread = 0;
     uint32_t IntelScratchSpacePrivateMemoryMinimalSizePerThread = 0;

     bool EnableDivergentBarrierHandling             = false;
     bool EnableBufferBoundsChecking                 = false;

     // Compile only up to vISA stage.
     bool EmitVisaOnly                               = false;


    uint64_t MinimumValidAddress              = 0;

private:
     void parseOptions(const char* internalOpts);
     };

class Options
{
public:
    Options(const TC::STB_TranslateInputArgs* pInputArgs)
    {
        if (pInputArgs == nullptr)
            return;

        if (pInputArgs->pOptions != nullptr)
        {
            parseOptions(pInputArgs->pOptions);
        }
    }

    bool CorrectlyRoundedSqrt               = false;
    bool NoSubgroupIFP                      = false;
    bool UniformWGS                         = false;
    bool EnableTakeGlobalAddress            = false;
    bool IsLibraryCompilation               = false;
    uint32_t LibraryCompileSIMDSize         = 0;
    bool IntelRequiredEUThreadCount         = false;
    bool EmitErrorsForLibCompilation        = false;
    uint32_t requiredEUThreadCount          = 0;

    bool GTPinReRA                          = false;
    bool GTPinGRFInfo                       = false;
    bool GTPinScratchAreaSize               = false;
    bool GTPinIndirRef                      = false;
    // Skip emission of Frame Descriptor Entry in VISA.
    bool SkipFDE                            = false;
    bool NoFusedCallWA                      = false;
    bool DisableCompaction                  = false;
    uint32_t GTPinScratchAreaSizeValue      = 0;

    std::vector<std::string> LargeGRFKernels;
    std::vector<std::string> RegularGRFKernels;

    bool Xfinalizer                         = false;
    std::string XfinalizerOption;

       // Enable compiler heuristics ("-autoGRFSelection" in VISA) for large GRF selection.
    bool IntelEnableAutoLargeGRF            = false;

    bool IntelLargeRegisterFile             = false;

    bool Intel128GRFPerThread               = false;
    bool Intel256GRFPerThread               = false;
    bool IntelGreaterThan4GBBufferRequired  = false;

    bool IntelExpGRFSize                    = false;
    uint32_t expGRFSize                     = 0;

    // Generic address related
    bool NoLocalToGeneric                   = false;

    // This option forces IGC to poison kernels using fp64
    // operations on platforms without HW support for fp64.
    bool EnableUnsupportedFP64Poisoning = false;
    // This option enables FP64 emulation for platforms that
    // cannot HW support for double operations
    bool EnableFP64GenEmu = false;
    // This option enables FP64 emulation for conversions
    // This applies to platforms that cannot HW support for double operations
    bool EnableFP64GenConvEmu = false;
    // This option enables static profile-guided trimming
    bool StaticProfileGuidedTrimming = false;
    // This option enables IEEE float exception trap bit in Control Register
    bool EnableIEEEFloatExceptionTrap = false;

private:
    void parseOptions(const char* opts);
};
} // namespace IGC

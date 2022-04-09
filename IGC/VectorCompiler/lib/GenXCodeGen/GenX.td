/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is a target description file for the Intel Gen architecture, referred
// to here as the "GenX" architecture.
//
//===----------------------------------------------------------------------===//

// Get the target-independent interfaces which we are implementing...
//
include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// GenX Subtarget state - these are typically inferred from the Proc
//===----------------------------------------------------------------------===//

def FeatureLongLong : SubtargetFeature<"longlong", "HasLongLong", "true",
                                       "supports long long">;
def FeatureFP64: SubtargetFeature<"fp64", "HasFP64", "true",
                                  "support for double types">;
def FeatureGRFByteSize64 : SubtargetFeature<"grf_byte_size_64",
                                            "GRFByteSize",
                                            "64",
                                            "Every GRF byte size is 64">;
def FeatureLSCMaxWidth32 : SubtargetFeature<"lsc_max_width_32",
                                            "LSCMaxWidth",
                                            "32",
                                            "LSC messages have a maximum width of 32">;
def FeatureTransLegacy : SubtargetFeature<"translate_legacy_message",
                                          "TranslateLegacyMessages",
                                          "true",
                                          "translate legacy message to LSC">;
def FeaturePartialI64Emulation : SubtargetFeature<"lightweight_i64_emulation",
                                       "PartialI64Emulation",
                                       "true",
                                       "emulate subset of 64-bit operations">;

def FeatureUseMulDDQ : SubtargetFeature<"mul_ddq",
                                        "UseMulDDQ",
                                        "true",
                                        "use native support for mul [U]Dx[U]D->Q">;

def FeatureLongLongEmulation : SubtargetFeature<"emulate_i64",
                                        "EmulateLongLong",
                                        "true",
                                        "emulate 64-bit operations">;

def FeatureNoJmpi : SubtargetFeature<"disable_jmpi", "DisableJmpi",
                                       "true", "disable jmpi">;

def FeatureVectorDecomp : SubtargetFeature<"disable_vec_decomp",
                                           "DisableVectorDecomposition",
                                           "true",
                                           "disable vector decomposition pass">;

def FeatureNoJumpTables : SubtargetFeature<"disable_jump_tables", "DisableJumpTables",
                                           "true", "disable switch to jump tables lowering">;

def FeatureSwitchjmp : SubtargetFeature<"switchjmp", "HasSwitchjmp", "true",
                                        "supports switchjmp visa instruction">;

def FeaturePreemption : SubtargetFeature<"preemption", "HasPreemption", "true",
                                         "supports preemption">;

def FeatureWAFusedEUNoMask : SubtargetFeature<
    "wa_nomask_fusedEU", "WaNoMaskFusedEU", "true",
    "needs workaround for nomask operations under divergent control-flow">;

def FeatureFusedEU : SubtargetFeature<"fusedEU", "HasFusedEU", "true", "has fused EUs">;

def FeatureIntDivRem32: SubtargetFeature<"divrem32",
                                         "HasIntDivRem32",
                                         "true",
                                         "supports 32-bit integer division">;
def FeatureInstrAdd64: SubtargetFeature<"add64",
                                       "HasAdd64",
                                       "true",
                                       "enable support for native add64 instruction">;

def FeatureInstrBitRotate: SubtargetFeature<"bitrotate",
                                           "HasBitRotate",
                                           "true",
                                           "support of rol/ror instructions">;

def FeatureInstr64BitRotate: SubtargetFeature<"bitrotate64",
                                              "Has64BitRotate",
                                              "true",
                                              "support of 64-bit rol/ror instructions">;

def FeatureHWTIDFromPredef: SubtargetFeature<"hwtidfrompredef",
                                             "GetsHWTIDFromPredef",
                                             "true",
                                             "hwtid is obtained from predefined variable">;

def FeatureHasL1ReadOnlyCache: SubtargetFeature<"has_l1_read_only_cache",
                                                "HasL1ReadOnlyCache",
                                                "true",
                                                "Has L1 read-only cache">;

def FeatureSupressLocalMemFence: SubtargetFeature<"supress_local_mem_fence",
                                                  "HasLocalMemFenceSupress",
                                                  "true",
                                                  "Supresses local memory fence">;
def FeatureHasPackedFloat : SubtargetFeature<"has_packed_float",
                                             "HasPackedFloat",
                                             "true",
                                             "true if packed float immediate vector operands are supported">;
def FeatureMultiTile: SubtargetFeature<"multi_tile",
                                       "HasMultiTile",
                                       "true",
                                       "Multi-tile">;

def FeatureL3CacheCoherentCrossTiles: SubtargetFeature<"l3_cache_coherent_cross_tiles",
                                                       "HasL3CacheCoherentCrossTiles",
                                                       "true",
                                                       "Has L3 cache coherent cross tiles">;

def FeatureL3FlushOnGPUScopeInvalidate: SubtargetFeature<"l3_flush_on_gpu_scope_invalidate",
                                                         "HasL3FlushOnGPUScopeInvalidate",
                                                         "true",
                                                         "Has L3 flush on GPU scope invalidate">;
// Targets that use compute walker command require loading of thread
// payload by compiler.
def FeatureThreadPayloadInMemory : SubtargetFeature<"thread_payload_in_memory",
                                                    "HasThreadPayloadInMemory",
                                                    "true",
                                                    "need to load thread payload from memory">;

def FeatureBfMixedModeWidth16 : SubtargetFeature<"feature_bf_mixed16",
                                                 "HasBfMixedModeWidth16",
                                                 "true",
                                                 "BF mixed mode operations extended width">;

def FeatureHasLSC : SubtargetFeature<"feature_has_lsc",
                                     "HasLSCMessages",
                                     "true",
                                     "Target supports LSC messages">;

def FeatureHasHalfSIMDLSC : SubtargetFeature<"feature_has_half_simd_lsc",
                                     "HasHalfSIMDLSC",
                                     "true",
                                     "Target supports half simd lsc">;

def WarnCallable : SubtargetFeature<"warn_callable", "WarnCallable",
                                    "true", "warn instead of error on callable violation">;

def OCLRuntime : SubtargetFeature<"ocl_runtime", "OCLRuntime", "true",
                                  "Prepare structures for OCL runtime">;


//===----------------------------------------------------------------------===//
// GenX processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic",         []>;
def : Proc<"BDW",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef]>;
def : Proc<"SKL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption]>;
def : Proc<"BXT",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption]>;
def : Proc<"KBL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption]>;
def : Proc<"GLK",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption]>;
def : Proc<"ICLLP",           [FeatureLongLongEmulation, FeatureSwitchjmp,
                               FeatureIntDivRem32, FeatureInstrBitRotate,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption]>;
def : Proc<"TGLLP",           [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption, FeatureFusedEU]>;
def : Proc<"RKL",             [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption, FeatureFusedEU]>;
def : Proc<"DG1",             [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption, FeatureFusedEU]>;
def : Proc<"XEHP",            [FeatureLongLong,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureThreadPayloadInMemory,
                               FeatureHasPackedFloat,
                               FeatureInstrBitRotate, FeatureFusedEU]>;
def : Proc<"ADLS",            [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption, FeatureFusedEU]>;
def : Proc<"ADLP",            [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHasPackedFloat,
                               FeatureHWTIDFromPredef,
                               FeaturePreemption, FeatureFusedEU]>;
def : Proc<"DG2",             [FeatureLongLongEmulation,
                               FeatureThreadPayloadInMemory,
                               FeatureHasPackedFloat,
                               FeatureInstrBitRotate,
                               FeatureHasLSC, FeatureFusedEU]>;
def : Proc<"PVC",             [FeatureLongLong, FeatureGRFByteSize64,
                               FeatureLSCMaxWidth32,
                               FeatureSwitchjmp,
                               FeatureFP64,
                               FeatureThreadPayloadInMemory,
                               FeatureHasPackedFloat,
                               FeatureInstr64BitRotate,
                               FeatureBfMixedModeWidth16,
                               FeatureHasLSC]>;
def : Proc<"PVCXT_A0",        [FeatureLongLong, FeatureGRFByteSize64,
                               FeatureLSCMaxWidth32,
                               FeaturePartialI64Emulation,
                               FeatureSwitchjmp,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureThreadPayloadInMemory,
                               FeatureInstr64BitRotate,
                               FeatureBfMixedModeWidth16,
                               FeatureHasLSC]>;
def : Proc<"PVCXT",           [FeatureLongLong, FeatureGRFByteSize64,
                               FeatureLSCMaxWidth32,
                               FeaturePartialI64Emulation,
                               FeatureSwitchjmp,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureHasPackedFloat,
                               FeatureThreadPayloadInMemory,
                               FeatureInstr64BitRotate,
                               FeatureBfMixedModeWidth16,
                               FeatureHasLSC]>;

def GenX : Target {
  // Nothing here (yet?)
}

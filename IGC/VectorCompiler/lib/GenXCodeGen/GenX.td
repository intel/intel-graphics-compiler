/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

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
def FeatureIEEEDivSqrt : SubtargetFeature<"ieee_div_sqrt",
                                          "HasIEEEDivSqrt",
                                          "true",
                                          "supports IEEE-754 div and sqrt">;
def FeatureFDivFSqrt64Emulation :
  SubtargetFeature<"fp64_fdiv_fsqrt_emu", "FDivFSqrt64Emu", "true",
                   "emulate double precision division and square root",
                   [FeatureFP64]>;
def FeatureGRFByteSize64 : SubtargetFeature<"grf_byte_size_64",
                                            "GRFByteSize",
                                            "64",
                                            "Every GRF byte size is 64">;
def FeatureLSCMaxWidth32 : SubtargetFeature<"lsc_max_width_32",
                                            "LSCMaxWidth",
                                            "32",
                                            "LSC messages have a maximum width of 32">;
def FeaturePartialI64Emulation : SubtargetFeature<"lightweight_i64_emulation",
                                       "PartialI64Emulation",
                                       "true",
                                       "emulate subset of 64-bit operations">;
def FeatureNoLegacyDataport : SubtargetFeature<"no_legacy_dataport",
                                               "NoLegacyDataport",
                                               "true",
                                               "true if platform has no legacy dataport">;
def FeatureUseMulDDQ : SubtargetFeature<"mul_ddq",
                                        "UseMulDDQ",
                                        "true",
                                        "use native support for mul [U]Dx[U]D->Q">;
def FeatureUseMadDDQ : SubtargetFeature<"mad_ddq",
                                        "UseMadDDQ",
                                        "true",
                                        "use native support for mad DxD+Q->Q">;
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
def FeatureSystolicDenormControl : SubtargetFeature<
  "systolic_denorm_control", "HasSystolicDenormControl", "true",
  "supports control for systolic pipeline types denormal values">;

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

def FeatureInstrLocalIntegerCas64: SubtargetFeature<"localintegercas64",
                                                    "HasLocalIntegerCas64",
                                                    "true",
                                                    "support of local 64-bit integer compare exchange instruction">;

def FeatureInstrGlobalAtomicAddF64: SubtargetFeature<"globaldoubleaddsub",
                                                    "HasGlobalAtomicAddF64",
                                                    "true",
                                                    "support of global double precision atomic add/sub instructions">;

def FeatureInstrAtomicHF16: SubtargetFeature<"halfprecisionatomics",
                                            "HasInstrAtomicHF16",
                                            "true",
                                            "support of half precision atomic instructions">;

def FeatureInstrLocalAtomicAddF32: SubtargetFeature<"localsingleaddsub",
                                                    "HasInstrLocalAtomicAddF32",
                                                    "true",
                                                    "support of local single precision atomic add/sub instructions">;

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

def FeatureHasLSC : SubtargetFeature<"feature_has_lsc",
                                     "HasLSCMessages", "true",
                                     "Target supports LSC messages">;
def FeatureHasLSCTyped : SubtargetFeature<"feature_has_lsc_typed",
                                          "HasLSCTypedMessages", "true",
                                          "Target supports typed LSC messages",
                                          [FeatureHasLSC]>;
def FeatureHasLSCOffset : SubtargetFeature<"feature_has_lsc_offset",
                                           "HasLSCOffset", "true",
                                           "Target supports constant offset for LSC messages",
                                           [FeatureHasLSC]>;
def FeatureTransLegacy : SubtargetFeature<"translate_legacy_message",
                                          "TranslateLegacyMessages",
                                          "true",
                                          "translate legacy message to LSC",
                                          [FeatureHasLSC]>;

def FeatureHasSampler : SubtargetFeature<"feature_has_sampler",
                                         "HasSampler", "true",
                                         "Target supports sampler messages">;

def FeatureHasAdd3 : SubtargetFeature<"feature_has_add3",
                                      "HasAdd3", "true",
                                      "Target supports 3-way addition">;

def FeatureHasBfn : SubtargetFeature<"feature_has_bfn",
                                     "HasBfn", "true",
                                     "Target supports 3-way boolean function">;

def FeatureHasHalfSIMDLSC : SubtargetFeature<"feature_has_half_simd_lsc",
                                     "HasHalfSIMDLSC",
                                     "true",
                                     "Target supports half simd lsc">;

def WarnCallable : SubtargetFeature<"warn_callable", "WarnCallable",
                                    "true", "warn instead of error on callable violation">;

def OCLRuntime : SubtargetFeature<"ocl_runtime", "OCLRuntime", "true",
                                  "Prepare structures for OCL runtime">;

def FeatureMultiIndirectByteRegioning : SubtargetFeature<"multi_indirect_byte_regioning",
                                        "HasMultiIndirectByteRegioning",
                                        "true",
                                        "Vx1 and VxH indirect addressing for Byte datatypes">;

def FeatureWaNoA32ByteScatter : SubtargetFeature<"wa_no_a32_byte_scatter_stateless",
                                                 "HasWaNoA32ByteScatter", "true",
                                                 "Target doesn't support A32 byte scatter stateless message">;

def FeatureIndirectGRFCrossing : SubtargetFeature<"indirect_grf_crossing",
                                                  "HasIndirectGRFCrossing", "true",
                                                  "Target supports an indirect region crossing one GRF boundary">;

def FeatureIndirectByteGRFCrossing : SubtargetFeature<"indirect_byte_grf_crossing",
                                                      "HasIndirectGRFCrossing", "true",
                                                      "Target supports an indirect region crossing one GRF boundary">;

def FeatureSLM64K : SubtargetFeature<"slm_64k",
                                     "MaxSLMSize", "64",
                                     "Target supports up to 64k of SLM">;

def FeatureSLM128K : SubtargetFeature<"slm_128k",
                                      "MaxSLMSize", "128",
                                      "Target supports up to 128k of SLM">;
def FeatureSLM192K : SubtargetFeature<"slm_192k",
                                      "MaxSLMSize", "192",
                                      "Target supports up to 192k of SLM">;
def FeatureSLM384K : SubtargetFeature<"slm_384k",
                                      "MaxSLMSize", "384",
                                      "Target supports up to 384k of SLM">;
def FeatureHasSad2 : SubtargetFeature<"feature_has_sad2",
                                      "HasSad2", "true",
                                      "Target supports sad2/sad2a instructions">;

def FeatureHasOWordSLM : SubtargetFeature<"feature_has_slm_oword",
                                          "HasSLMOWord", "true",
                                          "Target supports OWord block SLM messages">;

def FeatureHasMadSimd32 : SubtargetFeature<"feature_has_mad_simd32",
                                           "HasMadSimd32", "true",
                                           "Target supports SIMD32 mad instruction">;

def FeatureHasNamedBarriers : SubtargetFeature<"feature_has_named_barriers",
                                               "HasNamedBarriers", "true",
                                               "Target supports named barriers">;

def FeatureHasMediaWalker : SubtargetFeature<"feature_has_media_walker",
                                             "HasMediaWalker", "true",
                                             "Target supports media walker interface">;

def FeatureHasLargeGRF : SubtargetFeature<"feature_has_large_grf",
                                          "HasLargeGRF", "true",
                                          "Target supports large GRF mode">;
def FeatureHasVRT : SubtargetFeature<"feature_has_variable_regs_per_thread",
                                     "HasVRT", "true",
                                     "Target supports variable amount of registers per thread">;
def FeatureHas10ThreadsPerEU : SubtargetFeature<"feature_10threads_per_eu",
                                                "NumThreadsPerEU", "10",
                                                "Target has 10 threads per EU">;
def FeatureHas7ThreadsPerEU : SubtargetFeature<"feature_7threads_per_eu",
                                                "NumThreadsPerEU", "7",
                                                "Target has 7 threads per EU">;
def FeatureHas8ThreadsPerEU : SubtargetFeature<"feature_8threads_per_eu",
                                                "NumThreadsPerEU", "8",
                                                "Target has 8 threads per EU">;
def FeatureEfficient64b: SubtargetFeature<
  "efficient_64b", "HasEfficient64b", "true",
  "Target supports efficient 64-bit addressing">;
def FeatureEfficient64bEnabled: SubtargetFeature<
  "efficient_64b_enabled", "EnabledEfficient64b", "true",
  "Efficient 64-bit addressing is enabled for the target">;
def FeatureL1L2L3Cache: SubtargetFeature<
  "l1_l2_l3_cache", "NumCacheLevels", "3",
  "L1, L2 and L3 cache hierarchy is supported for the target">;
def FeatureHasNativeBFloat16 : SubtargetFeature<
  "feature_has_native_bfloat16", "HasNativeBFloat16", "true",
  "Native bfloat16 arithmetic operations are supported by the target">;
def FeatureHasMxfp : SubtargetFeature<
  "feature_has_mxfp", "HasMxfp", "true",
  "Target supports MXFP data types">;
def FeatureAddressRegisterElements32 : SubtargetFeature<"addr_register_size_32",
                                            "AddressRegisterElements",
                                            "32",
                                            "Number of Address Register Elements is 32">;
def FeatureHasEfficientSIMD32: SubtargetFeature<
  "feature_has_efficient_simd32", "HasEfficientSIMD32", "true",
  "support of SIMD32 programming model for all types">;


//===----------------------------------------------------------------------===//
// GenX processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic", []>;

def : Proc<"Gen8", [
  FeatureFP64,
  FeatureHWTIDFromPredef,
  FeatureHas7ThreadsPerEU,
  FeatureHasMediaWalker,
  FeatureHasPackedFloat,
  FeatureHasSad2,
  FeatureHasSampler,
  FeatureIEEEDivSqrt,
  FeatureInstrAdd64,
  FeatureIntDivRem32,
  FeatureLongLong,
  FeatureMultiIndirectByteRegioning,
  FeatureSLM64K,
  FeatureSwitchjmp,
  FeatureWaNoA32ByteScatter,
]>;

def : Proc<"Gen9", [
  FeatureFP64,
  FeatureHWTIDFromPredef,
  FeatureHas7ThreadsPerEU,
  FeatureHasMediaWalker,
  FeatureHasPackedFloat,
  FeatureHasSad2,
  FeatureHasSampler,
  FeatureIEEEDivSqrt,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrAdd64,
  FeatureIntDivRem32,
  FeatureLongLong,
  FeatureMultiIndirectByteRegioning,
  FeaturePreemption,
  FeatureSLM64K,
  FeatureSwitchjmp,
  FeatureUseMulDDQ,
  FeatureWaNoA32ByteScatter,
]>;

def : Proc<"Gen9LP", [
  FeatureFP64,
  FeatureHWTIDFromPredef,
  FeatureHas7ThreadsPerEU,
  FeatureHasMediaWalker,
  FeatureHasPackedFloat,
  FeatureHasSad2,
  FeatureHasSampler,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrAdd64,
  FeatureIntDivRem32,
  FeatureLongLong,
  FeatureMultiIndirectByteRegioning,
  FeaturePreemption,
  FeatureSLM64K,
  FeatureSwitchjmp,
  FeatureUseMulDDQ,
  FeatureWaNoA32ByteScatter,
]>;


def : Proc<"Gen11", [
  FeatureHWTIDFromPredef,
  FeatureHas7ThreadsPerEU,
  FeatureHasMadSimd32,
  FeatureHasMediaWalker,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIEEEDivSqrt,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrBitRotate,
  FeatureIntDivRem32,
  FeatureLongLongEmulation,
  FeatureMultiIndirectByteRegioning,
  FeaturePreemption,
  FeatureSLM64K,
  FeatureSwitchjmp,
]>;

def : Proc<"XeLP", [
  FeatureFusedEU,
  FeatureHWTIDFromPredef,
  FeatureHas7ThreadsPerEU,
  FeatureHasMadSimd32,
  FeatureHasMediaWalker,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrBitRotate,
  FeatureIntDivRem32,
  FeatureLongLongEmulation,
  FeatureMultiIndirectByteRegioning,
  FeaturePreemption,
  FeatureSLM64K,
  FeatureWAFusedEUNoMask,
]>;

def : Proc<"XeHP", [
  FeatureFP64,
  FeatureFusedEU,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIEEEDivSqrt,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrAdd64,
  FeatureInstrBitRotate,
  FeatureLongLong,
  FeatureMultiIndirectByteRegioning,
  FeatureSLM128K,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"XeHPG", [
  FeatureFusedEU,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrBitRotate,
  FeatureLongLongEmulation,
  FeatureMultiIndirectByteRegioning,
  FeatureSLM128K,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"XeLPG", [
  FeatureFDivFSqrt64Emulation,
  FeatureFP64,
  FeatureFusedEU,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrBitRotate,
  FeatureLongLongEmulation,
  FeatureMultiIndirectByteRegioning,
  FeatureSLM128K,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"XeLPGPlus", [
  FeatureFDivFSqrt64Emulation,
  FeatureFP64,
  FeatureFusedEU,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureHasSampler,
  FeatureIndirectByteGRFCrossing,
  FeatureIndirectGRFCrossing,
  FeatureInstrBitRotate,
  FeatureLongLongEmulation,
  FeatureMultiIndirectByteRegioning,
  FeatureSLM128K,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"XeHPC", [
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasNamedBarriers,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalIntegerCas64,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeaturePartialI64Emulation,
  FeatureSLM128K,
  FeatureSwitchjmp,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"XeHPCVG", [
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasNamedBarriers,
  FeatureHasOWordSLM,
  FeatureHasPackedFloat,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalIntegerCas64,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeaturePartialI64Emulation,
  FeatureSLM128K,
  FeatureSwitchjmp,
  FeatureThreadPayloadInMemory,
]>;

def : Proc<"Xe2", [
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLSCOffset,
  FeatureHasLSCTyped,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasNamedBarriers,
  FeatureHasOWordSLM,
  FeatureHasSampler,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalIntegerCas64,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeatureNoLegacyDataport,
  FeaturePartialI64Emulation,
  FeaturePreemption,
  FeatureSLM128K,
  FeatureSwitchjmp,
  FeatureSystolicDenormControl,
  FeatureThreadPayloadInMemory,
  FeatureTransLegacy,
]>;

def : Proc<"Xe3", [
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas10ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLSCOffset,
  FeatureHasLSCTyped,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasNamedBarriers,
  FeatureHasOWordSLM,
  FeatureHasSampler,
  FeatureHasVRT,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalIntegerCas64,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeatureNoLegacyDataport,
  FeaturePartialI64Emulation,
  FeaturePreemption,
  FeatureSLM128K,
  FeatureSwitchjmp,
  FeatureSystolicDenormControl,
  FeatureThreadPayloadInMemory,
  FeatureTransLegacy,
]>;

def : Proc<"Xe3P", [
  FeatureAddressRegisterElements32,
  FeatureEfficient64b,
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas8ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasLSC,
  FeatureHasLSCOffset,
  FeatureHasLSCTyped,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasMxfp,
  FeatureHasNamedBarriers,
  FeatureHasNativeBFloat16,
  FeatureHasOWordSLM,
  FeatureHasVRT,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrAtomicHF16,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalAtomicAddF32,
  FeatureInstrLocalIntegerCas64,
  FeatureL1L2L3Cache,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeatureNoLegacyDataport,
  FeaturePartialI64Emulation,
  FeaturePreemption,
  FeatureSLM384K,
  FeatureSwitchjmp,
  FeatureSystolicDenormControl,
  FeatureThreadPayloadInMemory,
  FeatureTransLegacy,
  FeatureUseMadDDQ,
  FeatureUseMulDDQ,
]>;

def : Proc<"Xe3PLPG", [
  FeatureAddressRegisterElements32,
  FeatureEfficient64b,
  FeatureFP64,
  FeatureGRFByteSize64,
  FeatureHas10ThreadsPerEU,
  FeatureHasAdd3,
  FeatureHasBfn,
  FeatureHasEfficientSIMD32,
  FeatureHasLSC,
  FeatureHasLSCOffset,
  FeatureHasLSCTyped,
  FeatureHasLargeGRF,
  FeatureHasMadSimd32,
  FeatureHasMxfp,
  FeatureHasNamedBarriers,
  FeatureHasNativeBFloat16,
  FeatureHasOWordSLM,
  FeatureHasSampler,
  FeatureHasVRT,
  FeatureIEEEDivSqrt,
  FeatureIndirectGRFCrossing,
  FeatureInstr64BitRotate,
  FeatureInstrAdd64,
  FeatureInstrAtomicHF16,
  FeatureInstrGlobalAtomicAddF64,
  FeatureInstrLocalAtomicAddF32,
  FeatureInstrLocalIntegerCas64,
  FeatureL1L2L3Cache,
  FeatureLSCMaxWidth32,
  FeatureLongLong,
  FeatureNoLegacyDataport,
  FeaturePartialI64Emulation,
  FeaturePreemption,
  FeatureSLM192K,
  FeatureSwitchjmp,
  FeatureSystolicDenormControl,
  FeatureThreadPayloadInMemory,
  FeatureTransLegacy,
  FeatureUseMadDDQ,
  FeatureUseMulDDQ,
]>;

def GenX : Target {
  // Nothing here (yet?)
}

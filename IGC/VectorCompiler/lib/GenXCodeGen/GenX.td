/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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

def FeatureIntDivRem32: SubtargetFeature<"divrem32",
                                         "HasIntDivRem32",
                                         "true",
                                         "supports 32-bit integer division">;
def FeatureInstrAdd64: SubtargetFeature<"add64",
                                       "HasAdd64",
                                       "true",
                                       "enable support for native add64 intruction">;

def FeatureInstrBitRotate: SubtargetFeature<"bitrotate",
                                           "HasBitRotate",
                                           "true",
                                           "support of rol/ror instructions">;


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
// Targets that use compute walker command require loading of thread
// payload by compiler.
def FeatureThreadPayloadInMemory : SubtargetFeature<"thread_payload_in_memory",
                                                    "HasThreadPayloadInMemory",
                                                    "true",
                                                    "need to load thread payload from memory">;

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
                               FeaturePreemption]>;
def : Proc<"RKL",             [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption]>;
def : Proc<"DG1",             [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureInstrBitRotate, FeatureWAFusedEUNoMask,
                               FeatureHWTIDFromPredef,
                               FeatureHasPackedFloat,
                               FeaturePreemption]>;
def : Proc<"XEHP",            [FeatureLongLong,
                               FeatureInstrAdd64,
                               FeatureFP64,
                               FeatureThreadPayloadInMemory,
                               FeatureHasPackedFloat,
                               FeatureInstrBitRotate]>;

def GenX : Target {
  // Nothing here (yet?)
}

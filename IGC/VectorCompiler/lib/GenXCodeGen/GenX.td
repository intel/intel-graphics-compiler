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
// GenX Subtarget features - these are typically passed in as features
//===----------------------------------------------------------------------===//

def DumpRegAlloc: SubtargetFeature<"dump_regalloc", "DumpRegAlloc",
                                   "true", "dump regalloc information">;

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
def : Proc<"HSW",             [FeatureLongLongEmulation, FeatureSwitchjmp,
                               FeatureIntDivRem32]>;
def : Proc<"BDW",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"CHV",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"SKL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"BXT",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"KBL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"GLK",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"CNL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"ICL",             [FeatureLongLong, FeatureSwitchjmp,
                               FeatureIntDivRem32,
                               FeatureUseMulDDQ,
                               FeatureInstrAdd64,
                               FeatureFP64]>;
def : Proc<"ICLLP",           [FeatureLongLongEmulation, FeatureSwitchjmp,
                               FeatureIntDivRem32]>;
def : Proc<"TGLLP",           [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureWAFusedEUNoMask]>;
def : Proc<"DG1",             [FeatureLongLongEmulation, FeatureIntDivRem32,
                               FeatureWAFusedEUNoMask]>;
def : Proc<"XEHP",            [FeatureLongLong,
                               FeatureInstrAdd64,
                               FeatureFP64]>;

def GenX : Target {
  // Nothing here (yet?)
}

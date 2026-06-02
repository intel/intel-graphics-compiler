/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Predicate-flattening regression test for IGCCSPIRVSupportTblGen.
// Each extension exercises one or more PlatformSupport TD shapes; the CHECK
// lines pin the exact Pred:: template emitted into the .inc.
//
// CHECK-DAG is used because the generator emits extensions in TableGen's
// internal record order, not source order. Each pattern is unique enough
// that DAG matching is unambiguous.
//
// RUN: igcc-spirv-support-tblgen -gen-igcc-spirv-extension-support-header \
// RUN:   -I %spirv_extensions_inc %s | FileCheck %s

include "SPIRVExtensions_Common.td"

// Fake platform/core symbols. Their string names flow straight through the
// generator into the emitted Pred:: template arguments — they don't need to
// be real PRODUCT_FAMILY / GFXCORE_FAMILY enumerators because the generator
// never resolves them and FileCheck only inspects the textual output.
def CORE_A : CoreFamily<"CORE_A">;
def CORE_B : CoreFamily<"CORE_B">;
def PLAT_X : Platform<"PLAT_X">;
def PLAT_Y : Platform<"PLAT_Y">;
def PLAT_Z : Platform<"PLAT_Z">;

def XYGroup : PlatformGroup<[PLAT_X, PLAT_Y]>;

// --- AllPlatformSupport singleton ---------------------------------------
def Ext_All : Extension<"Ext_All",
  [ Capability<"Cap_All"> ],
  "https://example.com/Ext_All",
  AllPlatformSupport>;
// CHECK-DAG: {"Ext_All", "https://example.com/Ext_All", Pred::AllPlatformSupport, nullptr,
// CHECK-DAG: {"Cap_All", Pred::AllPlatformSupport, nullptr,

// --- isCoreChildOf ------------------------------------------------------
def Ext_CoreChild : Extension<"Ext_CoreChild",
  [ Capability<"Cap_CoreChild"> ],
  "https://example.com/Ext_CoreChild",
  isCoreChildOf<CORE_A>>;
// CHECK-DAG: {"Ext_CoreChild", "https://example.com/Ext_CoreChild", Pred::isCoreChildOf<CORE_A>, nullptr,
// CHECK-DAG: {"Cap_CoreChild", Pred::isCoreChildOf<CORE_A>, nullptr,

// --- ExactCoreFamily ----------------------------------------------------
def Ext_ExactCore : Extension<"Ext_ExactCore",
  [ Capability<"Cap_ExactCore"> ],
  "https://example.com/Ext_ExactCore",
  ExactCoreFamily<CORE_B>>;
// CHECK-DAG: {"Ext_ExactCore", "https://example.com/Ext_ExactCore", Pred::ExactCoreFamily<CORE_B>, nullptr,
// CHECK-DAG: {"Cap_ExactCore", Pred::ExactCoreFamily<CORE_B>, nullptr,

// --- isProductChildOf ---------------------------------------------------
def Ext_ProductChild : Extension<"Ext_ProductChild",
  [ Capability<"Cap_ProductChild"> ],
  "https://example.com/Ext_ProductChild",
  isProductChildOf<PLAT_X>>;
// CHECK-DAG: {"Ext_ProductChild", "https://example.com/Ext_ProductChild", Pred::isProductChildOf<PLAT_X>, nullptr,
// CHECK-DAG: {"Cap_ProductChild", Pred::isProductChildOf<PLAT_X>, nullptr,

// --- ExactPlatform ------------------------------------------------------
def Ext_ExactPlatform : Extension<"Ext_ExactPlatform",
  [ Capability<"Cap_ExactPlatform"> ],
  "https://example.com/Ext_ExactPlatform",
  ExactPlatform<PLAT_Y>>;
// CHECK-DAG: {"Ext_ExactPlatform", "https://example.com/Ext_ExactPlatform", Pred::ExactPlatform<PLAT_Y>, nullptr,
// CHECK-DAG: {"Cap_ExactPlatform", Pred::ExactPlatform<PLAT_Y>, nullptr,

// --- isInGroup (variadic over group members) ----------------------------
def Ext_InGroup : Extension<"Ext_InGroup",
  [ Capability<"Cap_InGroup"> ],
  "https://example.com/Ext_InGroup",
  isInGroup<XYGroup>>;
// CHECK-DAG: {"Ext_InGroup", "https://example.com/Ext_InGroup", Pred::isInGroup<PLAT_X, PLAT_Y>, nullptr,
// CHECK-DAG: {"Cap_InGroup", Pred::isInGroup<PLAT_X, PLAT_Y>, nullptr,

// --- AnyOf, AllOf, Not (composed) ---------------------------------------
def Ext_Composed : Extension<"Ext_Composed",
  [ Capability<"Cap_Composed"> ],
  "https://example.com/Ext_Composed",
  AllOf<[ isCoreChildOf<CORE_A>,
          Not<ExactPlatform<PLAT_Z>>,
          AnyOf<[ ExactPlatform<PLAT_X>, ExactPlatform<PLAT_Y> ]> ]>>;
// CHECK-DAG: {"Ext_Composed", "https://example.com/Ext_Composed", Pred::AllOf<Pred::isCoreChildOf<CORE_A>, Pred::Not<Pred::ExactPlatform<PLAT_Z>>, Pred::AnyOf<Pred::ExactPlatform<PLAT_X>, Pred::ExactPlatform<PLAT_Y>>>, nullptr,
// CHECK-DAG: {"Cap_Composed", Pred::AllOf<Pred::isCoreChildOf<CORE_A>, Pred::Not<Pred::ExactPlatform<PLAT_Z>>, Pred::AnyOf<Pred::ExactPlatform<PLAT_X>, Pred::ExactPlatform<PLAT_Y>>>, nullptr,

// --- NotSupported singleton ---------------------------------------------
// NotSupported collapses to nullptr in BOTH the extension column and the
// capability column (emitTierColumn special-cases it). The capability
// inherits NotSupported from the extension via InheritFromExtension default.
def Ext_None : Extension<"Ext_None",
  [ Capability<"Cap_None"> ],
  "https://example.com/Ext_None",
  NotSupported>;
// CHECK-DAG: {"Ext_None", "https://example.com/Ext_None", nullptr, nullptr,
// CHECK-DAG: {"Cap_None", nullptr, nullptr,

// --- InheritFromCapabilities aggregation --------------------------------
// Extension column collapses to nullptr (signalling aggregate mode);
// per-capability columns carry the real predicates.
def Ext_Aggregate : Extension<"Ext_Aggregate",
  [ Capability<"Cap_Aggr_A", isCoreChildOf<CORE_A>>,
    Capability<"Cap_Aggr_B", ExactPlatform<PLAT_Z>> ],
  "https://example.com/Ext_Aggregate",
  InheritFromCapabilities>;
// CHECK-DAG: {"Ext_Aggregate", "https://example.com/Ext_Aggregate", nullptr, nullptr,
// CHECK-DAG: {"Cap_Aggr_A", Pred::isCoreChildOf<CORE_A>, nullptr,
// CHECK-DAG: {"Cap_Aggr_B", Pred::ExactPlatform<PLAT_Z>, nullptr,

// --- Experimental tier (separate column) --------------------------------
// Capability ID is "0" because the extension has no production support
// (formatCapabilityId requires both Ext.ProductionSupport and
// Cap.ProductionSupport to be non-null before reporting a real ID).
def Ext_Exp_experimental : ExperimentalExtension<"Ext_Exp",
  [ Capability<"Cap_Exp"> ],
  "https://example.com/Ext_Exp",
  ExactPlatform<PLAT_X>>;
// CHECK-DAG: {"Ext_Exp", "https://example.com/Ext_Exp", nullptr, Pred::ExactPlatform<PLAT_X>,
// CHECK-DAG: {"Cap_Exp", nullptr, Pred::ExactPlatform<PLAT_X>, 0},

// --- Dual definition: production + experimental on the same name --------
def Ext_Dual : Extension<"Ext_Dual",
  [ Capability<"Cap_Dual"> ],
  "https://example.com/Ext_Dual",
  isCoreChildOf<CORE_A>>;
def Ext_Dual_experimental : ExperimentalExtension<"Ext_Dual",
  [ Capability<"Cap_Dual"> ],
  "https://example.com/Ext_Dual",
  isCoreChildOf<CORE_B>>;
// CHECK-DAG: {"Ext_Dual", "https://example.com/Ext_Dual", Pred::isCoreChildOf<CORE_A>, Pred::isCoreChildOf<CORE_B>,
// CHECK-DAG: {"Cap_Dual", Pred::isCoreChildOf<CORE_A>, Pred::isCoreChildOf<CORE_B>,

// --- Capability ID handling --------------------------------------------
// Default (Cap.Id == -1): the generator emits a static_cast over
//   spv::Capability<Name> from the vendored SPIRV-Headers, so the consumer
//   compiles iff <Name> exists in spv::.
// CapabilityWithManualId<Name, Id>: the generator emits the literal ID
//   followed by 'u'. Use this for capabilities not yet in the headers.
// Manual and default IDs can be mixed within one extension.
def Ext_IdMix : Extension<"Ext_IdMix",
  [ Capability<"Cap_IdDefault">,
    CapabilityWithManualId<"Cap_IdManual", 4242> ],
  "https://example.com/Ext_IdMix",
  AllPlatformSupport>;
// CHECK-DAG: {"Cap_IdDefault", Pred::AllPlatformSupport, nullptr, static_cast<uint32_t>(spv::CapabilityCap_IdDefault)},
// CHECK-DAG: {"Cap_IdManual", Pred::AllPlatformSupport, nullptr, 4242u},

// Manual ID on an experimental-only extension still collapses to "0",
// because formatCapabilityId gates the ID on production support being
// present at BOTH the extension and capability levels.
def Ext_IdExp_experimental : ExperimentalExtension<"Ext_IdExp",
  [ CapabilityWithManualId<"Cap_IdExp", 7777> ],
  "https://example.com/Ext_IdExp",
  AllPlatformSupport>;
// CHECK-DAG: {"Cap_IdExp", nullptr, Pred::AllPlatformSupport, 0},

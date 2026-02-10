/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_MODREF_H
#define IGCLLVM_SUPPORT_MODREF_H

#include "IGC/common/LLVMWarningsPush.hpp"
#if LLVM_VERSION_MAJOR >= 17 && !defined(IGC_LLVM_TRUNK_REVISION)
#include "llvm/IR/AttributeMask.h"
#endif
#if LLVM_VERSION_MAJOR >= 16
#include "llvm/Support/ModRef.h"
#endif // LLVM_VERSION_MAJOR
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/LLVMContext.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#include <Probe/Assertion.h>
#include <optional>
#include <vector>

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 16
using ModRefInfo = llvm::ModRefInfo;
#else  // LLVM_VERSION_MAJOR
/// Below is a mere copy of llvm::ModRefInfo
enum class ModRefInfo : uint8_t {
  /// The access neither references nor modifies the value stored in memory.
  NoModRef = 0,
  /// The access may reference the value stored in memory.
  Ref = 1,
  /// The access may modify the value stored in memory.
  Mod = 2,
  /// The access may reference and may modify the value stored in memory.
  ModRef = Ref | Mod,
};
#endif // LLVM_VERSION_MAJOR

/// This enumeration serves as a translation medium between pre-LLVM 16
/// (attribute-based) and LLVM 16+ (location bitmask) semantics for IR memory
/// location info. The key restriction before LLVM 16 is exclusiveness: an
/// explicit specification of a memory location implies "no access" (NoModRef)
/// for other locations.
enum class ExclusiveIRMemLoc : uint8_t {
  Any = 0,
  ArgMem = 1,
  InaccessibleMem = 2,
  InaccessibleOrArgMem = ArgMem | InaccessibleMem,
};

/// The 'detail' namespace implements the translation logic between
/// pre-LLVM 16, WrapperLLVM and LLVM 16+ layouts of attributes:
/*=============================================================================
/-----------------------------------------------------------------------------\
|                Allowed memory access                                        |
|-----------------------------------------------------------------------------|
| LLVM 16 memory effects & LLVM Wrapper | Pre-LLVM 16 attr (if any)           |
|-----------------------------------------------------------------------------|
| ModRefInfo::NoModRef                  | ReadNone                            |
| ModRefInfo::Ref                       | ReadOnly                            |
| ModRefInfo::Mod                       | WriteOnly                           |
| ModRefInfo::ModRef                    | no location/access attrs            |
\-----------------------------------------------------------------------------/

/-----------------------------------------------------------------------------\
|                Memory location - LLVM 16+                                   |
|-----------------------------------------------------------------------------|
| LLVM Wrapper                             | LLVM 16 memory effects           |
|-----------------------------------------------------------------------------|
| ExclusiveIRMemLoc::ArgMem                | ::Location::ArgMem               |
| ExclusiveIRMemLoc::InaccessibleMem       | ::Location::InaccessibleMem      |
| "other location" unsupported             | ::Location::Other                |
| ExclusiveIRMemLoc::InaccessibleOrArgMem  | ArgMem + InaccessibleMem         |
| ExclusiveIRMemLoc::Any                   | ArgMem + InaccessibleMem + Other |
|-----------------------------------------------------------------------------|
|                Memory location - pre-LLVM 16                                |
|-----------------------------------------------------------------------------|
| LLVM Wrapper                             | Pre-LLVM 16 attr (if any)        |
|-----------------------------------------------------------------------------|
| ExclusiveIRMemLoc::ArgMem                | ArgMemOnly                       |
| ExclusiveIRMemLoc::InaccessibleMem       | InaccessibleMemOnly              |
| "other location" unsupported             | "other location" unsupported     |
| ExclusiveIRMemLoc::InaccessibleOrArgMem  | InaccessibleMemOrArgMemOnly      |
| ExclusiveIRMemLoc::Any                   | no location attrs                |
\-----------------------------------------------------------------------------/
=============================================================================*/
namespace detail {
using OptionalAttrKind = std::optional<llvm::Attribute::AttrKind>;
#undef IGCLLVM_SELECT_LLVM_16
// TODO: Factor out into a common preprocesser helper file
#if LLVM_VERSION_MAJOR >= 16
#define IGCLLVM_SELECT_LLVM_16(entry_ge_llvm16, entry_lt_llvm16) entry_ge_llvm16
#else // LLVM_VERSION_MAJOR
#define IGCLLVM_SELECT_LLVM_16(entry_ge_llvm16, entry_lt_llvm16) entry_lt_llvm16
#endif

using MemoryEffectsLLVMTy = IGCLLVM_SELECT_LLVM_16(llvm::MemoryEffects, OptionalAttrKind);

/// FIXME: An inheritance-based implementation would be more elegant for
/// translateMemoryEffect(...) helpers, especially if a constexpr map-like
/// structure. This would be more expressive for the translation logic (no need
/// for comment block descriptions as above). Additionally, were the map to
/// provide "vector of keys" getter capabilities (akin to C++23 std::flat_map),
/// we could then encapsulate MemoryEffects::getOverridenAttrKinds().
inline MemoryEffectsLLVMTy translateMemoryEffect(ExclusiveIRMemLoc Loc) {
  if (Loc == ExclusiveIRMemLoc::ArgMem)
    return IGCLLVM_SELECT_LLVM_16(llvm::MemoryEffects::argMemOnly(), llvm::Attribute::ArgMemOnly);
  if (Loc == ExclusiveIRMemLoc::InaccessibleMem)
    return IGCLLVM_SELECT_LLVM_16(llvm::MemoryEffects::inaccessibleMemOnly(), llvm::Attribute::InaccessibleMemOnly);
  if (Loc == ExclusiveIRMemLoc::InaccessibleOrArgMem)
    return IGCLLVM_SELECT_LLVM_16(llvm::MemoryEffects::inaccessibleOrArgMemOnly(),
                                  llvm::Attribute::InaccessibleMemOrArgMemOnly);
  // No restrictions
  IGC_ASSERT_MESSAGE(Loc == ExclusiveIRMemLoc::Any, "Unknown ExclusiveIRMemLoc value");
  return IGCLLVM_SELECT_LLVM_16(llvm::MemoryEffects::unknown(), std::nullopt);
}
inline MemoryEffectsLLVMTy translateMemoryEffect(ModRefInfo MR) {
#if LLVM_VERSION_MAJOR >= 16
  return MemoryEffectsLLVMTy(MR);
#else  // LLVM_VERSION_MAJOR
  if (MR == ModRefInfo::NoModRef)
    return llvm::Attribute::ReadNone;
  if (MR == ModRefInfo::Ref)
    return llvm::Attribute::ReadOnly;
  if (MR == ModRefInfo::Mod)
    return llvm::Attribute::WriteOnly;
  // No restrictions
  IGC_ASSERT_MESSAGE(MR == ModRefInfo::ModRef, "Unknown ModRefInfo value");
  return {};
#endif // LLVM_VERSION_MAJOR
}
#undef IGCLLVM_SELECT_LLVM_16
} // namespace detail

/// This wrapper serves to reduce LLVM 16's MemoryEffects API to a level that
/// can be feasibly emulated through legacy attributes:
/// - it should be possible to initialize the wrapper based on static data,
///   same as with the original MemoryEffects. Therefore, no dynamic objects
///   like LLVMContext/AttributeSet are stored per instance - only accepted as
///   parameters/returned by certain methods
/// - matematical operations made available with LLVM 16 are forbidden through
///   composition. To emulate and/or semantics, we'd have to store a dynamic
///   attribute set, violating the above design and resorting to error-prone
///   AttrKind manipulations for each operation
/// - because of the "exclusive" memory location semantics, there is no
///   iteration over locations
///
/// Since LLVM 16+ MemoryEffects are only an interim descriptor used for
/// llvm::Attribute::Memory generation, any collection of contextual attributes
/// can still work as the ultimate version wrapper, dynamically queried by IGC
/// code to fetch and operate on legacy/new attribute layout (*).
///
/// NB: Static generators of custom intrinsics are the exemplary use-case for
/// this wrapper, since lightweight, context-independent info about memory
/// effects may have to be stored before the actual instantiation. Whenever
/// possible, stable high-level APIs provided by llvm::Function,
/// llvm::CallBase, etc. should actually be preferred to manual
/// MemoryEffects-based initialization of attributes.
///
/// (*) https://github.com/llvm/llvm-project/blob/release/16.x/llvm/docs/ReleaseNotes.rst#changes-to-the-llvm-ir
class MemoryEffects {
private:
  ExclusiveIRMemLoc Loc;
  ModRefInfo MR;

public:
  constexpr MemoryEffects(ExclusiveIRMemLoc Loc, ModRefInfo MR) : Loc(Loc), MR(MR) {}

  explicit constexpr MemoryEffects(ModRefInfo MR) : MemoryEffects(ExclusiveIRMemLoc::Any, MR) {}
  MemoryEffects(const MemoryEffects &) = default;
  MemoryEffects &operator=(const MemoryEffects &) = default;
  ~MemoryEffects() = default;

  /// Constructs and returns an LLVMContext-dependent attribute set based on
  /// the static memory effects' info.
  /// For LLVM 16+, a single Memory attribute is constructed.
  /// For earlier versions, up to 2 legacy attributes can be returned.
  llvm::AttributeSet getAsAttributeSet(llvm::LLVMContext &C) const {
#if LLVM_VERSION_MAJOR >= 16
    llvm::MemoryEffects ME = detail::translateMemoryEffect(Loc) & detail::translateMemoryEffect(MR);
    auto MemAttr = llvm::Attribute::getWithMemoryEffects(C, ME);
    return llvm::AttributeSet::get(C, {MemAttr});
#else  // LLVM_VERSION_MAJOR
    llvm::AttributeSet ResultSet;
    if (auto LocAttr = detail::translateMemoryEffect(Loc))
      ResultSet = ResultSet.addAttribute(C, *LocAttr);
    if (auto ModRefAttr = detail::translateMemoryEffect(MR))
      ResultSet = ResultSet.addAttribute(C, *ModRefAttr);
    return ResultSet;
#endif // LLVM_VERSION_MAJOR
  }

  /// Returns an AttrBuilder based on the static memory effects' info.
  llvm::AttrBuilder getAsAttrBuilder(llvm::LLVMContext &C) const { return llvm::AttrBuilder(C, getAsAttributeSet(C)); }

  /// Pre-LLVM 16, returns a list of attribute kinds that would conflict with
  /// underlying memory effect attributes for the given
  /// IGCLLVM::MemoryEffects instance. The list is empty for LLVM 16+.
  llvm::AttributeMask getOverridenAttrKinds() const {
#if LLVM_VERSION_MAJOR >= 16
    // The list is empty for LLVM 16+, which corresponds to the standard LLORG
    // practice of retaining old instances of Attribute::Memory and relying on
    // the internal logic to merge the memory effects correctly.
    return {};
#else  // LLVM_VERSION_MAJOR
    using AttrVector = std::vector<llvm::Attribute::AttrKind>;
    AttrVector ModRefList{llvm::Attribute::ReadNone, llvm::Attribute::ReadOnly, llvm::Attribute::WriteOnly};
    AttrVector LocsList{llvm::Attribute::ArgMemOnly, llvm::Attribute::InaccessibleMemOnly,
                        llvm::Attribute::InaccessibleMemOrArgMemOnly};
    llvm::AttributeMask Result;

    AttrVector ResultVec(ModRefList.begin(), ModRefList.end());
    for (auto ModRefKind : ModRefList)
      Result.addAttribute(ModRefKind);
    if (Loc != ExclusiveIRMemLoc::Any || MR == ModRefInfo::NoModRef) {
      for (auto LocKind : LocsList)
        Result.addAttribute(LocKind);
    }

    return Result;
#endif // LLVM_VERSION_MAJOR
  }

  /// Define static getters matching those from LLVM 16+ llvm::MemoryEffects.
  /// The logic is simple and sound enough to replicate.
  static constexpr MemoryEffects unknown() { return MemoryEffects(ModRefInfo::ModRef); }
  static constexpr MemoryEffects none() { return MemoryEffects(ModRefInfo::NoModRef); }
  static constexpr MemoryEffects readOnly() { return MemoryEffects(ModRefInfo::Ref); }
  static constexpr MemoryEffects writeOnly() { return MemoryEffects(ModRefInfo::Mod); }
  static constexpr MemoryEffects argMemOnly(ModRefInfo MR = ModRefInfo::ModRef) {
    return MemoryEffects(ExclusiveIRMemLoc::ArgMem, MR);
  }
  static constexpr MemoryEffects inaccessibleMemOnly(ModRefInfo MR = ModRefInfo::ModRef) {
    return MemoryEffects(ExclusiveIRMemLoc::InaccessibleMem, MR);
  }
  static constexpr MemoryEffects inaccessibleOrArgMemOnly(ModRefInfo MR = ModRefInfo::ModRef) {
    return MemoryEffects(ExclusiveIRMemLoc::InaccessibleOrArgMem, MR);
  }
};

} // end namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_MODREF_H

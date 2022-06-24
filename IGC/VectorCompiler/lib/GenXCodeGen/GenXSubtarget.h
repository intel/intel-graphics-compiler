/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXSubtarget : subtarget information
/// -------------------------------------
///
/// GenXSubtarget is the GenX-specific subclass of TargetSubtargetInfo. It takes
/// features detected by the front end (what the Gen architecture is),
/// and exposes flags to the rest of the GenX backend for
/// various features (e.g. whether 64 bit operations are supported).
///
/// Where subtarget features are used is noted in the documentation of GenX
/// backend passes.
///
/// The flags exposed to the rest of the GenX backend are as follows. Most of
/// these are currently not used.
///
//===----------------------------------------------------------------------===//

#ifndef GENXSUBTARGET_H
#define GENXSUBTARGET_H

#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "visa_igc_common_header.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#define GET_SUBTARGETINFO_ENUM
#include "GenXGenSubtargetInfo.inc"

namespace llvm {
class GlobalValue;
class Instruction;
class StringRef;
class TargetMachine;

class GenXSubtarget final : public GenXGenSubtargetInfo {

protected:
  // TargetTriple - What processor and OS we're targeting.
  Triple TargetTriple;

  enum GenXTag {
    UNDEFINED_ARCH,
    GENERIC_ARCH,
    GENX_BDW,
    GENX_SKL,
    GENX_BXT,
    GENX_KBL,
    GENX_GLK,
    GENX_ICLLP,
    GENX_TGLLP,
    GENX_RKL,
    GENX_DG1,
    GENX_ADLS,
    GENX_ADLP,
    GENX_ADLN,
    XE_HP_SDV,
    XE_DG2,
    XE_PVC,
    XE_PVCXT_A0,
    XE_PVCXT,
  };

  // GenXVariant - GenX Tag identifying the variant to compile for
  GenXTag GenXVariant;

private:
  // HasLongLong - True if subtarget supports long long type
  bool HasLongLong = false;

  // HasFP64 - True if subtarget supports double type
  bool HasFP64 = false;

  // HasIEEEDivSqrt - True if subtarget supports IEEE-754 div and sqrt
  bool HasIEEEDivSqrt = false;

  // DisableJmpi - True if jmpi is disabled.
  bool DisableJmpi = false;

  // DisableVectorDecomposition - True if vector decomposition is disabled.
  bool DisableVectorDecomposition = false;

  // DisableJumpTables - True if switch to jump tables lowering is disabled.
  bool DisableJumpTables = false;

  // Only generate warning when callable is used in the middle of the kernel
  bool WarnCallable = false;

  // Size of one general register in bytes.
  unsigned GRFByteSize = 32;

  // Maximum width of LSC messages.
  unsigned LSCMaxWidth = 16;

  // True if legacy data-port messages are disabled
  bool TranslateLegacyMessages = false;

  // Currenly used for PVC B-stepping (some i64 operations are unsupported)
  bool PartialI64Emulation = false;
  // Some targets do not support i64 ops natively, we have an option to emulate
  bool EmulateLongLong = false;

  // True if target supports native 64-bit add
  bool HasAdd64 = false;

  // True if it is profitable to use native DxD->Q multiplication
  bool UseMulDDQ = false;

  // True if codegenerating for OCL runtime.
  bool OCLRuntime = false;

  // True if subtarget supports switchjmp visa instruction
  bool HasSwitchjmp = false;

  // True if subtarget supports preemption
  bool HasPreemption = false;

  // True if subtarget requires WA for nomask instructions under divergent
  // control flow
  bool WaNoMaskFusedEU = false;

  // True if subtarget has fused EUs
  bool HasFusedEU = false;

  // True if subtarget supports 32-bit integer division
  bool HasIntDivRem32 = false;

  // True if subtarget supports 32-bit rol/ror instructions
  bool HasBitRotate = false;

  // True if subtarget supports 64-bit rol/ror instructions
  bool Has64BitRotate = false;

  // True if subtarget gets HWTID from predefined variable
  bool GetsHWTIDFromPredef = false;

  // True is thread payload should be loaded from memory.
  bool HasThreadPayloadInMemory = false;

  // Has L1 read-only cache.
  bool HasL1ReadOnlyCache = false;

  // Supress local memory fence.
  bool HasLocalMemFenceSupress = false;

  /// Packed float immediate vector operands are supported.
  bool HasPackedFloat = false;

  /// True if subtarget accepts 16-wide BF mixed mode operations
  bool HasBfMixedModeWidth16 = false;

  /// True if subtarget supports LSC messages
  bool HasLSCMessages = false;

  /// True if subtarget supports half SIMD LSC messages
  bool HasHalfSIMDLSC = false;

  // Has multi-tile.
  bool HasMultiTile = false;

  // Has L3 cache-coherent cross tiles.
  bool HasL3CacheCoherentCrossTiles = false;

  // Has L3 flush on GPU-scope invalidate.
  bool HasL3FlushOnGPUScopeInvalidate = false;

  // Shows which surface should we use for stack
  PreDefined_Surface StackSurf;

public:
  // This constructor initializes the data members to match that
  // of the specified triple.
  //
  GenXSubtarget(const Triple &TT, const std::string &CPU,
                const std::string &FS);

  // GRF size in bytes.
  unsigned getGRFByteSize() const { return GRFByteSize; }

  // LSC instructions can operate either in full SIMD mode or
  // in half SIMD mode. This defines how many registers are
  // used by the data payload.
  // getLSCMinWidth() returns half of the maximum SIMD width.
  // getLSCMaxWidth() returns the maximum SIMD width.
  // Instructions narrower than getLSCMinWidth() still use
  // the same amount of registers for their data payload
  // as if they were getLSCMinWidth() wide.
  unsigned getLSCMinWidth() const { return getLSCMaxWidth() / 2; }
  unsigned getLSCMaxWidth() const { return LSCMaxWidth; }

  // The maximum amount of registers that an LSC message's data payload
  // can take up.
  unsigned getLSCMaxDataRegisters() const { return 8; }

  bool isOCLRuntime() const { return OCLRuntime; }

  // ParseSubtargetFeatures - Parses features string setting specified
  // subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU,
#if LLVM_VERSION_MAJOR >= 12
                              StringRef TuneCPU,
#endif
                              StringRef FS);

  // \brief Initialize the features for the GenX target.
  void initSubtargetFeatures(StringRef CPU, StringRef FS);

public:

  /// * isBDW - true if target is BDW
  bool isBDW() const { return GenXVariant == GENX_BDW; }

  /// * isBDWplus - true if target is BDW or later
  bool isBDWplus() const { return GenXVariant >= GENX_BDW; }

  /// * isSKL - true if target is SKL
  bool isSKL() const { return GenXVariant == GENX_SKL; }

  /// * isSKLplus - true if target is SKL or later
  bool isSKLplus() const { return GenXVariant >= GENX_SKL; }

  /// * isBXT - true if target is BXT
  bool isBXT() const { return GenXVariant == GENX_BXT; }

  /// * isKBL - true if target is KBL
  bool isKBL() const { return GenXVariant == GENX_KBL; }

  /// * isGLK - true if target is GLK
  bool isGLK() const { return GenXVariant == GENX_GLK; }

  /// * isICLLPplus - true if target is ICLLP or later
  bool isICLLPplus() const { return GenXVariant >= GENX_ICLLP; }

  /// * isICLLP - true if target is ICL LP
  bool isICLLP() const { return GenXVariant == GENX_ICLLP; }
  /// * isTGLLP - true if target is TGL LP
  bool isTGLLP() const { return GenXVariant == GENX_TGLLP; }
  /// * isRKL - true if target is RKL
  bool isRKL() const { return GenXVariant == GENX_RKL; }
  /// * isDG1 - true if target is DG1
  bool isDG1() const { return GenXVariant == GENX_DG1; }
  /// * isXEHP - true if target is XEHP
  bool isXEHP() const {
    return GenXVariant == XE_HP_SDV;
  }
  /// * isADLS - true if target is ADLS
  bool isADLS() const { return GenXVariant == GENX_ADLS; }
  /// * isADLP - true if target is ADLP
  bool isADLP() const { return GenXVariant == GENX_ADLP; }
  /// * isADLN - true if target is ADLN
  bool isADLN() const { return GenXVariant == GENX_ADLN; }
  /// * translateMediaWalker - true if translate media walker APIs
  bool translateMediaWalker() const { return GenXVariant >= XE_HP_SDV; }
  // TODO: consider implementing 2 different getters
  /// * has add3 and bfn instructions
  bool hasAdd3Bfn() const { return GenXVariant >= XE_HP_SDV; }
  int dpasWidth() const {
    if (isPVC())
      return 16;
    return 8;
  }
  unsigned bfMixedModeWidth() const {
    if (HasBfMixedModeWidth16)
      return 16;
    return 8;
  }
  /// * isDG2 - true if target is DG2
  bool isDG2() const { return GenXVariant == XE_DG2; }
  /// * isPVC - true if target is PVC
  bool isPVC() const { return isPVCXL() || isPVCXT_A0() || isPVCXT(); }

  /// * isPVCXT - true if target is PVCXT
  bool isPVCXT() const { return GenXVariant == XE_PVCXT; }

  /// * isPVCXT_A0 - true if target is PVCXT_A0
  bool isPVCXT_A0() const { return GenXVariant == XE_PVCXT_A0; }

  /// * isPVCXL - true if target is PVCXL
  bool isPVCXL() const { return GenXVariant == XE_PVC; }

  int getNumElementsInAddrReg() const {
    if (isPVC())
      return 16;
    return 8;
  }

  bool translateLegacyMessages() const {
    return (HasLSCMessages && TranslateLegacyMessages);
  }

  bool hasHalfSIMDLSC() const { return HasHalfSIMDLSC; }

  bool partialI64Emulation() const { return PartialI64Emulation; }

  /// * hasPackedFloat - true if packed float immediate vector operands are supported
  bool hasPackedFloat() const { return HasPackedFloat; }

  /// * emulateLongLong - true if i64 emulation is requested
  bool emulateLongLong() const { return EmulateLongLong; }

  /// * hasLongLong - true if target supports long long
  bool hasLongLong() const { return HasLongLong; }

  /// * hasFP64 - true if target supports double fp
  bool hasFP64() const { return HasFP64; }

  /// * hasIEEEDivSqrt - true if target supports IEEE-754 div and sqrt
  bool hasIEEEDivSqrt() const { return HasIEEEDivSqrt; }

  /// * hasAdd64 - true if target supports native 64-bit add/sub
  bool hasAdd64() const { return HasAdd64; }

  /// * useMulDDQ - true if is desired to emit DxD->Q mul instruction
  bool useMulDDQ() const { return UseMulDDQ; }

  /// * disableJmpi - true if jmpi is disabled.
  bool disableJmpi() const { return DisableJmpi; }

  /// * WaNoA32ByteScatteredStatelessMessages - true if there is no A32 byte
  ///   scatter stateless message.
  bool WaNoA32ByteScatteredStatelessMessages() const { return !isICLLPplus(); }

  /// * disableVectorDecomposition - true if vector decomposition is disabled.
  bool disableVectorDecomposition() const { return DisableVectorDecomposition; }

  /// * disableJumpTables - true if switch to jump tables lowering is disabled.
  bool disableJumpTables() const { return DisableJumpTables; }

  /// * has switchjmp instruction
  bool hasSwitchjmp() const { return HasSwitchjmp; }

  /// * has preemption
  bool hasPreemption() const { return HasPreemption; }

  /// * needsWANoMaskFusedEU() - true if we need to apply WA for NoMask ops
  bool needsWANoMaskFusedEU() const { return WaNoMaskFusedEU; }

  /// * hasFusedEU() - true if subtarget has fused EUs
  bool hasFusedEU() const { return HasFusedEU; }

  /// * has integer div/rem instruction
  bool hasIntDivRem32() const { return HasIntDivRem32; }

  /// * warnCallable() - true if compiler only generate warning for
  ///   callable in the middle
  bool warnCallable() const { return WarnCallable; }

  /// * hasIndirectGRFCrossing - true if target supports an indirect region
  ///   crossing one GRF boundary
  bool hasIndirectGRFCrossing() const { return isSKLplus(); }

  /// * hasIndirectByteGRFCrossing - true if target supports an indirect region
  ///   crossing one GRF boundary with byte type
  bool hasIndirectByteGRFCrossing() const {
    return hasIndirectGRFCrossing() && !isPVC();
  }

  /// * hasMultiIndirectByteRegioning - true if target supports an mutli
  /// indirect regions with byte type
  bool hasMultiIndirectByteRegioning() const { return !isPVC(); }

  bool hasNBarrier() const { return GenXVariant >= XE_PVC; }

  /// * getMaxSlmSize - returns maximum allowed SLM size (in KB)
  unsigned getMaxSlmSize() const {
    if (isXEHP() || isDG2() || isPVC())
      return 128;
    return 64;
  }

  bool hasThreadPayloadInMemory() const { return HasThreadPayloadInMemory; }

  /// * hasSad2Support - returns true if sad2/sada2 are supported by target
  bool hasSad2Support() const {
    if (isICLLP() || isTGLLP())
      return false;
    if (isDG1())
      return false;
    if (isXEHP())
      return false;
    if (isDG2() || isPVC())
      return false;
    return true;
  }

  bool hasBitRotate() const { return HasBitRotate; }
  bool has64BitRotate() const { return Has64BitRotate; }

  bool hasL1ReadOnlyCache() const { return HasL1ReadOnlyCache; }
  bool hasLocalMemFenceSupress() const { return HasLocalMemFenceSupress; }
  bool hasMultiTile() const { return HasMultiTile; };
  bool hasL3CacheCoherentCrossTiles() const {
    return HasL3CacheCoherentCrossTiles;
  }
  bool hasL3FlushOnGPUScopeInvalidate() const {
    return HasL3FlushOnGPUScopeInvalidate;
  }

  /// * getsHWTIDFromPredef - some subtargets get HWTID from
  // predefined variable instead of sr0, returns *true* for such ones.
  bool getsHWTIDFromPredef() const { return GetsHWTIDFromPredef; }

  // Generic helper functions...
  const Triple &getTargetTriple() const { return TargetTriple; }

  TARGET_PLATFORM getVisaPlatform() const {
    switch (GenXVariant) {
    case GENX_BDW:
      return TARGET_PLATFORM::GENX_BDW;
    case GENX_SKL:
      return TARGET_PLATFORM::GENX_SKL;
    case GENX_BXT:
      return TARGET_PLATFORM::GENX_BXT;
    case GENX_ICLLP:
      return TARGET_PLATFORM::GENX_ICLLP;
    case GENX_TGLLP:
      return TARGET_PLATFORM::GENX_TGLLP;
    case GENX_RKL:
      return TARGET_PLATFORM::GENX_TGLLP;
    case GENX_DG1:
      return TARGET_PLATFORM::GENX_TGLLP;
    case XE_HP_SDV:
      return TARGET_PLATFORM::Xe_XeHPSDV;
    case GENX_ADLS:
      return TARGET_PLATFORM::GENX_TGLLP;
    case GENX_ADLP:
      return TARGET_PLATFORM::GENX_TGLLP;
    case GENX_ADLN:
      return TARGET_PLATFORM::GENX_TGLLP;
    case XE_DG2:
      return TARGET_PLATFORM::Xe_DG2;
    case XE_PVC:
      return TARGET_PLATFORM::Xe_PVC;
    case XE_PVCXT_A0:
    case XE_PVCXT:
      return TARGET_PLATFORM::Xe_PVCXT;
    case GENX_KBL:
      return TARGET_PLATFORM::GENX_SKL;
    case GENX_GLK:
      return TARGET_PLATFORM::GENX_BXT;
    default:
      return TARGET_PLATFORM::GENX_NONE;
    }
  }

  /// * stackSurface - return a surface that should be used for stack.
  PreDefined_Surface stackSurface() const { return StackSurf; }
};

} // End llvm namespace

#endif

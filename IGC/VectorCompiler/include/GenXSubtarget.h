/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

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

#include "GenX.h"
#include "visa_igc_common_header.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvmWrapper/TargetParser/Triple.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Pass.h"

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
public:
  enum GenXTargetId {
    Gen8,
    Gen9,
    Gen9LP,
    Gen11,
    XeLP,
    XeHP,
    XeHPG,
    XeLPG,
    XeLPGPlus,
    XeHPC,
    XeHPCVG,
    Xe2,
    Xe3,
    Xe3P,
    Xe3PLPG,
    Invalid,
  };

protected:
  // TargetTriple - What processor and OS we're targeting.
  Triple TargetTriple;

  GenXTargetId TargetId;

private:
  // HasLongLong - True if subtarget supports long long type
  bool HasLongLong = false;

  // HasFP64 - True if subtarget supports double type
  bool HasFP64 = false;

  // HasNativeBFloat16 - True if subtarget supports bfloat16 arithmeics
  bool HasNativeBFloat16 = false;

  // HasMxfp - True if subtarget supports mxfp* operations
  bool HasMxfp = false;

  // HasIEEEDivSqrt - True if subtarget supports IEEE-754 div and sqrt
  bool HasIEEEDivSqrt = false;

  // FDivFSqrt64Emu - True if subtarget requires partial fp64 emulation
  bool FDivFSqrt64Emu = false;

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

  unsigned NumThreadsPerEU = 0;

  // True if legacy data-port messages are disabled
  bool TranslateLegacyMessages = false;

  // Currenly used for PVC B-stepping (some i64 operations are unsupported)
  bool PartialI64Emulation = false;

  // True if there is no legacy dataport shared function.
  bool NoLegacyDataport = false;

  // Some targets do not support i64 ops natively, we have an option to emulate
  bool EmulateLongLong = false;

  // True if target supports native 64-bit add
  bool HasAdd64 = false;

  // True if it is profitable to use native DxD->Q multiplication
  bool UseMulDDQ = false;

  // True if it is profitable to use native DxD+D->Q and DxD+Q->Q multiply-add
  // operations
  bool UseMadDDQ = false;

  // True if codegenerating for OCL runtime (set by default since CMRT removed)
  bool OCLRuntime = true;

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

  /// True if subtarget supports LSC messages
  bool HasLSCMessages = false;

  /// True if subtarget supports typed LSC messages
  bool HasLSCTypedMessages = false;

  /// True if subtarget supports constant offset for LSC message address
  bool HasLSCOffset = false;

  /// True if subtarget supports half SIMD LSC messages
  bool HasHalfSIMDLSC = false;

  /// True if subtarget supports efficient 64-bit addressing mode
  bool HasEfficient64b = false;

  /// True if efficient 64-bit mode is enabled
  bool EnabledEfficient64b = false;

  /// Number of supported cache levels
  unsigned NumCacheLevels = 2;

  /// True if subtarget supports sampler messages
  bool HasSampler = false;

  /// Has multi-tile.
  bool HasMultiTile = false;

  /// Has L3 cache-coherent cross tiles.
  bool HasL3CacheCoherentCrossTiles = false;

  /// Has L3 flush on GPU-scope invalidate.
  bool HasL3FlushOnGPUScopeInvalidate = false;

  /// Has denormal control for BF16 and TF32 types on DPAS
  bool HasSystolicDenormControl = false;

  /// True if Vx1 and VxH indirect addressing are allowed for Byte datatypes
  bool HasMultiIndirectByteRegioning = false;

  /// True if subtarget supports ADD3 instruction
  bool HasAdd3 = false;

  /// True if subtarget supports BFN instruction
  bool HasBfn = false;

  /// True if subtarget supports SAD and SADA2 instructions
  bool HasSad2 = false;

  /// True if subtarget supports OWord SLM read/write messages
  bool HasSLMOWord = false;

  /// True if subtarget supports SIMD32 MAD instruction
  bool HasMadSimd32 = false;

  /// True if subtarget requires A32 byte scatter emulation
  bool HasWaNoA32ByteScatter = false;

  /// True if subtarget supports indirect cross-grf access
  bool HasIndirectGRFCrossing = false;

  /// True if subtarget supports indirect cross-grf byte access
  bool HasIndirectByteGRFCrossing = false;

  /// True if subtarget supports named barriers
  bool HasNamedBarriers = false;

  /// True if subtarget supports media walker
  bool HasMediaWalker = false;

  /// True if subtarget supports large GRF mode
  bool HasLargeGRF = false;

  /// True if subtarget supports VRT
  bool HasVRT = false;

  // True if target supports local integer compare exchange 64-bit
  bool HasLocalIntegerCas64 = false;

  // True if target supports global double precision atomic add/sub
  bool HasGlobalAtomicAddF64 = false;

  // True if target supports half precision atomics
  bool HasInstrAtomicHF16 = false;

  // True if target supports local single precision atomic add/sub
  bool HasInstrLocalAtomicAddF32 = false;

  /// Max supported SLM size (in kbytes)
  int MaxSLMSize = 64;

  // Number of elements in Address Register
  unsigned AddressRegisterElements = 16;

  // True if subtarget supports SIMD32 programming model
  bool HasEfficientSIMD32 = false;

  // Shows which surface should we use for stack
  PreDefined_Surface StackSurf;

public:
  // This constructor initializes the data members to match that
  // of the specified triple.
  //
  GenXSubtarget(const Triple &TT, const std::string &CPU,
                const std::string &FS);

  GenXTargetId getTargetId() const { return TargetId; }

  // GRF size in bytes.
  unsigned getGRFByteSize() const { return GRFByteSize; }

  unsigned getNumThreadsPerEU() const { return NumThreadsPerEU; }

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

  unsigned getSamplerMinWidth() const { return GRFByteSize / genx::DWordBytes; }
  unsigned getSamplerMaxWidth() const { return 2 * getSamplerMinWidth(); }

  // ParseSubtargetFeatures - Parses features string setting specified
  // subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  // \brief Initialize the features for the GenX target.
  void initSubtargetFeatures(StringRef CPU, StringRef FS);

  bool isInternalIntrinsicSupported(unsigned ID) const;

public:
  /// * translateMediaWalker - true if translate media walker APIs
  bool translateMediaWalker() const { return !HasMediaWalker; }

  // TODO: consider implementing 2 different getters
  /// * has add3 and bfn instructions
  bool hasAdd3Bfn() const { return HasAdd3 && HasBfn; }

  int dpasWidth() const { return GRFByteSize / 4; }

  int getNumElementsInAddrReg() const { return GRFByteSize / 4; }

  bool hasLSCMessages() const { return HasLSCMessages; }

  bool hasLSCTypedMessages() const { return HasLSCTypedMessages; }

  bool hasLSCOffset() const { return HasLSCOffset; }

  // * efficient 64-bit addressing is supported
  bool supportEfficient64b() const { return HasEfficient64b; }

  // * efficient 64-bit addressing is supported and enabled
  bool hasEfficient64b() const {
    return HasEfficient64b && EnabledEfficient64b;
  }

  bool hasLSCBase() const { return hasEfficient64b(); }

  unsigned getLSCScaleMax() const { return hasEfficient64b() ? 32 : 1; }

  bool translateLegacyMessages() const {
    return HasLSCMessages && TranslateLegacyMessages;
  }

  bool translateMediaBlockMessages() const {
    return HasLSCTypedMessages && TranslateLegacyMessages;
  }

  bool hasHalfSIMDLSC() const { return HasHalfSIMDLSC; }

  bool partialI64Emulation() const { return PartialI64Emulation; }

  bool noLegacyDataport() const { return NoLegacyDataport; }

  bool hasSampler() const { return HasSampler; }

  /// * hasPackedFloat - true if packed float immediate vector operands are
  /// supported
  bool hasPackedFloat() const { return HasPackedFloat; }

  /// * emulateLongLong - true if i64 emulation is requested
  bool emulateLongLong() const { return EmulateLongLong; }

  /// * hasLongLong - true if target supports long long
  bool hasLongLong() const { return HasLongLong; }

  /// * hasFP64 - true if target supports double fp
  bool hasFP64() const { return HasFP64; }

  /// * hasNativeBFloat16 - true if target supports bfloat16 arithmetic
  bool hasNativeBFloat16() const { return HasNativeBFloat16; }

  /// * hasMxfp - true if target supports mxfp* operations
  bool hasMxfp() const { return HasMxfp; }

  /// * hasIEEEDivSqrt - true if target supports IEEE-754 div and sqrt
  bool hasIEEEDivSqrt() const { return HasIEEEDivSqrt; }

  /// * emulateFDivFSqrt64 - true if target requires partial fp64 emulation
  bool emulateFDivFSqrt64() const { return FDivFSqrt64Emu; }

  /// * hasAdd64 - true if target supports native 64-bit add/sub
  bool hasAdd64() const { return HasAdd64; }

  /// * useMulDDQ - true if is desired to emit DxD->Q mul instruction
  bool useMulDDQ() const { return UseMulDDQ; }

  /// * useMadDDQ - true if is desired to emit DxD+Q->Q and DxD+D->Q mad
  /// instruction
  bool useMadDDQ() const { return UseMadDDQ; }

  /// * disableJmpi - true if jmpi is disabled.
  bool disableJmpi() const { return DisableJmpi; }

  /// * WaNoA32ByteScatteredStatelessMessages - true if there is no A32 byte
  ///   scatter stateless message.
  bool WaNoA32ByteScatteredStatelessMessages() const {
    return HasWaNoA32ByteScatter;
  }

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
  bool hasIndirectGRFCrossing() const { return HasIndirectGRFCrossing; }

  /// * hasIndirectByteGRFCrossing - true if target supports an indirect region
  ///   crossing one GRF boundary with byte type
  bool hasIndirectByteGRFCrossing() const { return HasIndirectByteGRFCrossing; }

  /// * hasMultiIndirectByteRegioning - true if target supports an multi
  /// indirect regions with byte type
  bool hasMultiIndirectByteRegioning() const {
    return HasMultiIndirectByteRegioning;
  };

  bool hasNBarrier() const { return HasNamedBarriers; }

  /// * getMaxSlmSize - returns maximum allowed SLM size (in KB)
  unsigned getMaxSlmSize() const {
    return MaxSLMSize;
  }

  bool hasThreadPayloadInMemory() const { return HasThreadPayloadInMemory; }

  /// * hasSad2Support - returns true if sad2/sada2 are supported by target
  bool hasSad2Support() const { return HasSad2; }

  bool hasBitRotate() const { return HasBitRotate; }
  bool has64BitRotate() const { return Has64BitRotate; }

  bool hasLocalIntegerCas64() const { return HasLocalIntegerCas64; }

  bool hasGlobalAtomicAddF64() const { return HasGlobalAtomicAddF64; }

  bool hasInstrAtomicHF16() const { return HasInstrAtomicHF16; }
  bool hasInstrLocalAtomicAddF32() const { return HasInstrLocalAtomicAddF32; }

  bool hasL1ReadOnlyCache() const { return HasL1ReadOnlyCache; }
  bool hasLocalMemFenceSupress() const { return HasLocalMemFenceSupress; }
  bool hasMultiTile() const { return HasMultiTile; };
  bool hasL3CacheCoherentCrossTiles() const {
    return HasL3CacheCoherentCrossTiles;
  }
  bool hasL3FlushOnGPUScopeInvalidate() const {
    return HasL3FlushOnGPUScopeInvalidate;
  }

  bool hasSLMOWord() const { return HasSLMOWord; }

  bool hasMadSimd32() const { return HasMadSimd32; }

  bool hasLargeGRF() const { return HasLargeGRF; }

  bool hasVRT() const { return HasVRT; }

  /// * getsHWTIDFromPredef - some subtargets get HWTID from
  // predefined variable instead of sr0, returns *true* for such ones.
  bool getsHWTIDFromPredef() const { return GetsHWTIDFromPredef; }

  bool hasSystolicDenormControl() const { return HasSystolicDenormControl; }

  uint32_t getMaxThreadsNumPerSubDevice() const;
  ArrayRef<std::pair<int, int>> getThreadIdReservedBits() const;

  /// bit fields for SliceID and SubsliceID (from lsb to msb).
  ArrayRef<std::pair<int, int>> getSubsliceIdBits() const;

  /// bit fields for EU ID (from lsb to msb).
  ArrayRef<std::pair<int, int>> getEUIdBits() const;

  /// bit fields for ThreadID (from lsb to msb).
  ArrayRef<std::pair<int, int>> getThreadIdBits() const;

  unsigned getNumCacheLevels() const {
    if (hasEfficient64b())
      return NumCacheLevels;
    return 2;
  }

  // Address Register size in elements.
  unsigned getAddressRegisterElements() const {
    return AddressRegisterElements;
  }

  bool hasEfficientSIMD32() const { return HasEfficientSIMD32; }

  // Generic helper functions...
  const Triple &getTargetTriple() const { return TargetTriple; }

  TARGET_PLATFORM getVisaPlatform() const;

  /// * stackSurface - return a surface that should be used for stack.
  PreDefined_Surface stackSurface() const { return StackSurf; }

  bool isIntrinsicSupported(unsigned ID) const;

  ArrayRef<unsigned> getSupportedGRFSizes() const;
  bool isValidGRFSize(unsigned Size) const;
};

} // namespace llvm

#endif

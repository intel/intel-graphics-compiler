/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file contains defines for vISA and the vISA writer.
//
//===----------------------------------------------------------------------===//
#ifndef GENXVISA_H
#define GENXVISA_H

#include "GenX.h"
#include "GenXBaling.h"
#include "GenXModule.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"

#include <map>
#include <string>
#include <vector>

namespace llvm {
  namespace visa {

    // vISA relational operators
    enum { EQ, NE, GT, GE, LT, LE };


    enum {
      CLASS_GENERAL, CLASS_ADDRESS, CLASS_PREDICATE, CLASS_INDIRECT,
      CLASS_IMMEDIATE = 5, CLASS_STATE };

    // vISA vector operand modifiers
    enum { MOD_ABS = 0x8, MOD_NEG = 0x10, MOD_NEGABS = 0x18,
      MOD_SAT = 0x20, MOD_NOT = 0x28 };

    enum { VISA_NUM_RESERVED_REGS = 32,
           VISA_NUM_RESERVED_PREDICATES = 1,
           VISA_NUM_RESERVED_SURFACES = 6 };

    // These reserved indices are used by CM Frontend
    // and some passes (like TPM) to create an stateless/slm/stack accesses
    // TODO: consider introducing as set of new intrinsics with explicit
    // specification of access (to get rid of the relevant hacky code base).
    enum ReservedSurfaceIndex {
      RSI_Stack = 253, // 253 is for stack access (T1), used by TPM pass
      RSI_Slm = 254, // 254 is SLM, which is T0 in vISA
      RSI_Stateless = 255 // 255 is stateless, which is T5 in vISA
    };

    // In vISA spec max ARG size is 32 registers and RET is 12. But IGC calling
    // convention limits them to 12 and 8.
    constexpr static unsigned ArgRegSizeInGRFs = 12;
    constexpr static unsigned RetRegSizeInGRFs = 8;
    constexpr static alignment_t BytesPerSVMPtr = 8;
    constexpr static unsigned BytesPerOword = 16;
    constexpr static unsigned StackPerThreadScratch = 256;
    constexpr static unsigned StackPerThreadSVM = 8192*2;

    // Extracts surface Index (which is expected to be constant)
    // from llvm::Value
    // TODO: consider replacing dync_cast_or_null to dyn_cast
    // TODO: rename convert->extract
    inline int convertToSurfaceIndex(const Value* ValueIdx) {
      if (const auto CI = dyn_cast_or_null<ConstantInt>(ValueIdx)) {
        int InputValue = static_cast<int>(CI->getZExtValue());
        return InputValue;
      }
      return -1;
    }

    inline ReservedSurfaceIndex getReservedSurfaceIndex(PreDefined_Surface Surface) {
      switch(Surface) {
      case PreDefined_Surface::PREDEFINED_SURFACE_STACK:
        return RSI_Stack;
      case PreDefined_Surface::PREDEFINED_SURFACE_SLM:
        return RSI_Slm;
      case PreDefined_Surface::PREDEFINED_SURFACE_T255:
        return RSI_Stateless;
      default:
        // other types of prefefined surfaces are not used by CM backend
        break;
      }
      IGC_ASSERT_EXIT_MESSAGE(0, "unsupported predefined surface");
    }

    inline bool isReservedSurfaceIndex(int SurfaceIndex) {
      return SurfaceIndex == RSI_Stateless || SurfaceIndex == RSI_Slm ||
             SurfaceIndex == RSI_Stack;
    }

    inline PreDefined_Surface getReservedSurface(int SurfaceIndex) {
      IGC_ASSERT(isReservedSurfaceIndex(SurfaceIndex));
      switch(SurfaceIndex) {
      case RSI_Stack:
        return PreDefined_Surface::PREDEFINED_SURFACE_STACK;
      case RSI_Slm:
        return PreDefined_Surface::PREDEFINED_SURFACE_SLM;
      case RSI_Stateless:
        return PreDefined_Surface::PREDEFINED_SURFACE_T255;
      }
      IGC_ASSERT_EXIT_MESSAGE(0, "unexpected surface index");
    }

    enum { VISA_MAX_GENERAL_REGS = 65536 * 256 - 1,
           VISA_MAX_ADDRESS_REGS = 4096,
           VISA_MAX_PREDICATE_REGS = 4096,
           VISA_MAX_SAMPLER_REGS = 32 - 1,
           VISA_MAX_SURFACE_REGS = 256 };

    enum { VISA_WIDTH_GENERAL_REG = 32 };

    enum { VISA_ABI_INPUT_REGS_RESERVED = 1,
           VISA_ABI_INPUT_REGS_MAX = 128 };

    enum InputVarType {
      VISA_INPUT_GENERAL = 0x0,
      VISA_INPUT_SAMPLER = 0x1,
      VISA_INPUT_SURFACE = 0x2,
      VISA_INPUT_UNKNOWN
    };

namespace Variable {

namespace General {

constexpr int MinNumElements = 1;
constexpr int MaxNumElements = 4096;
constexpr int MaxSizeInBytes = 4096;

// Checks whether a general variable with the provided properties is legal
// according to vISA spec.
// Args:
//    ElemSize - size of variable element, must contain a legal value;
//    NumElems - number of variable elements, must be positive.
bool isLegal(int ElemSize, int NumElems);
// Checks whether a general variable with the provided type is legal
// according to vISA spec.
bool isLegal(IGCLLVM::FixedVectorType &Ty, const DataLayout &DL);

} // namespace General

namespace Predicate {

constexpr int MinNumElements = 1;
constexpr int MaxNumElements = 32;

// Checks whether a predicate variable with the provided properties are legal
// according to vISA spec.
// Args:
//    NumElems - number of variable elements, must be positive.
bool isLegal(int NumElems);
// Checks whether a predicate variable with the provided type is legal
// according to vISA spec.
// Only <N x i1> types are allowed.
bool isLegal(IGCLLVM::FixedVectorType &Ty);

} // namespace Predicate

bool isLegal(IGCLLVM::FixedVectorType &Ty, const DataLayout &DL);

} // namespace Variable

} // end namespace visa

} // end namespace llvm
#endif // ndef GENXVISA_H

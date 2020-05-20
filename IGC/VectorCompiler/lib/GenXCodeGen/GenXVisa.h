/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
// This file contains defines for vISA and the vISA writer.
//
//===----------------------------------------------------------------------===//
#ifndef GENXVISA_H
#define GENXVISA_H

#include "GenX.h"
#include "GenXBaling.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include <map>
#include <string>
#include <vector>
#include "GenXModule.h"

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
      llvm_unreachable("unsupported predefined surface");
    }

    inline bool isReservedSurfaceIndex(int SurfaceIndex) {
      return SurfaceIndex == RSI_Stateless || SurfaceIndex == RSI_Slm ||
             SurfaceIndex == RSI_Stack;
    }

    inline PreDefined_Surface getReservedSurface(int SurfaceIndex) {
      assert(isReservedSurfaceIndex(SurfaceIndex));
      switch(SurfaceIndex) {
      case RSI_Stack:
        return PreDefined_Surface::PREDEFINED_SURFACE_STACK;
      case RSI_Slm:
        return PreDefined_Surface::PREDEFINED_SURFACE_SLM;
      case RSI_Stateless:
        return PreDefined_Surface::PREDEFINED_SURFACE_T255;
      }
      llvm_unreachable("unexpected surface index");
    }

    enum { VISA_MAX_GENERAL_REGS = 65536 * 256 - 1,
           VISA_MAX_ADDRESS_REGS = 4096,
           VISA_MAX_PREDICATE_REGS = 4096,
           VISA_MAX_SAMPLER_REGS = 32 - 1,
           VISA_MAX_SURFACE_REGS = 256,
           VISA_MAX_VME_REGS = 16 };

    enum { VISA_WIDTH_GENERAL_REG = 32 };

    enum { VISA_ABI_INPUT_REGS_RESERVED = 1,
           VISA_ABI_INPUT_REGS_MAX = 128 };

    enum InputVarType {
      VISA_INPUT_GENERAL = 0x0,
      VISA_INPUT_SAMPLER = 0x1,
      VISA_INPUT_SURFACE = 0x2,
      VISA_INPUT_UNKNOWN
    };

  } // end namespace Visa

} // end namespace llvm
#endif // ndef GENXVISA_H

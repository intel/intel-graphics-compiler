/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file declares a class to access a table of extra information about the
// llvm.genx.* intrinsics, used by the vISA register allocator and function
// writer to decide exactly what operand type to use. The more usual approach
// in an LLVM target is to have an intrinsic map to an instruction in
// instruction selection, then have register category information on the
// instruction. But we are not using the target independent code generator, we
// are generating code directly from LLVM IR.
//
//===----------------------------------------------------------------------===//
#ifndef GENXINTRINSICS_H
#define GENXINTRINSICS_H
#include "GenX.h"
#include "GenXVisa.h"
#include "Probe/Assertion.h"
#include <unordered_map>

#define GENX_ITR_CATVAL(v) ((v) << CATBASE)
#define GENX_ITR_FLAGENUM(o, v) ((v) << ((o) + FLAGBASE))
#define GENX_ITR_FLAGMASK(o, w) (((1 << (w)) - 1) << ((o) + FLAGBASE))
#define GENX_ITR_FLAGVAL(o) GENX_ITR_FLAGENUM(o, 1)

namespace llvm {
class CallInst;

class GenXIntrinsicInfo {
public:
  enum {
    // General format of intrinsic descriptor words:
    //   Bits 31..24: Category enumeration
    //   Bits 23..8:  Flags, if any, meaning and layout depends on category
    //   Bits 7..0:   Operand or literal, if any
    //
    // One exception to the above is LITERAL, where everything that isn't
    // the category field is assumed to be the literal value.
    //
    // If you want to re-apportion space in the descriptor word (typically
    // because you need another flag and you can't express what you need to
    // do without creating one) then just modify FLAGBASE and FLAGWIDTH
    // below, and everything else will shake itself out appropriately.
    // Currently 8 bits are allocated for the category enumaration bitfield,
    // although the actual enumeration values defined only require 6 bits -
    // and there is still plenty of space left over even within that.
    // Similarly, there are 8 bits allocated to the operand bitfield, and
    // currently the maximum needed is 5.
    //
    // At the moment, the GENERAL category has 4 unused flag bits available
    // to it, the RAW category has 13 unused bits, and the ARGCOUNT category
    // has 13 unused bits. No other categories make use of the flags yet,
    // so it should be a good while yet before it's necessary to resize
    // the bitfields.

    FLAGBASE = 8,
    FLAGWIDTH = 16,
    CATBASE = FLAGBASE + FLAGWIDTH,

    CATMASK = ~((1 << CATBASE) - 1),
    FLAGMASK = ((~((1 << FLAGBASE) - 1)) ^ CATMASK),
    OPNDMASK = ~(CATMASK | FLAGMASK),

    // A field that does not contain an operand number or literal value:
    IMPLICITPRED =          GENX_ITR_CATVAL(0x01), // implicit predication field
    NULLRAW =               GENX_ITR_CATVAL(0x02), // null raw operand
    ISBARRIER =             GENX_ITR_CATVAL(0x03), // intrinsic is barrier: suppress nobarrier attribute

    EXECSIZE =              GENX_ITR_CATVAL(0x04), // execution size
    EXECSIZE_GE2 =          GENX_ITR_CATVAL(0x05), // execution size (must be >= 2)
    EXECSIZE_GE4 =          GENX_ITR_CATVAL(0x06), // execution size (must be >= 4)
    EXECSIZE_GE8 =          GENX_ITR_CATVAL(0x07), // execution size (must be >= 8)
    EXECSIZE_NOT2 =         GENX_ITR_CATVAL(0x08), // execution size (cannot be 2)

    // A field that contains a literal value the operand field
    LITERAL =               GENX_ITR_CATVAL(0x09), // literal byte (usually opcode)
    LITMASK =               ~CATMASK,

    // A field that contains an operand number, other than general:
    FIRST_OPERAND =         GENX_ITR_CATVAL(0x10),
    LOG2OWORDS =            GENX_ITR_CATVAL(0x10), // log2 number of owords
    NUMGRFS =               GENX_ITR_CATVAL(0x11), // rounded up number of GRFs
    EXECSIZE_FROM_ARG =     GENX_ITR_CATVAL(0x12), // exec_size field inferred from width of
                                                   // predication arg
    SVMGATHERBLOCKSIZE =    GENX_ITR_CATVAL(0x13), // svm gather block size, inferred from data type
    LOG2OWORDS_PLUS_8 =     GENX_ITR_CATVAL(0x14), // log2 number of owords, plus 8
    GATHERNUMELTS =         GENX_ITR_CATVAL(0x15), // gather/scatter "num elements" field
    TRANSPOSEHEIGHT =       GENX_ITR_CATVAL(0x16), // block_height field in transpose
    LOG2ELTSIZE =           GENX_ITR_CATVAL(0x17), // log2 element size in gather/scatter
    ARGCOUNT =              GENX_ITR_CATVAL(0x18), // Byte containing number of non-undef operands
    EXECSIZE_FROM_BYTE =    GENX_ITR_CATVAL(0x19), // exec_size specified in byte
      ARGCOUNTMASK =        GENX_ITR_FLAGMASK(0, 3), // Space for minumum argument count
      ARGCOUNTMIN1 =        GENX_ITR_FLAGENUM(0, 1), // Must have at least one argument
    BITWIDTH =              GENX_ITR_CATVAL(0x1a), // bit width of svm atomic instructions

    // A field that contains an operand number, other than general, and it
    // is the "real" use of the operand, rather than an auxiliary use
    // such as a "number of GRFs" field relating to this operand.
    FIRST_REAL_OPERAND =    GENX_ITR_CATVAL(0x20),
    BYTE =                  GENX_ITR_CATVAL(0x20), // constant byte operand
    SHORT =                 GENX_ITR_CATVAL(0x21), // constant short operand
    INT =                   GENX_ITR_CATVAL(0x22), // constant int operand
    ADDRESS =               GENX_ITR_CATVAL(0x23), // address operand
    PREDICATE =             GENX_ITR_CATVAL(0x24), // predicate operand
      PREDICATE_ZEROED =    GENX_ITR_FLAGVAL(0),
    Z_PREDICATE = PREDICATE | PREDICATE_ZEROED,
    SAMPLER =               GENX_ITR_CATVAL(0x25), // sampler operand
    SURFACE =               GENX_ITR_CATVAL(0x26), // surface operand
    // byte height of media 2D block, inferred from the width operand
    // pointed at and the size of the return type or final operand type
    MEDIAHEIGHT =           GENX_ITR_CATVAL(0x27),
    // predication control field from explicit predicate arg
    PREDICATION =           GENX_ITR_CATVAL(0x28),
    // chmask field in load/sample, with exec size bit
    SAMPLECHMASK =          GENX_ITR_CATVAL(0x29),
    // does not appear in the vISA output, but needs to be two address
    // coalesced with result
    TWOADDR =               GENX_ITR_CATVAL(0x2a),
    CONSTVI1ASI32 =         GENX_ITR_CATVAL(0x2b), // constant vXi1 written as i32 (used in setp)
    RAW =                   GENX_ITR_CATVAL(0x2c), // raw operand or result,
      // Raw descriptor flags, 3 bits used
      RAW_UNSIGNED =        GENX_ITR_FLAGVAL(0),   // raw operand/result must be unsigned
      RAW_SIGNED =          GENX_ITR_FLAGVAL(1),   // raw operand/result must be signed
      RAW_NULLALLOWED =     GENX_ITR_FLAGVAL(2),   // raw operand or result can be null (V0)
    URAW =                  RAW | RAW_UNSIGNED,
    SRAW =                  RAW | RAW_SIGNED,
    EXECSIZE_NOMASK =       GENX_ITR_CATVAL(0x2d), // execution size with NoMask

    // A general operand
    GENERAL =               GENX_ITR_CATVAL(0x2e),
    // A general operand with compile-time signedness choosing
    GENERAL_CTSIGN =        GENERAL,
    // A predefined surface operand
    PREDEF_SURFACE =        GENX_ITR_CATVAL(0x2f),
    // Modifiers for destination or source, 7 bits used
    UNSIGNED =              GENX_ITR_FLAGVAL(0), // int type forced to unsigned
    SIGNED =                GENX_ITR_FLAGVAL(1), // int type forced to signed
    OWALIGNED =             GENX_ITR_FLAGVAL(2), // must be oword aligned
    GRFALIGNED =            GENX_ITR_FLAGVAL(3), // must be grf aligned
    RESTRICTION =           GENX_ITR_FLAGMASK(4, 3), // field with operand width restriction
      FIXED4 =              GENX_ITR_FLAGENUM(4, 1), // operand is fixed size 4 vector and contiguous
      CONTIGUOUS =          GENX_ITR_FLAGENUM(4, 2), // operand must be contiguous
      SCALARORCONTIGUOUS =  GENX_ITR_FLAGENUM(4, 3), // operand must be stride 0 or contiguous
      TWICEWIDTH =          GENX_ITR_FLAGENUM(4, 4), // operand is twice the execution width
      STRIDE1 =             GENX_ITR_FLAGENUM(4, 5), // horizontal stride must be 1
    // Modifiers for destination only, 2 bits used
    SATURATION =            GENX_ITR_FLAGMASK(7, 2),
    SATURATION_DEFAULT =    GENX_ITR_FLAGENUM(7, 0), // saturation default: not saturated, fp is
                                                     //  allowed to bale in to saturate inst
    SATURATION_SATURATE =   GENX_ITR_FLAGENUM(7, 1), // saturated
    SATURATION_NOSAT =      GENX_ITR_FLAGENUM(7, 2), // fp not allowed to bale in to saturate inst
    SATURATION_INTALLOWED = GENX_ITR_FLAGENUM(7, 3), // int is allowed to bale in to saturate,
                                   // because inst cannot overflow so
                                   // saturation only required on destination
                                   // truncation
    // Modifiers for source only, 3 bits used
    NOIMM =                 GENX_ITR_FLAGVAL(7), // source not allowed to be immediate
    MODIFIER =              GENX_ITR_FLAGMASK(8, 2),
    MODIFIER_DEFAULT =      GENX_ITR_FLAGENUM(8, 0), // src modifier default: none
    MODIFIER_ARITH =        GENX_ITR_FLAGENUM(8, 1), // src modifier: arithmetic
    MODIFIER_LOGIC =        GENX_ITR_FLAGENUM(8, 2), // src modifier: logic
    MODIFIER_EXTONLY =      GENX_ITR_FLAGENUM(8, 3), // src modifier: extend only
    DIRECTONLY =            GENX_ITR_FLAGVAL(10), // indirect region not allowed
  };
  struct ArgInfo {
    unsigned Info;
    // Construct argument with empty info.
    ArgInfo() : Info(0) {}
    // Construct from a field read from the intrinsics info table.
    ArgInfo(unsigned Info) : Info(Info) {}
    // getCategory : return field category
    unsigned getCategory() const { return Info & CATMASK; }
    // getAlignment : get any special alignment requirement, else align to 1
    // byte
    VISA_Align getAlignment() const {
      if (isGeneral()) {
        if (Info & GRFALIGNED)
          return VISA_Align::ALIGN_GRF;
        if (Info & OWALIGNED)
          return VISA_Align::ALIGN_OWORD;
        return VISA_Align::ALIGN_BYTE;
      }
      if (isRaw())
        return VISA_Align::ALIGN_GRF;
      return VISA_Align::ALIGN_BYTE;
    }
    // isGeneral : test whether this is a general operand
    bool isGeneral() const { return getCategory() == GENERAL; }
    bool needsSigned() const {
      if (isGeneral())
        return Info & SIGNED;
      if (isRaw())
        return Info & RAW_SIGNED;
      return false;
    }
    bool needsUnsigned() const {
      if (isGeneral())
        return Info & UNSIGNED;
      if (isRaw())
        return Info & RAW_UNSIGNED;
      return false;
    }
    bool isDirectOnly() const { return Info & DIRECTONLY; }
    bool rawNullAllowed() const {
      IGC_ASSERT(isRaw());
      return Info & RAW_NULLALLOWED;
    }
    // isArgOrRet : test whether this field has an arg index
    bool isArgOrRet() const {
      if (isGeneral()) return true;
      if ((Info & CATMASK) >= FIRST_OPERAND)
        return true;
      return false;
    }
    // isRealArgOrRet : test whether this field has an arg index, and is
    // a "real" use of the arg
    bool isRealArgOrRet() const {
      if (isGeneral()) return true;
      if ((Info & CATMASK) >= FIRST_REAL_OPERAND)
        return true;
      return false;
    }
    // getArgCountMin : return minimum number of arguments
    int getArgCountMin() const {
      IGC_ASSERT(getCategory() == ARGCOUNT);
      return (Info & ARGCOUNTMASK) >> FLAGBASE;
    }
    // getArgIdx : return argument index for this field, or -1 for return value
    //  (assuming isArgOrRet())
    int getArgIdx() const {
      IGC_ASSERT(isArgOrRet());
      return (Info & OPNDMASK) - 1;
    }
    // getLiteral : for a LITERAL or EXECSIZE field, return the literal value
    unsigned getLiteral() const { return Info & LITMASK; }
    // isRet : test whether this is the field for the return value
    //  (assuming isArgOrRet())
    bool isRet() const { return getArgIdx() < 0; }
    // isRaw : test whether this is a raw arg or return value
    bool isRaw() const { return getCategory() == RAW; }
    // getSaturation : return saturation info for the arg
    unsigned getSaturation() const { return Info & SATURATION; }
    // getRestriction : return operand width/region restriction, one of
    // 0 (no restriction), FIXED4, CONTIGUOUS, TWICEWIDTH
    unsigned getRestriction() const { return Info & RESTRICTION; }
    // isImmediateDisallowed : test whether immediate disallowed
    //  (assuming isArgOrRet())
    bool isImmediateDisallowed() const {
      IGC_ASSERT(isArgOrRet());
      if (isGeneral())
        return Info & NOIMM;
      if (isRaw())
        return true;
      switch (Info & CATMASK) {
        case TWOADDR:
        case PREDICATION:
        case SURFACE:
        case SAMPLER:
          return true;
        default: break;
      }
      return false;
    }
    // getModifier : get what source modifier is allowed
    unsigned getModifier() const {
      IGC_ASSERT(isGeneral());
      IGC_ASSERT(isArgOrRet());
      IGC_ASSERT(!isRet());
      return Info & MODIFIER;
    }
  };

  using DescrType = llvm::SmallVector<ArgInfo, 8>;

  // Construct a GenXIntrinsicInfo for a particular intrinsic
  GenXIntrinsicInfo(unsigned IntrinId) { InfoIt = Table.find(IntrinId); }
  bool isNull() const { return InfoIt == Table.cend(); }
  bool isNotNull() const { return !isNull(); }
  // Return instruction description.
  ArrayRef<ArgInfo> getInstDesc() const {
    return isNull() ? ArrayRef<ArgInfo>{} : ArrayRef<ArgInfo>{InfoIt->second};
  }
  // Get the category and modifier for an arg idx
  ArgInfo getArgInfo(int Idx);
  // Get the trailing null zone, if any.
  unsigned getTrailingNullZoneStart(CallInst *CI);
  // Get the category and modifier for the return value
  ArgInfo getRetInfo() { return getArgInfo(-1); }
  // Get bitmap of allowed execution sizes
  unsigned getExecSizeAllowedBits();
  // Determine if a predicated destination mask is permitted
  bool getPredAllowed();
  // Get The overrided execution size or 0.
  static unsigned getOverridedExecSize(CallInst *CI,
                                       const GenXSubtarget *ST = nullptr);

private:
  using TableType = std::unordered_map<unsigned, DescrType>;
  static const TableType Table;
  TableType::const_iterator InfoIt;
};

} // namespace llvm
#endif // ndef GENXINTRINSICS_H

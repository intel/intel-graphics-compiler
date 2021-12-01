/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//==== G4Instruction.h - info about G4 instructions ====//
//
// This file contains descriptions of all G4 instructions (real or pseudo).
// It is the single place to enumerate description for each instruction.
// Adding, subtracting, commenting instructions shall happen in this file.

// HANDLE_INST: describe each instruction
// HANDLE_NAME_INST: a variant of HANDLE_INST, using given instruction name
//                   instead of opcode as name.

//
// 1. none of those two macros are defined, this file should be empty
// 2. If both macros are the same, users of this file need to define
//    HANDLE_INST only, no need to define both.
//
#ifndef HANDLE_INST
#define HANDLE_INST(op, nsrc, ndst, type, plat, attr)
#endif
#ifndef HANDLE_NAME_INST
#define HANDLE_NAME_INST(op, name, nsrc, ndst, type, plat, attr) \
            HANDLE_INST(op, nsrc, ndst, type, plat, attr)
#endif

//
// First one is illegal instruction
//
HANDLE_INST(illegal, 0, 0, InstTypeMisc, GENX_BDW, ATTR_NONE)

//
// InstTypeMov
//
HANDLE_INST(mov,   1, 1, InstTypeMov, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(sel,   2, 1, InstTypeMov, GENX_BDW, ATTR_NONE)
HANDLE_INST(movi,  1, 1, InstTypeMov, GENX_BDW, ATTR_NONE)
HANDLE_INST(smov,  2, 1, InstTypeMov, GENX_BDW, ATTR_NONE)
HANDLE_INST(csel,  3, 1, InstTypeMov, GENX_SKL, ATTR_FLOAT_SRC_ONLY)
  //
  // fcvt: special pseudo instruction for converting b/w a standard float type
  //       and bf8/tf32, which are denoted by Type_UB/Type_UD, respectively.
  //
HANDLE_INST(fcvt,  1, 1, InstTypeMov, Xe_PVCXT, ATTR_PSEUDO)

//
// InstTypeLogic
//
HANDLE_INST(not,   1, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(and,   2, 1, InstTypeLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(or,    2, 1, InstTypeLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(xor,   2, 1, InstTypeLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(bfrev, 1, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(bfe,   3, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(bfi1,  2, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(bfi2,  3, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(fbh,   1, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(fbl,   1, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(cbit,  1, 1, InstTypeLogic, GENX_BDW, ATTR_NONE)
HANDLE_INST(bfn,  3, 1, InstTypeLogic, Xe_XeHPSDV, ATTR_NONE)

//
// InstTypeArith
//
HANDLE_INST(shr, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(shl, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(asr, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)

HANDLE_INST(ror, 2, 1, InstTypeArith, GENX_ICLLP, ATTR_NONE)
HANDLE_INST(rol, 2, 1, InstTypeArith, GENX_ICLLP, ATTR_NONE)

HANDLE_INST(math, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(add,  2, 1, InstTypeArith, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(mul,  2, 1, InstTypeArith, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(avg,  2, 1, InstTypeArith, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(frc,  1, 1, InstTypeArith, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(rndu, 1, 1, InstTypeArith, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(rndd, 1, 1, InstTypeArith, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(rnde, 1, 1, InstTypeArith, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(rndz, 1, 1, InstTypeArith, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(mac,  2, 1, InstTypeArith, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(mach, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(lzd,  1, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(addc, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
HANDLE_INST(subb, 2, 1, InstTypeArith, GENX_BDW, ATTR_NONE)

HANDLE_INST(dp4a, 3, 1, InstTypeArith, GENX_TGLLP, ATTR_NONE)
HANDLE_INST(dpas,  3, 1, InstTypeArith, Xe_XeHPSDV, ATTR_NONE)
HANDLE_INST(dpasw, 3, 1, InstTypeArith, Xe_XeHPSDV, ATTR_NONE)
HANDLE_INST(add3,  3, 1, InstTypeArith, Xe_XeHPSDV, ATTR_COMMUTATIVE)


HANDLE_INST(madm,   3, 1, InstTypeArith, GENX_BDW, ATTR_NONE)
  //
  // Following are pseudo instructions
  //
HANDLE_INST(mulh,   2, 1, InstTypeArith, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(srnd,   2, 1, InstTypeArith, Xe_PVCXT, ATTR_PSEUDO)
HANDLE_INST(madw,   3, 1, InstTypeArith, GENX_BDW, ATTR_WIDE_DST)

//
// InstTypeCompare
//
HANDLE_INST(cmp,    2, 1, InstTypeCompare, GENX_BDW, ATTR_NONE)
HANDLE_INST(cmpn,   2, 1, InstTypeCompare, GENX_BDW, ATTR_FLOAT_SRC_ONLY)

//
// InstTypeFlow
//
HANDLE_INST(jmpi,   1, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(brd,    1, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(if,     0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(brc,    2, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(else,   0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(endif,  0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(while,  0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(break,  0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(cont,   0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(halt,   0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(call,   1, 1, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(return, 1, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(goto,   0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(join,   0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
  //
  // Following are pseudo instructions
  //
HANDLE_INST(do,           0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(pseudo_fcall, 1, 1, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(pseudo_fret,  1, 1, InstTypeFlow, GENX_BDW, ATTR_NONE)
HANDLE_INST(pseudo_exit,  0, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)
  // pseudo_fc_call are generated for calls to callable kernels.
  // Destination of such calls is not available at compile time.
  // Just before binary encoding these instructions are converted to call
  // and their dst operand is left for runtime to patch. An .fcpatch file
  // is emitted with VISA instruction number that needs patching.
HANDLE_INST(pseudo_fc_call, 1, 1, InstTypeFlow, GENX_BDW, ATTR_NONE)
  // pseudo_fc_ret are generated for return statements from callable kernels.
  // This has to be done because for kernels, we convert VISA ret instruction
  // to EOT. But for callable kernels, we dont want to emit EOT because they
  // may have to return to a top-level kernel. Only top-level kernel will
  // have VISA ret lowered to EOT.
HANDLE_INST(pseudo_fc_ret, 1, 0, InstTypeFlow, GENX_BDW, ATTR_NONE)

//
// InstTypeVector
//
HANDLE_INST(sad2,  2, 1,  InstTypeVector, GENX_BDW, ATTR_NONE)
HANDLE_INST(sada2, 2, 1,  InstTypeVector, GENX_BDW, ATTR_NONE)
HANDLE_INST(dp4,   2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(dph,   2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(dp3,   2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(dp2,   2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(line,  2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(pln,   2, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
HANDLE_INST(mad,   3, 1,  InstTypeVector, GENX_BDW, ATTR_NONE)
HANDLE_INST(lrp,   3, 1,  InstTypeVector, GENX_BDW, ATTR_FLOAT_SRC_ONLY)
  //
  // Following are pseudo instructions
  //
HANDLE_INST(pseudo_mad,   3, 1,  InstTypeVector, GENX_BDW, ATTR_NONE)
HANDLE_INST(pseudo_sada2, 3, 1,  InstTypeVector, GENX_BDW, ATTR_NONE)

//
// InstTypeMisc
//
HANDLE_INST(wait,   1, 0, InstTypeMisc, GENX_BDW, ATTR_NONE)
HANDLE_INST(send,   2, 1, InstTypeMisc, GENX_BDW, ATTR_NONE)
HANDLE_INST(sendc,  2, 1, InstTypeMisc, GENX_BDW, ATTR_NONE)
HANDLE_INST(sends,  4, 1, InstTypeMisc, GENX_SKL, ATTR_NONE)
HANDLE_INST(sendsc, 4, 1, InstTypeMisc, GENX_SKL, ATTR_NONE)
HANDLE_INST(nop,    0, 0, InstTypeMisc, GENX_BDW, ATTR_NONE)

HANDLE_INST(sync_nop,   1, 0, InstTypeMisc, GENX_TGLLP, ATTR_NONE)
HANDLE_INST(sync_allrd, 1, 0, InstTypeMisc, GENX_TGLLP, ATTR_NONE)
HANDLE_INST(sync_allwr, 1, 0, InstTypeMisc, GENX_TGLLP, ATTR_NONE)
HANDLE_INST(sync_fence, 1, 0, InstTypeMisc, Xe_PVC, ATTR_NONE)

  //
  // Following are pseudo instructions
  //
HANDLE_NAME_INST(label, "", 0, 0, InstTypeMisc, GENX_BDW, ATTR_NONE)
HANDLE_INST(intrinsic, 1, 3, InstTypeMisc, GENX_BDW, ATTR_NONE)

//
// InstTypePseudoLogic
//
HANDLE_INST(pseudo_and, 2, 1, InstTypePseudoLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(pseudo_or,  2, 1, InstTypePseudoLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(pseudo_xor, 2, 1, InstTypePseudoLogic, GENX_BDW, ATTR_COMMUTATIVE)
HANDLE_INST(pseudo_not, 1, 1, InstTypePseudoLogic, GENX_BDW, ATTR_NONE)

#undef HANDLE_INST
#undef HANDLE_NAME_INST

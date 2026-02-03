/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <fstream>
#include <iostream>
#include <list>
#include <math.h>
#include <sstream>
#include <string>

#include "Assertions.h"
#include "BuildIR.h"
#include "Common_ISA_framework.h"
#include "Common_ISA_util.h"
#include "JitterDataStruct.h"
#include "Timer.h"
#include "common.h"
#include "visa_igc_common_header.h"
#include <optional>


using namespace vISA;

DeclarePool::~DeclarePool() {
  for (unsigned i = 0, size = (unsigned)dcllist.size(); i < size; i++) {
    G4_Declare *dcl = dcllist[i];
    dcl->~G4_Declare();
  }
  dcllist.clear();
}

G4_Declare *
DeclarePool::cloneDeclare(G4_Kernel &kernel,
                          std::map<G4_Declare *, G4_Declare *> &dclMap,
                          const char *newDclName, G4_Declare *dcl) {
  if (dclMap.find(dcl) != dclMap.end()) {
    return dclMap[dcl];
  }
  G4_Declare *topDcl = dcl->getAliasDeclare();
  G4_Declare *cloneTopDcl;
  G4_Declare *cloneDcl =
      new (mem) G4_Declare(irb, newDclName, dcl->getRegFile(),
                           dcl->getTotalElems(), dcl->getElemType(), dcllist);
  cloneDcl->setName(newDclName);
  G4_RegVar *regVar =
      new (mem) G4_RegVar(cloneDcl, G4_RegVar::RegVarType::Default);
  cloneDcl->setRegVar(regVar);
  cloneDcl->setSubRegAlign(dcl->getSubRegAlign());
  if (dcl->getRegFile() == G4_FLAG) {
    cloneDcl->setNumberFlagElements((uint8_t)dcl->getNumberFlagElements());
  }
  if (topDcl) {
    vASSERT(dcl != topDcl);
    cloneTopDcl = (dclMap.find(topDcl) == dclMap.end())
                      ? cloneDeclare(kernel, dclMap, dcl->getName(), topDcl)
                      : dclMap[topDcl];
    cloneDcl->setAliasDeclare(cloneTopDcl, dcl->getAliasOffset());
  }
  kernel.Declares.push_back(cloneDcl);
  dclMap[dcl] = cloneDcl;
  return cloneDcl;
}

G4_Declare *DeclarePool::createDeclare(const char *name, G4_RegFileKind regFile,
                                       unsigned short nElems,
                                       unsigned short nRows, G4_Type ty,
                                       DeclareType kind, G4_RegVar *base,
                                       G4_Operand *repRegion,
                                       G4_ExecSize execSize) {
  G4_Declare *dcl =
      new (mem) G4_Declare(irb, name, regFile, nElems * nRows, ty, dcllist);
  G4_RegVar *regVar;
  if (kind == DeclareType::Regular)
    regVar = new (mem) G4_RegVar(dcl, G4_RegVar::RegVarType::Default);
  else if (kind == DeclareType::AddrSpill)
    regVar = new (mem) G4_RegVarAddrSpillLoc(dcl, addrSpillLocCount,
                                             irb.getNumAddrRegisters());
  else if (kind == DeclareType::Tmp)
    regVar = new (mem) G4_RegVarTmp(dcl, base);
  else if (kind == DeclareType::Spill)
    regVar = new (mem)
        G4_RegVarTransient(dcl, base, repRegion->asDstRegRegion(), execSize,
                           G4_RegVarTransient::TransientType::Spill);
  else if (kind == DeclareType::Fill)
    regVar = new (mem)
        G4_RegVarTransient(dcl, base, repRegion->asSrcRegRegion(), execSize,
                           G4_RegVarTransient::TransientType::Fill);
  else if (kind == DeclareType::CoalescedSpillFill)
    regVar = new (mem) G4_RegVar(dcl, G4_RegVar::RegVarType::Coalesced);
  else {
    vISA_ASSERT(false, ERROR_INTERNAL_ARGUMENT);
    regVar = NULL;
  }
  dcl->setRegVar(regVar);

  if (regFile == G4_ADDRESS || regFile == G4_SCALAR) {
    dcl->setSubRegAlign(Any);
  } else if (regFile != G4_FLAG) {
    if ((unsigned int)nElems * nRows * TypeSize(ty) >=
        irb.numEltPerGRF<Type_UB>()) {
      dcl->setSubRegAlign(irb.getGRFAlign());
    } else {
      // at a minimum subRegAlign has to be at least the type size
      dcl->setSubRegAlign(Get_G4_SubRegAlign_From_Type(ty));
    }
  } else {
    if (dcl->getNumberFlagElements() == 32) {
      dcl->setSubRegAlign(Even_Word);
    }
  }

  return dcl;
}

G4_Declare *IR_Builder::GlobalImmPool::addImmVal(G4_Imm *imm, int numElt) {
  ImmVal val = {imm, numElt};
  for (int i = 0; i < curSize; ++i) {
    if (val == immArray[i]) {
      return dclArray[i];
    }
  }
  if (curSize == MAX_POOL_SIZE) {
    return nullptr;
  }
  immArray[curSize] = val;
  dclArray[curSize] = builder.createTempVar(numElt, imm->getType(), Any);
  return dclArray[curSize++];
}

///////////////////////////////////////////////////////////////////////////////
// IR_Builder functions (except translateXXXX, which should be in VisaToG4)
//

void IR_Builder::dump(std::ostream &os) {
  os << "DECLARES:\n";
  for (const G4_Declare *dcl : kernel.Declares) {
    dcl->emit(os);
    os << "\n";
  }
  os << "\n";
  os << "INSTS:\n";
  for (G4_INST *i : instList) {
    i->emit(os);
    os << "\n";
  }
}

// bind a vISA input variable <dcl> to the GRF byte offset <offset>
void IR_Builder::bindInputDecl(
    G4_Declare *dcl,
    int offset) { // decide the physical register number and sub register number
  unsigned int regNum = offset / getGRFSize();
  unsigned int subRegNum = (offset % getGRFSize()) / dcl->getElemSize();
  dcl->getRegVar()->setPhyReg(phyregpool.getGreg(regNum), subRegNum);
  dcl->setRegFile(G4_INPUT);
  unsigned int reservedGRFNum = m_options->getuInt32Option(vISA_ReservedGRFNum);
  unsigned int totalGRFNum = kernel.grfMode.getInitalGRFNum();
  if (regNum + dcl->getNumRows() > totalGRFNum - reservedGRFNum) {
    vISA_ASSERT(false, "INPUT payload execeeds the register number");
  }
}

bool IR_Builder::isGRFDstAligned(G4_Operand* opnd, int alignByte) const
{
  bool isAligned = true;
  unsigned short offset = 0;
  int type_size = opnd->getTypeSize();
  G4_Declare *dcl = NULL;

  dcl = opnd->getBase()->asRegVar()->getDeclare();
  while (dcl && dcl->getAliasDeclare()) {
    // If the variable has sub register alignment requirement.
    // The sub alignment is in the word unit.
    if (dcl->getSubRegAlign() != Any &&
        (((dcl->getSubRegAlign() * 2) >= alignByte &&
          (dcl->getSubRegAlign() * 2) % alignByte != 0) ||
         ((dcl->getSubRegAlign() * 2) < alignByte &&
          alignByte % (dcl->getSubRegAlign() * 2) != 0))) {
      isAligned = false;
      break;
    }
    offset += (unsigned short)dcl->getAliasOffset();
    dcl = dcl->getAliasDeclare();
  }

  if (dcl && dcl->getRegVar() && dcl->getRegVar()->isPhyRegAssigned()) {
    // For preRA ACC sub, if the variable is assigned with register already,
    // don't put it in the candidate of ACC.
    return false;
  }

  if (!isAligned) {
    return false;
  }

  if (opnd->isDstRegRegion()) {
    if (opnd->asDstRegRegion()->getRegAccess() != Direct) {
      isAligned = false;
    }
    offset += opnd->asDstRegRegion()->getRegOff() * numEltPerGRF<Type_UB>() +
              opnd->asDstRegRegion()->getSubRegOff() * type_size;
  }

  if (offset % alignByte != 0) {
    return false;
  }

  if (dcl && dcl->getRegFile() == G4_GRF) {
    if (dcl->getSubRegAlign() == Any ||
        ((dcl->getSubRegAlign() * 2) < alignByte &&
         alignByte % (dcl->getSubRegAlign() * 2) == 0)) {
      isAligned = false;
    } else if ((dcl->getSubRegAlign() * 2) < alignByte ||
               (dcl->getSubRegAlign() * 2) % alignByte != 0) {
      isAligned = false;
    }
  } else {
    isAligned = false;
  }

  return isAligned;
}

// check if an operand is aligned to <alignByte>
bool IR_Builder::tryToAlignOperand(G4_Operand *opnd, unsigned short &offset,
                                   int alignByte) const {
  offset = 0;
  bool isAligned = true;

  switch (opnd->getKind()) {
  case G4_Operand::immediate:
  case G4_Operand::addrExp:
  case G4_Operand::label:
  case G4_Operand::condMod:
  case G4_Operand::predicate: {
    isAligned = true;
    break;
  }
  case G4_Operand::srcRegRegion:
  case G4_Operand::dstRegRegion: {
    int type_size = opnd->getTypeSize();
    G4_Declare *dcl = NULL;
    if (opnd->getBase()->isRegVar()) {
      dcl = opnd->getBase()->asRegVar()->getDeclare();
      while (dcl && dcl->getAliasDeclare()) {
        if (dcl->getSubRegAlign() != Any &&
            (((dcl->getSubRegAlign() * 2) >= alignByte &&
              (dcl->getSubRegAlign() * 2) % alignByte != 0) ||
             ((dcl->getSubRegAlign() * 2) < alignByte &&
              alignByte % (dcl->getSubRegAlign() * 2) != 0))) {
          isAligned = false;
          break;
        }
        offset += (unsigned short)dcl->getAliasOffset();
        dcl = dcl->getAliasDeclare();
      }

      if (dcl && dcl->getRegVar() && dcl->getRegVar()->isPhyRegAssigned()) {
        offset +=
            static_cast<unsigned short>(dcl->getRegVar()->getByteAddr(*this));
      }
    }
    if (!isAligned) {
      return isAligned;
    }

    if (opnd->isDstRegRegion()) {
      if (opnd->asDstRegRegion()->getRegAccess() != Direct) {
        isAligned = false;
      }
      offset += opnd->asDstRegRegion()->getRegOff() * numEltPerGRF<Type_UB>() +
                opnd->asDstRegRegion()->getSubRegOff() * type_size;
    } else if (opnd->isSrcRegRegion()) {
      if (opnd->asSrcRegRegion()->getRegAccess() != Direct) {
        isAligned = false;
      }
      offset += opnd->asSrcRegRegion()->getRegOff() * numEltPerGRF<Type_UB>() +
                opnd->asSrcRegRegion()->getSubRegOff() * type_size;
    }
    if (offset % alignByte != 0) {
      return false;
    }
    // Only alignment of the top dcl can be changed.
    if (dcl && dcl->getRegFile() == G4_GRF) {
      if (dcl->getSubRegAlign() == Any ||
          ((dcl->getSubRegAlign() * 2) < alignByte &&
           alignByte % (dcl->getSubRegAlign() * 2) == 0)) {
        dcl->setSubRegAlign(G4_SubReg_Align(alignByte / 2));
      } else if ((dcl->getSubRegAlign() * 2) < alignByte ||
                 (dcl->getSubRegAlign() * 2) % alignByte != 0) {
        isAligned = false;
      }
    } else if (opnd->getKind() == G4_Operand::dstRegRegion &&
               // Only care about GRF or half-GRF alignment.
               (alignByte == numEltPerGRF<Type_UB>() ||
                alignByte == numEltPerGRF<Type_UB>() / 2) &&
               dcl && (dcl->getRegFile() == G4_ADDRESS)) {

      G4_INST *inst = opnd->getInst();
      if (inst) {
        // Check address calculation like:
        //
        //    shl (1) V1  V0          imm
        //    add (1) a0  $V2 + off   V1
        //    ...
        //    (use)... r[a0, disp] ...
        //
        // need to check both disp, off, and V1 are aligned.
        //
        // Check acc_use_op's def-list.
        G4_INST *LEA = getSingleDefInst(inst, Opnd_dst);
        if (LEA && LEA->opcode() == G4_add && LEA->getExecSize() == g4::SIMD1) {
          isAligned = true;
          G4_Operand *Op0 = LEA->getSrc(0);
          G4_Operand *Op1 = LEA->getSrc(1);
          if (Op0->isSrcRegRegion()) {
            // TODO: Consider MUL as well.
            G4_INST *Def = getSingleDefInst(LEA, Opnd_src0);
            if (Def && Def->opcode() == G4_shl && Def->getSrc(1)->isImm()) {
              G4_Imm *Imm = Def->getSrc(1)->asImm();
              unsigned Factor = (1U << Imm->getInt());
              // TODO: We only perform alignment checking on
              // component wise and may need to consider checking
              // the accumulated result.
              if (Factor % alignByte != 0)
                isAligned = false;
            } else if (Def && Def->opcode() == G4_and &&
                       Def->getSrc(1)->isImm()) {
              G4_Imm *Imm = Def->getSrc(1)->asImm();
              uint64_t Mask = uint64_t(Imm->getInt());
              // alignByte could be 32 or 16 guarded previsouly.
              uint64_t AlignMask = alignByte - 1;
              if ((Mask & AlignMask) != 0)
                isAligned = false;
            } else
              isAligned = false;
          }
          if (isAligned && Op1->isAddrExp()) {
            G4_AddrExp *AE = Op1->asAddrExp();
            G4_Declare *Dcl = AE->getRegVar()->getDeclare();
            unsigned AliasOffset = 0;
            while (Dcl && Dcl->getAliasDeclare()) {
              AliasOffset += Dcl->getAliasOffset();
              Dcl = Dcl->getAliasDeclare();
            }
            // TODO: We only perform alignment checking on
            // component wise and may need to consider checking
            // the accumulated result.
            if ((AliasOffset % alignByte) != 0 ||
                (Dcl && Dcl->getSubRegAlign() != getGRFAlign() &&
                 Dcl->getSubRegAlign() != Sixteen_Word &&
                 Dcl->getSubRegAlign() != Eight_Word) ||
                AE->getOffset() % alignByte != 0) {
              isAligned = false;
            }
          } else
            isAligned = false;
          if (isAligned) {
            // TODO: We only perform alignment checking on
            // component wise and may need to consider checking
            // the accumulated result.
            if (opnd->asDstRegRegion()->getAddrImm() % alignByte != 0)
              isAligned = false;
          }
        }
      }
    } else if (dcl && dcl->getRegFile() == G4_FLAG) {
      // need to make flag even-word aligned if it's used in a setp with dword
      // source ToDo: should we fix input to use 16-bit value instead
      if (alignByte == 4) {
        dcl->setSubRegAlign(Even_Word);
      }
    }
    break;
  }
  default:
    break;
  }
  return isAligned;
}

bool IR_Builder::tryToAlignOperand(G4_Operand *opnd, int alignByte) const {
  uint16_t offset = 0; // ignored
  return tryToAlignOperand(opnd, offset, alignByte);
}

void IR_Builder::predefinedVarRegAssignment(uint8_t inputSize) {
  uint32_t preDefinedStart = ((inputSize + G4_DSIZE - 1) / G4_DSIZE) * G4_DSIZE;
  if (preDefinedStart == 0) {
    preDefinedStart = numEltPerGRF<Type_UB>();
  }
  for (PreDefinedVarsInternal i : allPreDefVars) {
    if (!predefinedVarNeedGRF(i)) {
      continue;
    }

    G4_Type ty = GetGenTypeFromVISAType(getPredefinedVarType(i));
    G4_Declare *dcl = preDefVars.getPreDefinedVar((PreDefinedVarsInternal)i);
    if (!isPredefinedVarInR0((PreDefinedVarsInternal)i)) {
      unsigned short new_offset =
          preDefinedStart + getPredefinedVarByteOffset(i);
      unsigned int regNum = new_offset / numEltPerGRF<Type_UB>();
      unsigned int subRegNum =
          (new_offset % numEltPerGRF<Type_UB>()) / TypeSize(ty);
      dcl->getRegVar()->setPhyReg(phyregpool.getGreg(regNum), subRegNum);
    } else {
      unsigned int regNum = 0;
      unsigned int subRegNum = getPredefinedVarByteOffset(i) / TypeSize(ty);
      dcl->getRegVar()->setPhyReg(phyregpool.getGreg(regNum), subRegNum);
    }
  }
}

// Expand some of the pre-defined variables at kernel entry
// -- replace pre-defined V17 (hw_tid)
// -- replace pre-defined V22 (color)
// -- replace pre-defined V1 (thread_x)
// -- replace pre-defined V2 (thread_y)
void IR_Builder::expandPredefinedVars() {
  // first non-label instruction
  auto iter = std::find_if(instList.begin(), instList.end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });

  if (preDefVars.isHasPredefined(PreDefinedVarsInternal::HW_TID)) {
    if (getPlatform() >= Xe3 && isKernelArgumentEnabled()) {
      // Use FFTID from state register
      // shr (1) hw_tid, sr0.1, 22
      //
      // 31:22  FFTID. This ID is assigned by TS and is a unique identifier for
      // the thread in comparison to other concurrent root threads. It is used
      // to free up resources used by the thread upon thread completion.
      G4_SrcRegRegion *src = createSrc(builtinSR0Dot1->getRegVar(), 0, 0,
                                       getRegionScalar(), Type_UD);
      G4_DstRegRegion *dst = createDstRegRegion(builtinHWTID, 1);
      G4_INST *inst =
          createBinOp(G4_shr, g4::SIMD1, dst, src, createImm(22, Type_UW),
                      InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    } else {
       // Use FFTID from msg header
       // and (1) hw_tid, r0.5, 0x3ff
       //
       // 9:0     FFTID. This ID is assigned by TS and is a unique identifier
       // for the thread in comparison to other concurrent root threads. It is
       // used to free up resources used by the thread upon thread completion.
       //
       // [Pre-DevBDW]: Format = U8. Bits 9:8 are Reserved, MBZ.
       //
       // [0:8] For Pre-Gen9
       // [0:9] For Gen10+
       //
      const unsigned fftid_mask = getPlatform() >= GENX_CNL ? 0x3FF : 0x1FF;
      G4_SrcRegRegion *src =
          createSrc(realR0->getRegVar(), 0, 5, getRegionScalar(), Type_UD);
      G4_Imm *mask1 = createImm(fftid_mask, Type_UD);
      G4_DstRegRegion *dst = createDstRegRegion(builtinHWTID, 1);
      G4_INST *inst = createBinOp(G4_and, g4::SIMD1, dst, src, mask1,
                                  InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    }
  }

  if (preDefVars.isHasPredefined(PreDefinedVarsInternal::X)) {
    if (useNewR0Format()) {
      // x -> and (1) thread_x<1>:uw r0.1:ud 0xFFF
      G4_SrcRegRegion *r0Dot1UD =
          createSrc(realR0->getRegVar(), 0, 1, getRegionScalar(), Type_UD);
      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::X), 1);
      G4_INST *inst =
          createBinOp(G4_and, g4::SIMD1, dst, r0Dot1UD,
                      createImm(0xFFF, Type_UW), InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    } else {
      //  We insert the new instruction
      //  and (1) thread_x<1>:uw, r0.2:uw, 0x01FF
      G4_SrcRegRegion *r0Dot2UW =
          createSrc(realR0->getRegVar(), 0, 2, getRegionScalar(), Type_UW);
      int64_t mask = getThreadIDMask();
      G4_Imm *src1 = createImm(mask, Type_UW);
      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::X), 1);
      G4_INST *inst = createBinOp(G4_and, g4::SIMD1, dst, r0Dot2UW, src1,
                                  InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    }
  }

  if (preDefVars.isHasPredefined(PreDefinedVarsInternal::Y)) {
    if (useNewR0Format()) {
      // y -> shr (1) thread_y<1>:uw r0.1:ud 12
      //      and (1) thread_y<1>:uw thread_y:uw 0xFFF
      G4_SrcRegRegion *r0Dot1UD =
          createSrc(realR0->getRegVar(), 0, 1, getRegionScalar(), Type_UD);

      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::Y), 1);
      G4_INST *inst1 =
          createBinOp(G4_shr, g4::SIMD1, dst, r0Dot1UD, createImm(12, Type_UW),
                      InstOpt_WriteEnable, false);
      instList.insert(iter, inst1);
      dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::Y), 1);
      G4_INST *inst2 =
          createBinOp(G4_and, g4::SIMD1, dst,
                      createSrcRegRegion(preDefVars.getPreDefinedVar(
                                             PreDefinedVarsInternal::Y),
                                         getRegionScalar()),
                      createImm(0xFFF, Type_UW), InstOpt_WriteEnable, false);
      instList.insert(iter, inst2);
    } else {
      //  We insert the new instruction
      //  and (1) thread_y<1>:uw, r0.3:uw, 0x01FF
      G4_SrcRegRegion *r0Dot3UW =
          createSrc(realR0->getRegVar(), 0, 3, getRegionScalar(), Type_UW);
      int64_t mask = getThreadIDMask();
      G4_Imm *src1 = createImmWithLowerType(mask, Type_UW);
      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::Y), 1);
      G4_INST *inst = createBinOp(G4_and, g4::SIMD1, dst, r0Dot3UW, src1,
                                  InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    }
  }

  // color bit
  if (preDefVars.isHasPredefined(PreDefinedVarsInternal::COLOR)) {
    if (useNewR0Format()) {
      // r0.1[31:24]
      // shr (1) color<2>:uw r0.1<0;1,0>:ud 24
      G4_SrcRegRegion *src =
          createSrc(realR0->getRegVar(), 0, 1, getRegionScalar(), Type_UD);
      G4_Imm *shift = createImm(24, Type_UW);
      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::COLOR), 2);
      G4_INST *inst = createBinOp(G4_shr, g4::SIMD1, dst, src, shift,
                                  InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    } else {
      // else: r0.2[3:0]
      // and (1) color<2>:uw r0.2<0;1,0>:ud 0xF
      G4_SrcRegRegion *src =
          createSrc(realR0->getRegVar(), 0, 2, getRegionScalar(), Type_UD);
      G4_Imm *mask = createImm(0xF, Type_UW);
      G4_DstRegRegion *dst = createDstRegRegion(
          preDefVars.getPreDefinedVar(PreDefinedVarsInternal::COLOR), 2);
      G4_INST *inst = createBinOp(G4_and, g4::SIMD1, dst, src, mask,
                                  InstOpt_WriteEnable, false);
      instList.insert(iter, inst);
    }
  }
}

FCPatchingInfo *IR_Builder::getFCPatchInfo() {
  // Create new instance of FC patching class if one is not yet created.
  if (!fcPatchInfo)
    fcPatchInfo = new FCPatchingInfo();
  return fcPatchInfo;
}

// We only keep variable/label names in debug mode or offline vISA executable.
const char *IR_Builder::getNameString(size_t size, const char *format, ...) {
#if defined(_DEBUG) || !defined(DLL_MODE)
  char *name = (char *)debugNameMem.alloc(size);
  va_list args;
  va_start(args, format);
  std::vsnprintf(name, size, format, args);
  va_end(args);
  return name;
#else
  return "";
#endif
}

std::optional<G4_FCALL> IR_Builder::getFcallInfo(const G4_INST *inst) const {
  auto it = m_fcallInfo.find(inst);
  return m_fcallInfo.end() == it ? std::nullopt : std::optional(it->second);
}

void IR_Builder::createPreDefinedVars() {
  for (PreDefinedVarsInternal i : allPreDefVars) {
    G4_Declare *dcl = nullptr;

    if (predefinedVarNeedGRF(i)) {
      // work item id variables are handled uniformly
      G4_Type ty = GetGenTypeFromVISAType(getPredefinedVarType(i));
      dcl = createPreVar(getPredefinedVarID(i), 1, ty);
    } else {
      const char *name = getPredefinedVarString(i);
      switch (i) {
      case PreDefinedVarsInternal::VAR_NULL:
        dcl = createDeclare(name, G4_GRF, 1, 1, Type_UD);
        dcl->getRegVar()->setPhyReg(phyregpool.getNullReg(), 0);
        break;
      case PreDefinedVarsInternal::TSC: {
        G4_Declare *tscDcl = createPreVar(i, 5, Type_UD);
        tscDcl->getRegVar()->setPhyReg(phyregpool.getTm0Reg(), 0);
        dcl = tscDcl;
        break;
      }
      case PreDefinedVarsInternal::R0: {
        dcl = getBuiltinR0();
        break;
      }
      case PreDefinedVarsInternal::SR0: {
        G4_Declare *sr0Dcl = createPreVar(i, 4, Type_UD);
        sr0Dcl->getRegVar()->setPhyReg(phyregpool.getSr0Reg(), 0);
        dcl = sr0Dcl;
        break;
      }
      case PreDefinedVarsInternal::CR0: {
        G4_Declare *cr0Dcl = createPreVar(i, 3, Type_UD);
        cr0Dcl->getRegVar()->setPhyReg(phyregpool.getCr0Reg(), 0);
        dcl = cr0Dcl;
        break;
      }
      case PreDefinedVarsInternal::CE0: {
        G4_Declare *ce0Dcl = createPreVar(i, 1, Type_UD);
        ce0Dcl->getRegVar()->setPhyReg(phyregpool.getMask0Reg(), 0);
        dcl = ce0Dcl;
        break;
      }
      case PreDefinedVarsInternal::DBG: {
        G4_Declare *dbgDcl = createPreVar(i, 2, Type_UD);
        dbgDcl->getRegVar()->setPhyReg(phyregpool.getDbgReg(), 0);
        dcl = dbgDcl;
        break;
      }
      case PreDefinedVarsInternal::ARG: {
        dcl =
            createDeclare(name, G4_INPUT, numEltPerGRF<Type_UD>(), 32, Type_UD);
        dcl->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.stackCall.argReg),
                                    0);
        break;
      }
      case PreDefinedVarsInternal::RET: {
        dcl = createDeclare(name, G4_GRF, numEltPerGRF<Type_UD>(), 12, Type_UD);
        dcl->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.stackCall.retReg),
                                    0);
        dcl->setLiveOut();
        break;
      }
      case PreDefinedVarsInternal::FE_SP: {
        unsigned int startReg = kernel.stackCall.getFPSPGRF();
        dcl = createDeclare(name, G4_GRF, 1, 1, Type_UQ);
        dcl->getRegVar()->setPhyReg(phyregpool.getGreg(startReg),
                                    kernel.stackCall.subRegs.FE_SP);
        break;
      }
      case PreDefinedVarsInternal::FE_FP: {
        // PREDEFINED_FE_FP
        unsigned int startReg = kernel.stackCall.getFPSPGRF();
        dcl = createDeclare(name, G4_GRF, 1, 1, Type_UQ);
        dcl->getRegVar()->setPhyReg(phyregpool.getGreg(startReg),
                                    kernel.stackCall.subRegs.FE_FP);
        break;
      }
      case PreDefinedVarsInternal::HW_TID: {
        // PREDEFINED_HW_TID
        dcl = getBuiltinHWTID();
        break;
      }
      case PreDefinedVarsInternal::X:
      case PreDefinedVarsInternal::Y:
      case PreDefinedVarsInternal::COLOR: {
        // these three are size 1 UW
        dcl = createDeclare(name, G4_GRF, 1, 1,
                            GetGenTypeFromVISAType(getPredefinedVarType(i)));
        break;
      }
      case PreDefinedVarsInternal::IMPL_ARG_BUF_PTR: {
        dcl = createDeclare("implBufPtr", G4_GRF, 1, 1, Type_UQ);
        auto phyReg = phyregpool.getGreg(kernel.stackCall.getSpillHeaderGRF());
        dcl->getRegVar()->setPhyReg(
            phyReg, (uint32_t)StackCallABI::SubRegs_ImplBufPtr);
        break;
      }

      case PreDefinedVarsInternal::LOCAL_ID_BUF_PTR: {
        dcl = createDeclare("localIdBufPtr", G4_GRF, 1, 1, Type_UQ);
        auto phyReg = phyregpool.getGreg(kernel.stackCall.getSpillHeaderGRF());
        dcl->getRegVar()->setPhyReg(
            phyReg, (uint32_t)StackCallABI::SubRegs_LocalIdBufPtr);
        break;
      }
      case PreDefinedVarsInternal::MSG0: {
        G4_Declare *msg0Dcl = createPreVar(i, 3, Type_UD);
        msg0Dcl->getRegVar()->setPhyReg(phyregpool.getMsg0Reg(), 0);
        dcl = msg0Dcl;
        break;
      }
      case PreDefinedVarsInternal::SCRATCHLOC: {
        G4_Declare *scratchDcl = createPreVar(i, 1, Type_UQ);
        scratchDcl->getRegVar()->setPhyReg(phyregpool.getScalarReg(), 7);
        dcl = scratchDcl;
        break;
      }
      default: {
        vISA_ASSERT(false, "Invalid PreDefinedVarsInternal value");
        break;
      }
      }
    }
    preDefVars.setPredefinedVar(i, dcl);
    dcl->setPreDefinedVar(true);
  }
}

void IR_Builder::createBuiltinDecls() {
  // realR0 is always tied to physical r0
  auto numR0DW = numEltPerGRF<Type_UD>();
  realR0 = createDeclare("BuiltInR0", G4_INPUT, numR0DW, 1, Type_UD);
  realR0->getRegVar()->setPhyReg(phyregpool.getGreg(0), 0);
  realR0->setBuiltin();

  // builtinR0 either gets allocated to r0 or to a different
  // register depending on conditions in RA.
  builtinR0 = createTempVar(numR0DW, Type_UD, getGRFAlign(), "R0_Copy");
  builtinR0->setDoNotSpill();
  builtinR0->setBuiltin();

  builtinA0 = createDeclare("BuiltinA0", G4_ADDRESS, 1, 1, Type_UD);
  builtinA0->getRegVar()->setPhyReg(phyregpool.getAddrReg(), 0);
  builtinA0->setBuiltin();

  builtinA0Dot2 = createDeclare("BuiltinA0Dot2", // a0.2
                                G4_ADDRESS, 1, 1, Type_UD);
  builtinA0Dot2->getRegVar()->setPhyReg(phyregpool.getAddrReg(), 2);
  builtinA0Dot2->setBuiltin();

  builtinHWTID = createDeclare("hw_tid", G4_GRF, 1, 1, Type_UD);
  builtinHWTID->setBuiltin();

  builtinSR0Dot1 = createDeclare("BuiltinSR0Dot1", G4_INPUT, 1, 1, Type_UD);
  builtinSR0Dot1->getRegVar()->setPhyReg(phyregpool.getSr0Reg(), 1);
  builtinSR0Dot1->setBuiltin();

  if (getPlatform() >= Xe3) {
    vISA_ASSERT(getScalarRegisterSizeInBytes() % 8 == 0,
                "reg size should be multiple of 8");
    builtinS0 =
      createDeclare("BuiltinS0", G4_SCALAR,
                    getScalarRegisterSizeInBytes() / 8, 1, Type_UQ);
    builtinS0->getRegVar()->setPhyReg(phyregpool.getScalarReg(), 0);
    builtinS0->setBuiltin();
  }

  builtinT252 = createDeclare(vISAPreDefSurf[PREDEFINED_SURFACE_T252].name,
                              G4_GRF, 1, 1, Type_UD);
  builtinT252->setBuiltin();
  builtinBindlessSampler = createDeclare("B_S", G4_GRF, 1, 1, Type_UD);
  builtinBindlessSampler->setBuiltin();

  builtinSamplerHeader = createDeclare("samplerHeader", G4_GRF,
                                       numEltPerGRF<Type_UD>(), 1, Type_UD);
  builtinSamplerHeader->setBuiltin();

  builtinScratchSurface = createDeclare(
      vISAPreDefSurf[PREDEFINED_SURFACE_SCRATCH].name, G4_GRF, 1, 1, Type_UD);
  builtinScratchSurface->setBuiltin();
}

G4_Declare *IR_Builder::getSpillFillHeader() {
  if (!spillFillHeader) {
    spillFillHeader = createTempVar(1, Type_UD, getGRFAlign(), "spillHeader");
    spillFillHeader->setLiveOut();
    spillFillHeader->setLiveIn();
    spillFillHeader->setDoNotSpill();
  }
  return spillFillHeader;
}

G4_Declare *IR_Builder::getScatterSpillBaseAddress() {
  if (!scatterSpillBaseAddress) {
    scatterSpillBaseAddress = createTempVar(kernel.getSimdSize(), Type_D,
                                            getGRFAlign(), "baseAddresses");
    scatterSpillBaseAddress->setLiveOut();
    scatterSpillBaseAddress->setLiveIn();
    scatterSpillBaseAddress->setDoNotSpill();
  }
  return scatterSpillBaseAddress;
}

G4_Declare *IR_Builder::getScatterSpillAddress() {
  if (!scatterSpillAddress) {
    scatterSpillAddress = createTempVar(kernel.getSimdSize(), Type_D,
                                        getGRFAlign(), "spillLaneAddresses");
    scatterSpillAddress->setLiveOut();
    scatterSpillAddress->setLiveIn();
    scatterSpillAddress->setDoNotSpill();
  }
  return scatterSpillAddress;
}

G4_Declare *IR_Builder::getEUFusionWATmpVar() {
  if (!euFusionWATmpVar) {
    euFusionWATmpVar = createTempVar(2, Type_UD, Even_Word, "euFusionWATmp");
    euFusionWATmpVar->setLiveOut();
    euFusionWATmpVar->setLiveIn();
    euFusionWATmpVar->setDoNotSpill();
  }
  return euFusionWATmpVar;
}

G4_Declare *IR_Builder::getOldA0Dot2Temp() {
  if (!oldA0Dot2Temp) {
    oldA0Dot2Temp = createTempVar(1, Type_UD, Any, "OldA0Dot2");
    oldA0Dot2Temp->setLiveOut();
    oldA0Dot2Temp->setLiveIn();
    oldA0Dot2Temp->setDoNotSpill();
  }
  return oldA0Dot2Temp;
}

IR_Builder::IR_Builder(INST_LIST_NODE_ALLOCATOR &alloc, G4_Kernel &k,
                       Mem_Manager &m, Options *options,
                       CISA_IR_Builder *parent, FINALIZER_INFO *jitInfo,
                       const WA_TABLE *pWaTable)
    : immPool(*this),
      metaData(jitInfo),
      m_pWaTable(pWaTable),
      m_options(options),
      CanonicalRegionStride0(0, 1, 0),
      CanonicalRegionStride1(1, 1, 0),
      CanonicalRegionStride2(2, 1, 0),
      CanonicalRegionStride4(4, 1, 0),
      parentBuilder(parent),
      freqInfoManager(this, k),
      r0AccessMode(getR0AccessFromOptions()),
      mem(m), phyregpool(m, k.grfMode.getInitalGRFNum()),
      hashtable(m), rgnpool(m), dclpool(m, *this), instList(alloc),
      kernel(k)
{
  num_temp_dcl = 0;
  kernel.setBuilder(this); // kernel needs pointer to the builder
  if (!getIsPayload())
    createBuiltinDecls();

  sampler8x8_group_id = 0;

  be_sp = be_fp = tmpFCRet = nullptr;

  arg_size = 0;
  return_var_size = 0;
  if (metaData != NULL) {
    *metaData = FINALIZER_INFO();
  }

  usedBarriers = BitSet(kernel.getMaxNumOfBarriers(), false);

  if (!getIsPayload())
    createPreDefinedVars();
}

IR_Builder::~IR_Builder() {
  // We need to invoke the destructor of every instruction ever allocated
  // so that its members will be freed.
  // Note that we don't delete the instruction itself as it's allocated from
  // the memory manager's pool
  for (unsigned i = 0, size = (unsigned)instAllocList.size(); i != size; i++) {
    G4_INST *inst = instAllocList[i];
    inst->~G4_INST();
  }
  instAllocList.clear();

  for (auto MD : allMDs) {
    MD->~Metadata();
  }

  for (auto node : allMDNodes) {
    node->~MDNode();
  }
  delete fcPatchInfo;
}

G4_Declare *
IR_Builder::cloneDeclare(std::map<G4_Declare *, G4_Declare *> &dclMap,
                         G4_Declare *dcl) {
  static int uid = 0;
  const char *newDclName =
      getNameString(16, "copy_%d_%s", uid++, dcl->getName());
  return dclpool.cloneDeclare(kernel, dclMap, newDclName, dcl);
}

G4_Declare *IR_Builder::createDeclare(const char *name, G4_RegFileKind regFile,
                                      unsigned short n_elems,
                                      unsigned short n_rows, G4_Type ty,
                                      DeclareType kind, G4_RegVar *base,
                                      G4_Operand *repRegion,
                                      G4_ExecSize execSize) {
  if (regFile == G4_FLAG) {
    vISA_ASSERT(ty == Type_UW, "flag decl must have type UW");
  }

  G4_Declare *dcl = dclpool.createDeclare(name, regFile, n_elems, n_rows, ty,
                                          kind, base, repRegion, execSize);

  kernel.Declares.push_back(dcl);

  return dcl;
}

uint32_t IR_Builder::getSplitEMask(unsigned execSize, uint32_t eMask,
                                   bool isLo) {
  const uint32_t qhMasks = InstOpt_M0 | InstOpt_M8 | InstOpt_M16 | InstOpt_M24;
  uint32_t other = eMask & ~qhMasks;
  uint32_t qh = eMask & qhMasks;

  switch (execSize) {
  case 16: // Split SIMD16 into SIMD8
    switch (qh) {
    case 0: // instOpt not specified, treat as 1H
    case InstOpt_M0:
      return (isLo ? InstOpt_M0 : InstOpt_M8) | other;
    case InstOpt_M16:
      return (isLo ? InstOpt_M16 : InstOpt_M24) | other;
    }
    break;
  case 32: // Split SIMD32 into SIMD16.
    switch (qh) {
    case 0:
      return (isLo ? InstOpt_M0 : InstOpt_M16) | other;
    }
    break;
  }

  vISA_ASSERT_UNREACHABLE("Unhandled cases for EMask splitting!");
  return ~0U;
}

void IR_Builder::initScratchSurfaceOffset() {
  bool allocScratchSurfaceOffset = !scratchSurfaceOffset;
  if (isEfficient64bEnabled()) {
    if (!scratchSurfaceEff64b) {
      scratchSurfaceEff64b =
          createDeclare("scratchSurfaceEff64", G4_SCALAR, 1, 1, Type_UQ);
      scratchSurfaceEff64b->setSubRegAlign(G4_SubReg_Align::Four_Word);
      scratchSurfaceEff64b->setLiveIn();
      scratchSurfaceEff64b->setLiveOut();
      scratchSurfaceEff64b->setDoNotSpill();
      // Tie it to s0.7:uq for all cases
      scratchSurfaceEff64b->getRegVar()->setPhyReg(phyregpool.getScalarReg(),
                                                   7);
    }
    // disable legacy a0.2-based scratch offset
    allocScratchSurfaceOffset = false;
  }
  // (W) and (1) sso r0.5 0xFFFFC00, placed at kernel entry
  if (allocScratchSurfaceOffset) {
    scratchSurfaceOffset = createTempVar(1, Type_UD, Any, "SSO");
    scratchSurfaceOffset->setLiveOut();
    scratchSurfaceOffset->setDoNotSpill();

    G4_SrcRegRegion *R0_5 =
        createSrc(builtinR0->getRegVar(), 0, 5, getRegionScalar(), Type_UD);
    if (kernel.getBoolKernelAttr(Attributes::ATTR_SepSpillPvtSS)) {
      G4_Declare *slot0SSO = createTempVar(1, Type_UD, Any, "Slot0SSO");
      G4_DstRegRegion *andDst = createDstRegRegion(slot0SSO, 1);
      auto andInst = createBinOp(G4_and, g4::SIMD1, andDst, R0_5,
                                  createImm(0xFFFFFC00, Type_UD),
                                  InstOpt_WriteEnable, true);
      instList.pop_back();
      auto iter =
          std::find_if(instList.begin(), instList.end(),
                        [](G4_INST *inst) { return !inst->isLabel(); });
      instList.insert(iter, andInst);
      setSSOInst(andInst);

      // scratchSurfaceOffset (r0.5+0x400)>>4 is reserved for spillfill,
      // pvtmem should use r0.5>>4 shift-right by 4 is done before send-msg
      // when setting a0
      G4_DstRegRegion *dst = createDstRegRegion(scratchSurfaceOffset, 1);
      createBinOp(G4_add, g4::SIMD1, dst,
                  createSrcRegRegion(slot0SSO, getRegionScalar()),
                  createImm(0x400, Type_UD), InstOpt_WriteEnable, true);
    } else {
      G4_DstRegRegion *andDst = createDstRegRegion(scratchSurfaceOffset, 1);
      auto andInst = createBinOp(G4_and, g4::SIMD1, andDst, R0_5,
                                  createImm(0xFFFFFC00, Type_UD),
                                  InstOpt_WriteEnable, true);
      andInst->setVISAId(UNMAPPABLE_VISA_INDEX);
      instList.pop_back();
      auto iter =
          std::find_if(instList.begin(), instList.end(),
                        [](G4_INST *inst) { return !inst->isLabel(); });
      instList.insert(iter, andInst);
      setSSOInst(andInst);
    }
  }

  // Init addresses for Scatter messages
  if (kernel.getOptions()->getOption(vISA_scatterSpill) &&
      !scatterSpillBaseAddress) {
    initAddressesForScatterSpills();
  }
}

// Init vector of addresses for scatter spills assuming a max
// message execution size of SIMD32.
// These instructions are placed at entry kernel
// TODO: Add support for a16 addresses to save registers
//       when spill size is less than 64KB.
void IR_Builder::initAddressesForScatterSpills() {
  auto iter = std::find_if(instList.begin(), instList.end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });
  G4_Declare *baseAddresses = getScatterSpillBaseAddress();
  getScatterSpillAddress();
  auto simdSize = kernel.getSimdSize();

  if (getGRFSize() == 32) {
    // Create 8, 16, or 32 addresses (32b each) based on SIMD size.
    // Generate the following sequence, e.g. for SIMD32:
    // (W) mov (8) r1.0<1>:w   0x76543210:v
    // (W) mov (8) r2.0<1>:d   r1.0<1;1,0>:w
    // (W) add (8) r3.0<1>:d   r1.0<1;1,0>:d  8:w
    // (W) add (8) r4.0<1>:d   r2.0<1;1,0>:d  16:w
    // (W) add (8) r5.0<1>:d   r2.0<1;1,0>:d  24:w
    // (W) mul (16) r2.0<1>:d   r2.0<1;1,0>:d  4:w   -> get addresses in bytes
    // (W) mul (16) r4.0<1>:d   r4.0<1;1,0>:d  4:w   -> get addresses in bytes
    // Result:
    //  r2-5 will hold 32 final 32b addresses

    // mov (8) r1.0<1>:w   0x76543210:v
    G4_Declare *addressVector =
        createTempVar(g4::SIMD8, Type_W, Sixteen_Word, "AddressVector");
    G4_DstRegRegion *movDst =
        createDst(addressVector->getRegVar(), 0, 0, 1, Type_W);
    G4_INST *mov = createMov(g4::SIMD8, movDst, createImm(0x76543210, Type_V),
                             InstOpt_WriteEnable, false);
    instList.insert(iter, mov);

    // (W) mov (8) r2.0<1>:d   r1.0<1;1,0>:w
    G4_DstRegRegion *movDstAddress =
        createDst(baseAddresses->getRegVar(), 0, 0, 1, Type_D);
    G4_INST *movAddresses =
        createMov(g4::SIMD8, movDstAddress,
                  createSrcRegRegion(addressVector, getRegionStride1()),
                  InstOpt_WriteEnable, false);
    instList.insert(iter, movAddresses);

    if (simdSize > g4::SIMD8) {
      // (W) add (8) r3.0<1>:d   r1.0<1;1,0>:d  8:w
      G4_DstRegRegion *addDstAddress =
          createDst(baseAddresses->getRegVar(), 1, 0, 1, Type_D);
      G4_INST *addAddreses =
          createBinOp(G4_add, g4::SIMD8, addDstAddress,
                      createSrcRegRegion(addressVector, getRegionStride1()),
                      createImm(8, Type_W), InstOpt_WriteEnable, false);
      instList.insert(iter, addAddreses);

      if (simdSize > g4::SIMD16) {
        // (W) add (8) r4.0<1>:d   r2.0<1;1,0>:d  16:w
        G4_DstRegRegion *addDstAddress2 =
            createDst(baseAddresses->getRegVar(), 2, 0, 1, Type_D);
        G4_INST *addAddreses2 =
            createBinOp(G4_add, g4::SIMD8, addDstAddress2,
                        createSrcRegRegion(baseAddresses, getRegionStride1()),
                        createImm(16, Type_W), InstOpt_WriteEnable, false);
        instList.insert(iter, addAddreses2);

        // (W) add (8) r5.0<1>:d   r2.0<1;1,0>:d  24:w
        G4_DstRegRegion *addDstAddress3 =
            createDst(baseAddresses->getRegVar(), 3, 0, 1, Type_D);
        G4_INST *addAddreses3 =
            createBinOp(G4_add, g4::SIMD8, addDstAddress3,
                        createSrcRegRegion(baseAddresses, getRegionStride1()),
                        createImm(24, Type_W), InstOpt_WriteEnable, false);
        instList.insert(iter, addAddreses3);
      }
    }

    // (W) mul (16) r2.0<1>:d   r2.0<1;1,0>:d  4:w   -> get addresses in bytes
    G4_DstRegRegion *mulDstAddress =
        createDst(baseAddresses->getRegVar(), 0, 0, 1, Type_D);
    G4_INST *mulAddresses =
        createBinOp(G4_mul, simdSize > g4::SIMD8 ? g4::SIMD16 : g4::SIMD8, mulDstAddress,
                    createSrcRegRegion(baseAddresses, getRegionStride1()),
                    createImm(4, Type_W), InstOpt_WriteEnable, false);
    instList.insert(iter, mulAddresses);

    if (simdSize > g4::SIMD16) {
      // (W) mul (16) r4.0<1>:d   r4.0<1;1,0>:d  4:w   -> get addresses in bytes
      G4_DstRegRegion *mulDstAddress2 =
          createDst(baseAddresses->getRegVar(), 2, 0, 1, Type_D);
      G4_INST *mulAddresses2 =
          createBinOp(G4_mul, g4::SIMD16, mulDstAddress2,
                      createSrc(baseAddresses->getRegVar(), 2, 0,
                                getRegionStride1(), Type_D),
                      createImm(4, Type_W), InstOpt_WriteEnable, false);
      instList.insert(iter, mulAddresses2);
    }
  } else if (getGRFSize() == 64) {
    // Create 8,16, or 32 addresses (32b each) based on SIMD size
    // Generate the following sequence, e.g. for SIMD32::
    // (W) mov (8) r1.0<1>:w   0x76543210:v
    // (W) mov (8) r2.0<1>:d    r1.0<1;1,0>:w
    // (W) add (8) r2.8<1>:d   r2.0<1;1,0>:d  8:w
    // (W) add (16) r3.0<1>:d   r2.0<1;1,0>:d  16:w
    // (W) mul (32) r2.0<1>:d   r2.0<1;1,0>:d  4:w   -> get addresses in bytes
    // Result:
    //  r2 and r3 will hold 32 final 32b addresses

    // (W) mov (8) r1.0<1>:w   0x76543210:v
    G4_Declare *addressVector =
        createTempVar(g4::SIMD8, Type_W, Sixteen_Word, "AddressVector");
    G4_DstRegRegion *movDst =
        createDst(addressVector->getRegVar(), 0, 0, 1, Type_W);
    G4_INST *mov = createMov(g4::SIMD8, movDst, createImm(0x76543210, Type_V),
                             InstOpt_WriteEnable, false);
    instList.insert(iter, mov);

    // (W) mov (8) r2.0<1>:d    r1.0<1;1,0>:w
    G4_DstRegRegion *movDstAddress =
        createDst(baseAddresses->getRegVar(), 0, 0, 1, Type_D);
    G4_INST *movAddresses =
        createMov(g4::SIMD8, movDstAddress,
                  createSrcRegRegion(addressVector, getRegionStride1()),
                  InstOpt_WriteEnable, false);
    instList.insert(iter, movAddresses);

    if (simdSize > g4::SIMD8) {
      // (W) add (8) r2.8<1>:d   r2.0<1;1,0>:d  8:w
      G4_DstRegRegion *addDstAddress =
          createDst(baseAddresses->getRegVar(), 0, 8, 1, Type_D);
      G4_INST *addAddresses =
          createBinOp(G4_add, g4::SIMD8, addDstAddress,
                      createSrcRegRegion(baseAddresses, getRegionStride1()),
                      createImm(8, Type_W), InstOpt_WriteEnable, false);
      instList.insert(iter, addAddresses);

      if (simdSize > g4::SIMD16) {
        // (W) add (16) r3.0<1>:d   r2.0<1;1,0>:d  16:w
        G4_DstRegRegion *addDstAddress2 =
            createDst(baseAddresses->getRegVar(), 1, 0, 1, Type_D);
        G4_INST *addAddresses2 =
            createBinOp(G4_add, g4::SIMD16, addDstAddress2,
                        createSrcRegRegion(baseAddresses, getRegionStride1()),
                        createImm(16, Type_W), InstOpt_WriteEnable, false);
        instList.insert(iter, addAddresses2);
      }
    }

    // (W) mul (8|16|32) r2.0<1>:d   r2.0<1;1,0>:d  4:w   -> get addresses in bytes
    G4_DstRegRegion *mulDstAddress =
        createDst(baseAddresses->getRegVar(), 0, 0, 1, Type_D);
    G4_INST *mulAddresses =
        createBinOp(G4_mul, simdSize, mulDstAddress,
                    createSrcRegRegion(baseAddresses, getRegionStride1()),
                    createImm(4, Type_W), InstOpt_WriteEnable, false);
    instList.insert(iter, mulAddresses);
  }
}

G4_Declare *IR_Builder::createTempVar(unsigned int numElements, G4_Type type,
                                      G4_SubReg_Align subAlign,
                                      const char *prefix, bool appendIdToName) {
  vISA_ASSERT(numElements > 0, "incorrect num elements passed");
  const char *name = appendIdToName
                         ? getNameString(20, "%s%d", prefix, num_temp_dcl++)
                         : getNameString(20, "%s", prefix);

  unsigned short dcl_width = 0, dcl_height = 1;
  const uint16_t typeSize = TypeSize(type);
  int totalByteSize = numElements * typeSize;
  if (totalByteSize <= (int)numEltPerGRF<Type_UB>()) {
    dcl_width = totalByteSize / typeSize;
  } else {
    // here we assume that the start point of the var is the beginning of a GRF?
    // so subregister must be 0?
    dcl_width = numEltPerGRF<Type_UB>() / typeSize;
    dcl_height = totalByteSize / numEltPerGRF<Type_UB>();
    if (totalByteSize % numEltPerGRF<Type_UB>() != 0) {
      dcl_height++;
    }
  }

  G4_Declare *dcl = createDeclare(name, G4_GRF, dcl_width, dcl_height, type);
  dcl->setSubRegAlign(subAlign);
  return dcl;
}

G4_Declare *IR_Builder::createAddrFlagSpillLoc(G4_Declare *dcl) {
  const char *name = getNameString(16, "SP_LOC_%d", numAddrFlagSpillLoc++);
  G4_Declare *spillLoc =
      createDeclare(name, G4_GRF, dcl->getNumElems(), 1, dcl->getElemType(),
                    DeclareType::AddrSpill);
  dcl->setSpilledDeclare(spillLoc);
  if (getPlatform() > Xe3 && dcl->getRegFile() == G4_ADDRESS)
    // On xe3p+ platform, src1 must be aligned to dst. In other words, both src1
    // and dst are GRF-aligned. For the addr_add instruction, if the address
    // variable in the dst is spilled to GRF variable, we must make sure the
    // GRF variable is also GRF-aligned. Otherwise, there would cause unaligned
    // regioning issue between dst and src1.
    spillLoc->setSubRegAlign(getGRFAlign());
  else
    spillLoc->setSubRegAlign(
        dcl->getSubRegAlign()); // for simd32 flag the spill loc has to be
                                // 2-word aligned since it's accessed as dw
  return spillLoc;
}

G4_Declare *IR_Builder::createHardwiredDeclare(uint32_t numElements,
                                               G4_Type type, uint32_t regNum,
                                               uint32_t regOff) {
  G4_Declare *dcl = createTempVar(numElements, type, Any);
  dcl->getRegVar()->setPhyReg(phyregpool.getGreg(regNum), regOff);
  return dcl;
}

void IR_Builder::createPseudoKills(std::initializer_list<G4_Declare *> dcls,
                                   PseudoKillType ty) {
  for (auto dcl : dcls) {
    createPseudoKill(dcl, ty, true);
  }
}

G4_INST *IR_Builder::createPseudoKill(G4_Declare *dcl, PseudoKillType ty,
                                      bool addToInstList) {
  auto dstRgn = createDst(dcl->getRegVar(), 0, 0, 1, Type_UD);
  G4_INST *inst =
      createIntrinsicInst(nullptr, Intrinsic::PseudoKill, g4::SIMD1, dstRgn,
                          createImm((unsigned int)ty, Type_UD), nullptr,
                          nullptr, InstOpt_WriteEnable, addToInstList);

  return inst;
}

static const unsigned int HWORD_BYTE_SIZE = 32;

G4_INST *IR_Builder::createEUWASpill(bool addToInstList) {
  const RegionDesc *rd = getRegionScalar();

  G4_Declare *dcl = getEUFusionWATmpVar();
  G4_SrcRegRegion *pseudoUseSrc =
      createSrc(dcl->getRegVar(), 0, 0, rd, Type_UD);

  G4_INST *pseudoUseInst = createIntrinsicInst(
      nullptr, Intrinsic::FlagSpill, g4::SIMD2, nullptr, pseudoUseSrc, nullptr,
      nullptr, InstOpt_NoOpt, addToInstList);

  return pseudoUseInst;
}

G4_INST *IR_Builder::createSpill(G4_DstRegRegion *dst, G4_SrcRegRegion *header,
                                 G4_SrcRegRegion *payload, G4_ExecSize execSize,
                                 uint16_t numRows, uint32_t offset,
                                 G4_Declare *fp, G4_InstOption option,
                                 bool addToInstList, bool isScatter) {
  G4_INST *spill =
      createIntrinsicInst(nullptr, Intrinsic::Spill, execSize, dst, header,
                          payload, nullptr, option, addToInstList);
  spill->asSpillIntrinsic()->setFP(fp);
  spill->asSpillIntrinsic()->setOffset(
      (uint32_t)(((uint64_t)offset * HWORD_BYTE_SIZE) /
                 numEltPerGRF<Type_UB>()));
  spill->asSpillIntrinsic()->setNumRows(numRows);
  spill->asSpillIntrinsic()->setScatterSpill(isScatter);

  return spill;
}

G4_INST *IR_Builder::createSpill(G4_DstRegRegion *dst, G4_SrcRegRegion *payload,
                                 G4_ExecSize execSize, uint16_t numRows,
                                 uint32_t offset, G4_Declare *fp,
                                 G4_InstOption option, bool addToInstList, bool isScatter) {
  auto builtInR0 = getBuiltinR0();
  auto rd = getRegionStride1();
  auto srcRgnr0 = createSrc(builtInR0->getRegVar(), 0, 0, rd, Type_UD);
  G4_INST *spill =
      createIntrinsicInst(nullptr, Intrinsic::Spill, execSize, dst, srcRgnr0,
                          payload, nullptr, option, addToInstList);
  spill->asSpillIntrinsic()->setFP(fp);
  spill->asSpillIntrinsic()->setOffset(
      (uint32_t)(((uint64_t)offset * HWORD_BYTE_SIZE) /
                 numEltPerGRF<Type_UB>()));
  spill->asSpillIntrinsic()->setNumRows(numRows);
  spill->asSpillIntrinsic()->setScatterSpill(isScatter);
  return spill;
}

G4_INST *IR_Builder::createFill(G4_SrcRegRegion *header,
                                G4_DstRegRegion *dstData, G4_ExecSize execSize,
                                uint16_t numRows, uint32_t offset,
                                G4_Declare *fp, G4_InstOption option,
                                bool addToInstList) {
  G4_INST *fill =
      createIntrinsicInst(nullptr, Intrinsic::Fill, execSize, dstData, header,
                          nullptr, nullptr, option, addToInstList);
  fill->asFillIntrinsic()->setFP(fp);
  fill->asFillIntrinsic()->setOffset(
      (uint32_t)(((uint64_t)offset * HWORD_BYTE_SIZE) /
                 numEltPerGRF<Type_UB>()));
  fill->asFillIntrinsic()->setNumRows(numRows);
  return fill;
}

G4_INST *IR_Builder::createFill(G4_DstRegRegion *dstData, G4_ExecSize execSize,
                                uint16_t numRows, uint32_t offset,
                                G4_Declare *fp, G4_InstOption option,
                                bool addToInstList) {
  auto builtInR0 = getBuiltinR0();
  auto rd = getRegionStride1();
  auto srcRgnr0 = createSrc(builtInR0->getRegVar(), 0, 0, rd, Type_UD);
  G4_INST *fill =
      createIntrinsicInst(nullptr, Intrinsic::Fill, execSize, dstData, srcRgnr0,
                          nullptr, nullptr, option, addToInstList);

  fill->asFillIntrinsic()->setFP(fp);
  fill->asFillIntrinsic()->setOffset(
      (uint32_t)(((uint64_t)offset * HWORD_BYTE_SIZE) /
                 numEltPerGRF<Type_UB>()));
  fill->asFillIntrinsic()->setNumRows(numRows);
  return fill;
}

G4_Declare *IR_Builder::createTempFlag(unsigned short numberOfFlags,
                                       const char *prefix) {
  const char *name = getNameString(20, "%s%d", prefix, num_temp_dcl++);

  G4_Declare *dcl = createDeclare(name, G4_FLAG, numberOfFlags, 1, Type_UW);

  return dcl;
}

G4_Declare *IR_Builder::createFlag(uint16_t numFlagElements, const char *name) {
  uint32_t numWords = (numFlagElements + 15) / 16;
  G4_Declare *dcl = createDeclare(name, G4_FLAG, numWords, 1, Type_UW);
  dcl->setNumberFlagElements((uint8_t)numFlagElements);
  return dcl;
}

G4_Declare *IR_Builder::createTempAddress(uint16_t numAddressElements,
                                          const char *prefix) {
  const char *name = getNameString(20, "%s%d", prefix, num_temp_dcl++);
  G4_Declare *dcl =
      createDeclare(name, G4_ADDRESS, numAddressElements, 1, Type_UD);
  return dcl;
}

G4_Declare *IR_Builder::createTempScalar(uint16_t numFlagElements,
                                         const char *prefix) {
  const char *name = getNameString(20, "%s%d", prefix, num_temp_dcl++);
  G4_Declare *dcl = createDeclare(name, G4_SCALAR, numFlagElements, 1, Type_UQ);
  return dcl;
}

G4_Declare *IR_Builder::createScalar(uint16_t numFlagElements,
                                     const char *name) {
  G4_Declare *dcl = createDeclare(name, G4_SCALAR, numFlagElements, 1, Type_UQ);
  return dcl;
}

G4_Declare *IR_Builder::createPreVar(PreDefinedVarsInternal preDefVar_index,
                                     unsigned short numElements, G4_Type type) {
  vISA_ASSERT(preDefVar_index < PreDefinedVarsInternal::VAR_LAST,
              "illegal predefined var index");
  unsigned short dcl_width = 0, dcl_height = 1;
  auto typeSize = TypeSize(type);
  int totalByteSize = numElements * typeSize;
  if (totalByteSize <= (int)numEltPerGRF<Type_UB>()) {
    dcl_width = totalByteSize / typeSize;
  } else {
    // here we assume that the start point of the var is the beginning of a GRF?
    // so subregister must be 0?
    dcl_width = numEltPerGRF<Type_UB>() / typeSize;
    dcl_height = totalByteSize / numEltPerGRF<Type_UB>();
    if (totalByteSize % numEltPerGRF<Type_UB>() != 0) {
      dcl_height++;
    }
  }

  G4_Declare *dcl =
      createPreVarDeclareNoLookup(preDefVar_index, dcl_width, dcl_height, type);
  // subAlign has to be type size at the minimum
  dcl->setSubRegAlign(Get_G4_SubRegAlign_From_Type(type));
  return dcl;
}

G4_SrcRegRegion *IR_Builder::createSrcWithNewRegOff(G4_SrcRegRegion *old,
                                                    short newRegOff) {
  if (old->getRegAccess() == Direct) {
    return createSrcRegRegion(old->getModifier(), Direct, old->getBase(),
                              newRegOff, old->getSubRegOff(), old->getRegion(),
                              old->getType(), old->getAccRegSel());
  } else {
    return createIndirectSrc(old->getModifier(), old->getBase(), newRegOff,
                             old->getSubRegOff(), old->getRegion(),
                             old->getType(), old->getAddrImm());
  }
}

G4_SrcRegRegion *IR_Builder::createSrcWithNewSubRegOff(G4_SrcRegRegion *old,
                                                       short newSubRegOff) {
  if (old->getRegAccess() == Direct) {
    return createSrcRegRegion(old->getModifier(), old->getRegAccess(),
                              old->getBase(), old->getRegOff(), newSubRegOff,
                              old->getRegion(), old->getType(),
                              old->getAccRegSel());
  } else {
    return createIndirectSrc(old->getModifier(), old->getBase(),
                             old->getRegOff(), newSubRegOff, old->getRegion(),
                             old->getType(), old->getAddrImm());
  }
}

G4_SrcRegRegion *IR_Builder::createSrcWithNewBase(G4_SrcRegRegion *old,
                                                  G4_VarBase *newBase) {
  if (old->getRegAccess() == Direct) {
    return createSrcRegRegion(old->getModifier(), Direct, newBase,
                              old->getRegOff(), old->getSubRegOff(),
                              old->getRegion(), old->getType(),
                              old->getAccRegSel());
  } else {
    return createIndirectSrc(old->getModifier(), newBase, old->getRegOff(),
                             old->getSubRegOff(), old->getRegion(),
                             old->getType(), old->getAddrImm());
  }
}

G4_DstRegRegion *IR_Builder::createDstWithNewSubRegOff(G4_DstRegRegion *old,
                                                       short newSubRegOff) {
  if (old->getRegAccess() == Direct) {
    return createDst(old->getBase(), old->getRegOff(), newSubRegOff,
                     old->getHorzStride(), old->getType(), old->getAccRegSel());
  } else {
    return createIndirectDst(old->getBase(), newSubRegOff, old->getHorzStride(),
                             old->getType(), old->getAddrImm());
  }
}

G4_Imm *IR_Builder::createImm(float fp) {
  uint32_t imm = *((uint32_t *)&fp);
  G4_Type immType = Type_F;
  if (getPlatform() >= GENX_CHV && m_options->getOption(vISA_FImmToHFImm) &&
      !VISA_WA_CHECK(getPWaTable(), WaSrc1ImmHfNotAllowed)) {
    // we may be able to lower it to HF
    // ieee32 format: 23-8-1
    // ieee16 format: 10-5-1
    // bit0-22 are fractions
    uint32_t fraction = imm & 0x7FFFFF;
    // bit23-30 are exponents
    uint32_t exponent = (imm >> 23) & 0xFF;
    uint32_t sign = (imm >> 31) & 0x1;
    int expVal = ((int)exponent) - 127;

    if (exponent == 0 && fraction == 0) {
      // 0 and -0
      immType = Type_HF;
      imm = sign << 15;
    } else if ((fraction & 0x1FFF) == 0 && (expVal <= 15 && expVal >= -16)) {
      // immediate can be exactly represented in HF.
      // we exclude denormal, infinity, and NaN.
      immType = Type_HF;
      uint32_t newExp = (expVal + 15) & 0x1F;
      imm = (sign << 15) | (newExp << 10) | (fraction >> 13);
    }
  }
  G4_Imm *i = hashtable.lookupImm(imm, immType);
  return (i != NULL) ? i : hashtable.createImm(imm, immType);
}

G4_Imm *IR_Builder::createDFImm(double fp) {
  int64_t val = (int64_t)(*(uint64_t *)&fp);
  G4_Imm *i = hashtable.lookupImm(val, Type_DF);
  return (i != NULL) ? i : hashtable.createImm(val, Type_DF);
}

G4_Type IR_Builder::getNewType(int64_t imm, G4_Type ty) {
  switch (ty) {
  case Type_Q:
  case Type_D:
    // It is legal to change a positive imm's type from signed to unsigned if it
    // fits in the unsigned type. We do prefer signed type however for
    // readability.
    if (imm >= MIN_WORD_VALUE && imm <= MAX_WORD_VALUE) {
      return Type_W;
    } else if (imm >= MIN_UWORD_VALUE && imm <= MAX_UWORD_VALUE) {
      return Type_UW;
    } else if (imm >= int(MIN_DWORD_VALUE) && imm <= int(MAX_DWORD_VALUE)) {
      return Type_D;
    } else if (imm >= unsigned(MIN_UDWORD_VALUE) &&
               imm <= unsigned(MAX_UDWORD_VALUE)) {
      return Type_UD;
    }
    break;
  case Type_UQ:
  case Type_UD: {
    // unsigned imm must stay as unsigned
    uint64_t immU = static_cast<uint64_t>(imm);
    if (immU <= MAX_UWORD_VALUE) {
      return Type_UW;
    } else if (immU <= unsigned(MAX_UDWORD_VALUE)) {
      return Type_UD;
    }
    break;
  }
  case Type_UB:
    return Type_UW;
  case Type_B:
    return Type_W;
  default:
    return ty;
  }
  return ty;
}

//
// look up an imm operand
//
G4_Imm *OperandHashTable::lookupImm(int64_t imm, G4_Type ty) {
  ImmKey key(imm, ty);
  auto iter = immTable.find(key);
  return iter != immTable.end() ? iter->second : nullptr;
}

//
// create a dst reg region
//
G4_Imm *OperandHashTable::createImm(int64_t imm, G4_Type ty) {
  G4_Imm *i = new (mem) G4_Imm(imm, ty);
  ImmKey key(imm, ty);
  immTable[key] = i;
  return i;
}

//
// create the region <vstride; width, hstride> if not yet created
//
const RegionDesc *RegionPool::createRegion(uint16_t vstride, uint16_t width,
                                           uint16_t hstride) {

  for (unsigned i = 0, size = (unsigned)rgnlist.size(); i < size; i++) {
    RegionDesc *region = rgnlist[i];
    if (region->vertStride == vstride && region->width == width &&
        region->horzStride == hstride) {
      return region; // exist
    }
  }
  //
  // create one
  //
  RegionDesc *rd = new (mem) RegionDesc(vstride, width, hstride);
  rgnlist.push_back(rd);
  return rd;
}

/*
    Used in IR_Builder::translateVISARawSendInst. All the bits in des and
   extDesc are already set.
*/
G4_SendDescRaw *IR_Builder::createGeneralMsgDesc(uint32_t desc,
                                                 uint32_t extDesc,
                                                 SendAccess access,
                                                 G4_Operand *bti,
                                                 G4_Operand *sti) {
  return new (mem) G4_SendDescRaw(desc, extDesc, access, bti, sti, *this);
}

G4_SendDescRaw *
IR_Builder::createSendMsgDesc(SFID sfid, uint32_t desc, uint32_t extDesc,
                              int src1Len, SendAccess access, G4_Operand *bti,
                              G4_ExecSize execSize, bool isValidFuncCtrl) {
  return new (mem) G4_SendDescRaw(sfid, desc, extDesc, src1Len, access, bti,
                                  execSize, isValidFuncCtrl, *this);
}

G4_SendDescRaw *IR_Builder::createSendMsgDesc(SFID sfid, uint32_t desc,
                                              uint32_t extDesc, int src1Len,
                                              SendAccess access,
                                              G4_Operand *bti,
                                              bool isValidFuncCtrl) {
  return new (mem) G4_SendDescRaw(sfid, desc, extDesc, src1Len, access, bti,
                                  isValidFuncCtrl, *this);
}

G4_SendDescRaw *IR_Builder::createSendMsgDesc(
    unsigned funcCtrl, unsigned regs2rcv, unsigned regs2snd, SFID funcID,
    unsigned extMsgLength, uint16_t extFuncCtrl, SendAccess access,
    G4_Operand *bti, G4_Operand *sti) {
  G4_SendDescRaw *msgDesc = new (mem) G4_SendDescRaw(
      funcCtrl, regs2rcv, regs2snd, funcID, (uint16_t)extMsgLength, extFuncCtrl,
      access, bti, sti, *this);
  return msgDesc;
}

// shorthand for read msg desc. Note that extDesc still needs to be explicitly
// created, SendMsgDesc ctor does not program all the bits
G4_SendDescRaw *IR_Builder::createReadMsgDesc(SFID sfid, uint32_t desc,
                                              G4_Operand *bti) {
  // ToDo: move extDesc into SendMsgDesc ctor
  uint32_t extDesc = G4_SendDescRaw::createExtDesc(sfid);
  return new (mem) G4_SendDescRaw(sfid, desc, extDesc, 0, SendAccess::READ_ONLY,
                                  bti, true, *this);
}

G4_SendDescRaw *IR_Builder::createWriteMsgDesc(SFID sfid, uint32_t desc,
                                               int src1Len, G4_Operand *bti) {
  // ToDo: move extDesc into SendMsgDesc ctor
  uint32_t extDesc = G4_SendDescRaw::createExtDesc(sfid, false, src1Len);
  return new (mem) G4_SendDescRaw(sfid, desc, extDesc, src1Len,
                                  SendAccess::WRITE_ONLY, bti, true, *this);
}

G4_SendDescRaw *IR_Builder::createSyncMsgDesc(SFID sfid, uint32_t desc) {
  // ToDo: move extDesc into SendMsgDesc ctor
  uint32_t extDesc = G4_SendDescRaw::createExtDesc(sfid);
  return new (mem) G4_SendDescRaw(sfid, desc, extDesc, 0,
                                  SendAccess::READ_WRITE, nullptr, true, *this);
}

G4_SendDescRaw *IR_Builder::createSampleMsgDesc(uint32_t desc, bool cps,
                                                int src1Len, G4_Operand *bti,
                                                G4_Operand *sti) {
#define CPS_LOD_COMPENSATION_ENABLE 11

  uint32_t extDesc =
      G4_SendDescRaw::createExtDesc(SFID::SAMPLER, false, src1Len);
  if (cps) {
    extDesc |= 1 << CPS_LOD_COMPENSATION_ENABLE;
  }
  return new (mem)
      G4_SendDescRaw(desc, extDesc, SendAccess::READ_ONLY, bti, sti, *this);
}

G4_Operand *IR_Builder::emitSampleIndexGE16(G4_Operand *sampler,
                                            G4_Declare *headerDecl) {
  G4_Operand *samplerIdx;

  G4_Declare *t0 = createTempVar(1, Type_UD, Any);
  G4_DstRegRegion *t0Dst = createDstRegRegion(t0, 1);
  G4_SrcRegRegion *t0Src = createSrcRegRegion(t0, getRegionScalar());

  G4_Declare *baseAdj = createTempVar(1, Type_UD, Any);
  G4_DstRegRegion *baseAdjDst = createDstRegRegion(baseAdj, 1);
  G4_SrcRegRegion *baseAdjSrc = createSrcRegRegion(baseAdj, getRegionScalar());

  G4_Declare *idxLow = createTempVar(1, Type_UD, Any);
  G4_DstRegRegion *idxLowDst = createDstRegRegion(idxLow, 1);
  G4_SrcRegRegion *idxLowSrc = createSrcRegRegion(idxLow, getRegionScalar());

  // calculate the sampler state base pointer offset based on
  // sample index, for putting to msg header M0.3
  createBinOp(G4_shr, g4::SIMD1, t0Dst, sampler, createImm(4, Type_UD),
              InstOpt_WriteEnable, true);
  createBinOp(G4_shl, g4::SIMD1, baseAdjDst, t0Src, createImm(8, Type_UD),
              InstOpt_WriteEnable, true);

  // get low 4 bits of sample index for putting into msg descriptor
  G4_SrcRegRegion *sampler2Src = createSrc(sampler->getTopDcl()->getRegVar(), 0,
                                           0, getRegionScalar(), Type_UD);
  createBinOp(G4_and, g4::SIMD1, idxLowDst, sampler2Src,
              createImm(0xf, Type_UD), InstOpt_WriteEnable, true);
  samplerIdx = idxLowSrc;

  // add the base pointer offset with r0.3 and put to M0.3
  G4_DstRegRegion *stateBaseRgn =
      createDst(headerDecl->getRegVar(), 0, 3, 1, Type_UD);
  G4_SrcRegRegion *src0 = nullptr;
  if (hasBindlessSampler() && !isBindlessSampler(sampler)) {
    src0 = createSrc(builtinSamplerHeader->getRegVar(), 0, 3, getRegionScalar(),
                     Type_UD);
  } else {
    src0 = createSrc(builtinR0->getRegVar(), 0, 3, getRegionScalar(), Type_UD);
  }
  createBinOp(G4_add, g4::SIMD1, stateBaseRgn, src0, baseAdjSrc,
              InstOpt_WriteEnable, true);

  return samplerIdx;
}

G4_INST *IR_Builder::createInst(G4_Predicate *prd, G4_opcode op,
                                G4_CondMod *mod, G4_Sat sat,
                                G4_ExecSize execSize, G4_DstRegRegion *dst,
                                G4_Operand *src0, G4_Operand *src1,
                                G4_InstOpts options, bool addToInstList) {
  vISA_ASSERT(
      op != G4_math,
      "IR_Builder::createInst should not be used to create math instructions");
  G4_INST *i = NULL;

  // ToDo: have separate functions to create call/jmp/ret
  if (G4_Inst_Table[op].instType == InstTypeFlow) {
    // TODO: remove this path
    vISA_ASSERT(!sat, "saturation not defined on branching ops");
    i = new (mem) G4_InstCF(*this, prd, op, mod, execSize, dst, src0, options);
  } else {
    i = new (mem)
        G4_INST(*this, prd, op, mod, sat, execSize, dst, src0, src1, options);
  }

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }

    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

// same as above, except we don't add it to the Builder's instList
G4_INST *IR_Builder::createInternalInst(G4_Predicate *prd, G4_opcode op,
                                        G4_CondMod *mod, G4_Sat sat,
                                        G4_ExecSize execSize,
                                        G4_DstRegRegion *dst, G4_Operand *src0,
                                        G4_Operand *src1, G4_InstOpts options) {
  vISA_ASSERT(op != G4_math, "IR_Builder::createInternalInst should not be "
                             "used to create math instructions");

  auto ii =
      createInst(prd, op, mod, sat, execSize, dst, src0, src1, options, false);

  return ii;
}

G4_INST *IR_Builder::createNop(G4_InstOpts instOpt) {
  return createInternalInst(nullptr, G4_nop, nullptr, g4::NOSAT, g4::SIMD1,
                            nullptr, nullptr, nullptr, instOpt);
}

// sync inst are always internal, so no option to append it to instList.
// Also currently don't take any InstOpt
G4_INST *IR_Builder::createSync(G4_opcode syncOp, G4_Operand *src) {
  vISA_ASSERT(G4_INST::isSyncOpcode(syncOp), "expect a sync op");
  return createInternalInst(nullptr, syncOp, nullptr, g4::NOSAT, g4::SIMD1,
                            nullptr, src, nullptr, InstOpt_NoOpt);
}

G4_INST *IR_Builder::createMov(G4_ExecSize execSize, G4_DstRegRegion *dst,
                               G4_Operand *src0, G4_InstOpts options,
                               bool appendToInstList) {
  return createMov(nullptr, execSize, dst, src0, options, appendToInstList);
}
G4_INST *IR_Builder::createMov(G4_Predicate *pred, G4_ExecSize execSize,
                               G4_DstRegRegion *dst, G4_Operand *src0,
                               G4_InstOpts options, bool appendToInstList) {
  G4_INST *newInst = nullptr;
  if (appendToInstList) {
    newInst = createInst(pred, G4_mov, nullptr, g4::NOSAT, execSize, dst, src0,
                         nullptr, options, true);
  } else {
    newInst = createInternalInst(pred, G4_mov, nullptr, g4::NOSAT, execSize,
                                 dst, src0, nullptr, options);
  }
  return newInst;
}

G4_INST *IR_Builder::createBinOp(G4_Predicate *pred, G4_opcode op,
                                 G4_ExecSize execSize, G4_DstRegRegion *dst,
                                 G4_Operand *src0, G4_Operand *src1,
                                 G4_InstOpts options, bool appendToInstList) {
  if (appendToInstList) {
    return createInst(pred, op, nullptr, g4::NOSAT, execSize, dst, src0, src1,
                      options, true);
  } else {
    return createInternalInst(pred, op, nullptr, g4::NOSAT, execSize, dst, src0,
                              src1, options);
  }
}

// mach creates both implicit acc and src using the supplied accType. AccWrCtrl
// is turned on. acc0.0 is always used
G4_INST *IR_Builder::createMach(G4_ExecSize execSize, G4_DstRegRegion *dst,
                                G4_Operand *src0, G4_Operand *src1,
                                G4_InstOpts options, G4_Type accType) {
  auto machInst = createInternalInst(nullptr, G4_mach, nullptr, g4::NOSAT,
                                     execSize, dst, src0, src1, options);
  const RegionDesc *rd =
      execSize > g4::SIMD1 ? getRegionStride1() : getRegionScalar();
  auto accSrc = createSrc(phyregpool.getAcc0Reg(), 0, 0, rd, accType);
  machInst->setImplAccSrc(accSrc);
  auto accDSt = createDst(phyregpool.getAcc0Reg(), 0, 0, 1, accType);
  machInst->setImplAccDst(accDSt);
  machInst->setOptionOn(InstOpt_AccWrCtrl);
  return machInst;
}

// macl creates an implicit src using the supplied the accType. AccWrCtrl is not
// set. acc0.0 is always used
G4_INST *IR_Builder::createMacl(G4_ExecSize execSize, G4_DstRegRegion *dst,
                                G4_Operand *src0, G4_Operand *src1,
                                G4_InstOpts options, G4_Type accType) {
  auto maclInst = createInternalInst(nullptr, G4_mach, nullptr, g4::NOSAT,
                                     execSize, dst, src0, src1, options);
  const RegionDesc *rd =
      execSize > g4::SIMD1 ? getRegionStride1() : getRegionScalar();
  auto accSrc = createSrc(phyregpool.getAcc0Reg(), 0, 0, rd, accType);
  maclInst->setImplAccSrc(accSrc);
  return maclInst;
}

G4_INST *IR_Builder::createMadm(G4_Predicate *pred, G4_ExecSize execSize,
                                G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
                                G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                                G4_InstOpts options) {
  // madm is currently only created in vISA->Gen IR translation
  return createInst(pred, G4_madm, nullptr, g4::NOSAT, execSize, dst, src0,
                    src1, src2, options, true);
}

G4_INST *IR_Builder::createIf(G4_Predicate *prd, G4_ExecSize execSize,
                              G4_InstOpts options) {
  auto inst =
      createCFInst(prd, G4_if, execSize, nullptr, nullptr, options, true);
  return inst;
}

G4_INST *IR_Builder::createElse(G4_ExecSize execSize, G4_InstOpts options) {
  auto inst =
      createCFInst(nullptr, G4_else, execSize, nullptr, nullptr, options, true);
  return inst;
}

G4_INST *IR_Builder::createEndif(G4_ExecSize execSize, G4_InstOpts options) {
  auto inst = createCFInst(nullptr, G4_endif, execSize, nullptr, nullptr,
                           options, true);
  return inst;
}

G4_INST *IR_Builder::createLabelInst(G4_Label *label, bool appendToInstList) {
  if (appendToInstList) {
    return createInst(nullptr, G4_label, nullptr, g4::NOSAT, g4::SIMD_UNDEFINED,
                      nullptr, label, nullptr, InstOpt_NoOpt, true);
  } else {
    return createInternalInst(nullptr, G4_label, nullptr, g4::NOSAT,
                              g4::SIMD_UNDEFINED, nullptr, label, nullptr, 0,
                              0);
  }
}

// jmpTarget may be either a label (direct jmp) or scalar operand (indirect jmp)
G4_INST *IR_Builder::createJmp(G4_Predicate *pred, G4_Operand *jmpTarget,
                               G4_InstOpts options, bool appendToInstList) {
  if (appendToInstList) {
    return createInst(pred, G4_jmpi, nullptr, g4::NOSAT, g4::SIMD1, nullptr,
                      jmpTarget, nullptr, options, true);
  } else {
    return createInternalInst(pred, G4_jmpi, nullptr, g4::NOSAT, g4::SIMD1,
                              nullptr, jmpTarget, nullptr, options);
  }
}

G4_INST *IR_Builder::createInternalCFInst(G4_Predicate *prd, G4_opcode op,
                                          G4_ExecSize execSize, G4_Label *jip,
                                          G4_Label *uip, G4_InstOpts options) {
  vISA_ASSERT(G4_Inst_Table[op].instType == InstTypeFlow,
              "IR_Builder::createInternalCFInst must be used with "
              "InstTypeFlow instruction class");

  auto ii = createCFInst(prd, op, execSize, jip, uip, options, false);
  return ii;
}

G4_INST *IR_Builder::createCFInst(G4_Predicate *prd, G4_opcode op,
                                  G4_ExecSize execSize, G4_Label *jip,
                                  G4_Label *uip, G4_InstOpts options,
                                  bool addToInstList) {
  vISA_ASSERT(G4_Inst_Table[op].instType == InstTypeFlow,
              "IR_Builder::createCFInst must be used with InstTypeFlow "
              "instruction class");

  G4_InstCF *ii =
      new (mem) G4_InstCF(*this, prd, op, execSize, jip, uip, options);

  if (addToInstList) {
    ii->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      ii->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(ii);
  }

  instAllocList.push_back(ii);

  return ii;
}

G4_INST *IR_Builder::createDpasInst(G4_opcode opc, G4_ExecSize execSize,
                                    G4_DstRegRegion *dst, G4_Operand *src0,
                                    G4_Operand *src1, G4_Operand *src2,
                                    G4_Operand *src3, G4_Operand *src4,
                                    G4_InstOpts options, GenPrecision A,
                                    GenPrecision W, uint8_t D, uint8_t C,
                                    bool addToInstList) {
  vASSERT(!src3 || opc == G4_bdpas);
  vASSERT(!src4 || opc == G4_bdpas);
  G4_INST *i = new (mem) G4_InstDpas(*this, opc, execSize, dst, src0, src1,
                                     src2, src3, src4, options, A, W, D, C);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);
    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createInternalDpasInst(
    G4_opcode opc, G4_ExecSize execSize, G4_DstRegRegion *dst, G4_Operand *src0,
    G4_Operand *src1, G4_Operand *src2, G4_InstOpts options, GenPrecision A,
    GenPrecision W, uint8_t D, uint8_t C, G4_Operand *src3, G4_Operand *src4) {
  return createDpasInst(opc, execSize, dst, src0, src1, src2, src3, src4,
                        options, A, W, D, C, false);
}

G4_INST *IR_Builder::createBfnInst(uint8_t booleanFuncCtrl, G4_Predicate *prd,
                                   G4_CondMod *mod, G4_Sat sat,
                                   G4_ExecSize execSize, G4_DstRegRegion *dst,
                                   G4_Operand *src0, G4_Operand *src1,
                                   G4_Operand *src2, G4_InstOpts options,
                                   bool addToInstList) {
  G4_INST *i = new (mem) G4_InstBfn(*this, prd, mod, sat, execSize, dst, src0,
                                    src1, src2, options, booleanFuncCtrl);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createInternalBfnInst(
    uint8_t booleanFuncCtrl, G4_Predicate *prd, G4_CondMod *mod, G4_Sat sat,
    G4_ExecSize execSize, G4_DstRegRegion *dst, G4_Operand *src0,
    G4_Operand *src1, G4_Operand *src2, G4_InstOpts options) {
  auto ii = createBfnInst(booleanFuncCtrl, prd, mod, sat, execSize, dst, src0,
                          src1, src2, options, false);

  return ii;
}

// todo: once all s0 creation is  moved to post-RA, remove the
// createSurfaceMove* functions from buildIR
G4_SrcRegRegion* IR_Builder::setupIndirectDescriptor(G4_Operand *val)
{
  if (isNullZero(val)) {
    // bail out if the value is 0; HW will treat IND0 or IND1 being
    // absent as 0's.
    return nullptr;
  }

  vISA_ASSERT(IS_INT(val->getType()), "invalid type for surface mov");
  G4_Type movDstType = IS_SIGNED_INT(val->getType()) ? Type_Q : Type_UQ;

  // HW requires that in moves to s0 the dst/src types match
  //   (W) mov (1) s0.#:DT    val:ST   // with DT /= ST
  // CASES:
  //    1. the source value is immediate (non-relocation)
  //       ==> zero/sign-extend immval to DT
  //
  //    2. the types are the same size but mismatch in signedness
  //       ==> flip types to match
  //       (W) mov (1) s0.#:ST    val:ST
  //       No change in the bits (will be raw mov); so this works
  //
  //    3. worst case; we need actual computation (sign/zero extension)
  //       use a mov op
  //
  // Relocation immediates fall in cases 2 and 3.
  if (movDstType != val->getType()) {
    if (val->isImm() && !val->isRelocImm()) {
      // 1. just move
      val = createImm(val->asImm()->getImm(), movDstType);
    } else {
      // 3. otherwise, we actually need to zero or sign extend or truncate val
      // use the a regular INT pipe operation for this
      auto tvDcl =
        createTempVar(1, movDstType, Get_G4_SubRegAlign_From_Type(movDstType));
      G4_DstRegRegion *tvDst = createDstRegRegion(tvDcl, 1);
      (void)createMov(g4::SIMD1, tvDst, val, InstOpt_WriteEnable, true);
      val = createSrcRegRegion(tvDcl, getRegionScalar());
    }
  }
  // IGC will fold the Blender state index (BSI, also known as render target
  // index in IGC) into the surface base address;
  return static_cast<G4_SrcRegRegion*>(val);
}
G4_SrcRegRegion *IR_Builder::createSurfaceMoveToNextS0(
  SFID sfid, G4_Operand *val, bool skipIfNullZero)
{
  bool s0IsA32 = sfid == SFID::SLM || sfid == SFID::URB;
  return createSurfaceMoveToNextS0(val, s0IsA32, skipIfNullZero);
}
G4_SrcRegRegion *IR_Builder::createSurfaceMoveToNextS0(
    G4_Operand *val, bool s0isA32, bool skipIfNullZero)
{
  // compute the next offset in s0 that we can use for sendg
  int s0qw = nextSurfaceS0QW++;
  if (nextSurfaceS0QW == getScalarRegisterSizeInBytes() / 8) {
    nextSurfaceS0QW = FIRST_SURFACE_S0_QW; // skip first Gather Send
  }
  return createSurfaceMoveToS0(val, s0qw, s0isA32, skipIfNullZero);
}
G4_SrcRegRegion *IR_Builder::createSurfaceMoveToS0(
  G4_Operand *val, int s0qw, bool s0isA32, bool skipIfNullZero)
{
  vISA_ASSERT((unsigned)s0qw < getScalarRegisterSizeInBytes() / 8,
              "qw is out of bounds");
  vISA_ASSERT(!val || IS_INT(val->getType()), "invalid type for surface mov");
  G4_Type movDstType = val && IS_SIGNED_INT(val->getType()) ? Type_Q : Type_UQ;

  if (skipIfNullZero && isNullZero(val)) {
    // bail out if the value is 0; HW will treat IND0 or IND1 being
    // absent as 0's.
    return nullptr;
  } else if (val == nullptr) {
    // HW should treat non-existent IND0 or IND1 as 0's on the sideband
    // but skipIfNullZero = false overrides the behavior and forces the
    // move to an s0 register.
    val = createImm(0, movDstType);
  }

  // HW requires that in moves to s0 the dst/src types match
  //   (W) mov (1) s0.#:DT    val:ST   // with DT /= ST
  // CASES:
  //    1. the source value is immediate (non-relocation)
  //       ==> zero/sign-extend immval to DT
  //
  //    2. the types are the same size but mismatch in signedness
  //       ==> flip types to match
  //       (W) mov (1) s0.#:ST    val:ST
  //       No change in the bits (will be raw mov); so this works
  //
  //    3. worst case; we need actual computation (sign/zero extension)
  //       use a mov op
  //
  // Relocation immediates fall in cases 2 and 3.
  if (movDstType != val->getType()) {
    if (val->isImm() && !val->isRelocImm()) {
      // 1. just move
      val = createImm(val->asImm()->getImm(), movDstType);
    } else if (TypeSize(movDstType) == TypeSize(val->getType())){
      // 2. just flip dstType to srcType; bits are bits
      movDstType = val->getType();
    } else {
      // 3. otherwise, we actually need to zero or sign extend or truncate val
      // use the a regular INT pipe operation for this
      auto tvDcl =
        createTempVar(1, movDstType, Get_G4_SubRegAlign_From_Type(movDstType));
      G4_DstRegRegion *tvDst = createDstRegRegion(tvDcl, 1);
      (void)createMov(g4::SIMD1, tvDst, val, InstOpt_WriteEnable, true);
      val = createSrcRegRegion(tvDcl, getRegionScalar());
    }
  }

  G4_DstRegRegion *dstTmpS0 = createS0Dst(s0qw, movDstType);
  (void)createMov(g4::SIMD1, dstTmpS0, val, InstOpt_WriteEnable, true);
  G4_SrcRegRegion *srcTmpS0 = createS0Src(s0qw);
  return srcTmpS0;
}
G4_DstRegRegion *IR_Builder::createS0Dst(int s0qw, G4_Type movDstType) {
  return createDst(builtinS0->getRegVar(), 0,
                   8 * s0qw / TypeSize(movDstType), 1, movDstType);
}
G4_SrcRegRegion *IR_Builder::createS0Src(int s0qw) {
  return createSrc(builtinS0->getRegVar(), 0, s0qw, getRegionScalar(), Type_UQ);
}
// scratch surfaces, write the content of T251 to extended message descriptor
// exdesc holds the value of the extended message descriptor for bit [0:11]
// add (1) a0.2<1>:ud T251<1>:ud exDesc:ud {NoMask}
// returns a0.2<0;1,0>:ud
G4_SrcRegRegion *IR_Builder::createScratchExDesc(uint32_t exdesc) {
  // virtual var for each exdesc
  G4_SrcRegRegion *T251 =
      createSrcRegRegion(builtinScratchSurface, getRegionScalar());
  const char *buf = getNameString(20, "ExDesc%d", num_temp_dcl++);
  G4_Declare *exDescDecl = createDeclare(buf, G4_ADDRESS, 1, 1, Type_UD);
  exDescDecl->setSubRegAlign(Four_Word);

  G4_DstRegRegion *dst = createDstRegRegion(exDescDecl, 1);
  if (useNewExtDescFormat()) {
    createMov(g4::SIMD1, dst, T251, InstOpt_WriteEnable, true);
  } else {
    createBinOp(G4_add, g4::SIMD1, dst, T251, createImm(exdesc, Type_UD),
                InstOpt_WriteEnable, true);
  }
  if (getOption(vISA_CopyA0ToDBG0)) {
    G4_SrcRegRegion *srcA0 = createSrcRegRegion(exDescDecl, getRegionScalar());
    G4_DstRegRegion *dstDbg0 =
        createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
    createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
  }
  return createSrcRegRegion(exDescDecl, getRegionScalar());
}

G4_INST *IR_Builder::createInst(G4_Predicate *prd, G4_opcode op,
                                G4_CondMod *mod, G4_Sat sat,
                                G4_ExecSize execSize, G4_DstRegRegion *dst,
                                G4_Operand *src0, G4_Operand *src1,
                                G4_Operand *src2, G4_InstOpts options,
                                bool addToInstList) {
  vISA_ASSERT(op != G4_math && G4_Inst_Table[op].instType != InstTypeFlow,
              "IR_Builder::createInst should not be used to create math/CF "
              "instructions");

  if (op == G4_madw) {
    vISA_ASSERT(getPlatform() >= Xe_PVC || execSize != g4::SIMD32,
                "SIMD32 is not supported on this platform for madw");
  }

  G4_INST *i = NULL;

  i = new (mem) G4_INST(*this, prd, op, mod, sat, execSize, dst, src0, src1,
                        src2, options);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }

    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

// same as above, except we don't add it to the Builder's instList
G4_INST *IR_Builder::createInternalInst(G4_Predicate *prd, G4_opcode op,
                                        G4_CondMod *mod, G4_Sat sat,
                                        G4_ExecSize execSize,
                                        G4_DstRegRegion *dst, G4_Operand *src0,
                                        G4_Operand *src1, G4_Operand *src2,
                                        G4_InstOpts options) {
  auto ii = createInst(prd, op, mod, sat, execSize, dst, src0, src1, src2,
                       options, false);

  return ii;
}

G4_InstSend *IR_Builder::createSendInst(
    G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize,
    G4_DstRegRegion *postDst, G4_SrcRegRegion *currSrc, G4_Operand *msg,
    G4_InstOpts options, G4_SendDescRaw *msgDesc, bool addToInstList) {

  vISA_ASSERT(msgDesc != nullptr, "msgDesc must not be null");
  G4_InstSend *m = new (mem) G4_InstSend(*this, prd, op, execSize, postDst,
                                         currSrc, msg, options, msgDesc);

  if (addToInstList) {
    m->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      m->setLocation(allocateMDLocation(curLine, curFile));
    }

    instList.push_back(m);
  }

  instAllocList.push_back(m);

  return m;
}

G4_InstSend *IR_Builder::createInternalSendInst(
    G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize,
    G4_DstRegRegion *postDst, G4_SrcRegRegion *currSrc, G4_Operand *msg,
    G4_InstOpts options, G4_SendDescRaw *msgDesc) {
  auto ii = createSendInst(prd, op, execSize, postDst, currSrc, msg, options,
                           msgDesc, false);

  return ii;
}

//
// Create a split send (sends) instruction
// sends (size) dst src0 src1 exDesc msgDesc
//

G4_InstSend *IR_Builder::createSendgInst(
  G4_Predicate *prd,
  G4_opcode op,
  G4_ExecSize execSize,
  G4_DstRegRegion *dst,
  G4_SrcRegRegion *src0addr,
  G4_SrcRegRegion *src1data,
  G4_SrcRegRegion *src2ind0, // scalar operand 0 IND0
  G4_SrcRegRegion *src3ind1, // scalar operand 1 IND1
  G4_SendgDesc *msgDesc,
  G4_InstOpts options,
  bool addToInstList)
{
  vISA_ASSERT(op == G4_sendg || op == G4_sendgc,
    "invalid op for helper function");

  if (!src1data) {
    vISA_ASSERT(msgDesc->getSrc1LenRegs() == 0,
                "src1 length must be 0 if it is null");
    src1data = createNullSrc(Type_UD);
  }

  G4_InstSend *m =
      new (mem) G4_InstSend(*this, prd, op, execSize, dst, src0addr, src1data,
                            src2ind0, src3ind1, options, msgDesc);

  if (addToInstList) {
    m->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      m->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(m);
  }

  instAllocList.push_back(m);

  return m;
}

G4_InstSend *IR_Builder::createSplitSendInst(
    G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize, G4_DstRegRegion *dst,
    G4_SrcRegRegion *src0, // can be header
    G4_SrcRegRegion *src1,
    G4_Operand *msg, // msg descriptor: imm or vec
    G4_InstOpts options, G4_SendDescRaw *msgDesc,
    G4_Operand *src3, // ext msg desciptor: imm or vec
    bool addToInstList) {

  if (!src1) {
    // src1 may be null if we need to force generate split send (e.g., for
    // bindless surfaces)
    vISA_ASSERT(msgDesc->getSrc1LenRegs() == 0,
                "src1 length must be 0 if it is null");
    src1 = createNullSrc(Type_UD);
  }
  if (!src3) {
    src3 = createImm(msgDesc->getExtendedDesc(), Type_UD);
  }

  G4_InstSend *m = new (mem) G4_InstSend(*this, prd, op, execSize, dst, src0,
                                         src1, msg, src3, options, msgDesc);

  if (addToInstList) {
    m->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      m->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(m);
  }

  instAllocList.push_back(m);

  return m;
}

G4_InstSend *IR_Builder::createInternalSplitSendInst(
    G4_ExecSize execSize, G4_DstRegRegion *dst,
    G4_SrcRegRegion *src0, // can be header
    G4_SrcRegRegion *src1,
    G4_Operand *msg, // msg descriptor: imm or vec
    G4_InstOpts options, G4_SendDescRaw *msgDesc,
    G4_Operand *src3) // ext msg desciptor: imm or vec)
{
  auto ii = createSplitSendInst(nullptr, G4_sends, execSize, dst, src0, src1,
                                msg, options, msgDesc, src3, false);

  return ii;
}

//
// Math instruction is like a generic one except:
// -- it takes a G4_MathOp to specify the function control
// -- conditional modifier is not allowed
// -- there are additional restrictions on dst/src regions that will be checked
// in HW conformity
//
G4_INST *IR_Builder::createMathInst(G4_Predicate *prd, G4_Sat sat,
                                    G4_ExecSize execSize, G4_DstRegRegion *dst,
                                    G4_Operand *src0, G4_Operand *src1,
                                    G4_MathOp mathOp, G4_InstOpts options,
                                    bool addToInstList) {
  G4_INST *i = new (mem) G4_InstMath(*this, prd, G4_math, NULL, sat, execSize,
                                     dst, src0, src1, options, mathOp);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createInternalMathInst(
    G4_Predicate *prd, G4_Sat sat, G4_ExecSize execSize, G4_DstRegRegion *dst,
    G4_Operand *src0, G4_Operand *src1, G4_MathOp mathOp, G4_InstOpts options) {
  auto ii = createMathInst(prd, sat, execSize, dst, src0, src1, mathOp, options,
                           false);
  return ii;
}

G4_INST *IR_Builder::createIntrinsicInst(G4_Predicate *prd, Intrinsic intrinId,
                                         G4_ExecSize size, G4_DstRegRegion *dst,
                                         G4_Operand *src0, G4_Operand *src1,
                                         G4_Operand *src2, G4_InstOpts options,
                                         bool addToInstList) {
  G4_INST *i = nullptr;

  if (intrinId == Intrinsic::Spill)
    i = new (mem) G4_SpillIntrinsic(*this, prd, intrinId, size, dst, src0, src1,
                                    src2, options);
  else if (intrinId == Intrinsic::Fill)
    i = new (mem) G4_FillIntrinsic(*this, prd, intrinId, size, dst, src0, src1,
                                   src2, options);
  else
    i = new (mem) G4_InstIntrinsic(*this, prd, intrinId, size, dst, src0, src1,
                                   src2, options);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }

    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createInternalIntrinsicInst(
    G4_Predicate *prd, Intrinsic intrinId, G4_ExecSize execSize,
    G4_DstRegRegion *dst, G4_Operand *src0, G4_Operand *src1, G4_Operand *src2,
    G4_InstOpts options) {
  auto ii = createIntrinsicInst(prd, intrinId, execSize, dst, src0, src1, src2,
                                options, false);

  return ii;
}

G4_INST *IR_Builder::createIntrinsicAddrMovInst(
    Intrinsic intrinId, G4_DstRegRegion *dst, G4_Operand *src0,
    G4_Operand *src1, G4_Operand *src2, G4_Operand *src3, G4_Operand *src4,
    G4_Operand *src5, G4_Operand *src6, G4_Operand *src7, bool addToInstList) {
  G4_INST *i = nullptr;
  vISA_ASSERT(intrinId == Intrinsic::PseudoAddrMov ||
                  intrinId == Intrinsic::PseudoAddrMovW,
              "expect pseudo_mov op");

  i = new (mem) G4_PseudoAddrMovIntrinsic(*this, intrinId, dst, src0, src1,
                                          src2, src3, src4, src5, src6, src7);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }

    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_MathOp IR_Builder::Get_MathFuncCtrl(ISA_Opcode op, G4_Type type) {
  switch (op) {
  case ISA_LOG:
    return MATH_LOG;
  case ISA_MOD: // remainder of IDIV
    return MATH_INT_DIV_REM;
  case ISA_POW:
    return MATH_POW;
  case ISA_SIN:
    return MATH_SIN;
  case ISA_COS:
    return MATH_COS;
  case ISA_SQRT:
    return MATH_SQRT;
  case ISA_RSQRT:
    return MATH_RSQ;
  case ISA_INV:
    return MATH_INV;
  case ISA_DIV:
    return (IS_FTYPE(type) || IS_HFTYPE(type) || IS_BFTYPE(type))
               ? MATH_FDIV
               : MATH_INT_DIV_QUOT;
  case ISA_EXP:
    return MATH_EXP;
  case ISA_TANH:
    return MATH_TANH;
  case ISA_SIGM:
    return MATH_SIGM;
  default:
    vISA_ASSERT_UNREACHABLE("Illegal math opcode.");
    return MATH_RESERVED;
  }
}

// After building IR total number number of rows required
// for arg and retvar become known, so resize the pre-defined
// vars here to the max required in current compilation unit.
void IR_Builder::resizePredefinedStackVars() {
  getStackCallArg()->resizeNumRows(this->getArgSize());
  getStackCallRet()->resizeNumRows(this->getRetVarSize());
}

G4_Operand *IR_Builder::duplicateOpndImpl(G4_Operand *opnd) {
  if (!opnd || opnd->isImm())
    return opnd;
  if (opnd->isSrcRegRegion()) {
    return createSrcRegRegion(*(opnd->asSrcRegRegion()));
  } else if (opnd->isDstRegRegion()) {
    return createDstRegRegion(*(opnd->asDstRegRegion()));
  } else if (opnd->isPredicate()) {
    return createPredicate(*(opnd->asPredicate()));
  } else if (opnd->isCondMod()) {
    return createCondMod(*(opnd->asCondMod()));
  } else {
    return opnd;
  }
}

/*
 * Create send instruction for specified GenX architecture.
 * bti: surface id
 * sti: sampler id
 */
G4_InstSend *IR_Builder::createSendInst(
    G4_Predicate *pred, G4_DstRegRegion *postDst, G4_SrcRegRegion *payload,
    unsigned regs2snd, unsigned regs2rcv, G4_ExecSize execSize, unsigned fc,
    SFID tf_id, bool header_present, SendAccess access, G4_Operand *bti,
    G4_Operand *sti, G4_InstOpts options, bool is_sendc) {
  G4_SendDescRaw *msgDesc =
      createSendMsgDesc(fc, regs2rcv, regs2snd, tf_id, 0, 0, access, bti, sti);

  msgDesc->setHeaderPresent(header_present);

  return createSendInst(pred, postDst, payload, execSize, msgDesc, options,
                        is_sendc);
}

// bindless surfaces, write the content of T252 to extended message descriptor
// exdesc holds the value of the extended message descriptor for bit [0:11]
// add (1) a0.2<1>:ud T252<1>:ud exDesc:ud {NoMask}
//  returns a0.2<0;1,0>:ud
G4_SrcRegRegion *IR_Builder::createBindlessExDesc(uint32_t exdesc) {
  G4_InstOpts dbgOpt = m_options->getOption(vISA_markSamplerMoves)
                           ? InstOpt_BreakPoint
                           : InstOpt_NoOpt;
  // virtual var for each exdesc
  G4_SrcRegRegion *T252 = createSrcRegRegion(builtinT252, getRegionScalar());
  const char *buf = getNameString(20, "ExDesc%d", num_temp_dcl++);
  G4_Declare *exDescDecl = createDeclare(buf, G4_ADDRESS, 1, 1, Type_UD);
  exDescDecl->setSubRegAlign(Four_Word);
  G4_DstRegRegion *dst = createDstRegRegion(exDescDecl, 1);
  if (useNewExtDescFormat()) {
    createMov(g4::SIMD1, dst, T252, InstOpt_WriteEnable | dbgOpt, true);
  } else {
    createBinOp(G4_add, g4::SIMD1, dst, T252, createImm(exdesc, Type_UD),
                InstOpt_WriteEnable, true);
  }
  if (getOption(vISA_CopyA0ToDBG0)) {
    G4_SrcRegRegion *srcA0 = createSrcRegRegion(exDescDecl, getRegionScalar());
    G4_DstRegRegion *dstDbg0 =
        createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
    createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable | dbgOpt, true);
  }
  return createSrcRegRegion(exDescDecl, getRegionScalar());
}

/*
 *
 *  this does two things:
 *  -- If send has exec size 16, its destination must have Type W.
 *  -- avoid using Q/UQ type on CHV/BXT
 */
static void fixSendDstType(G4_DstRegRegion *dst, G4_ExecSize execSize,
                           const IR_Builder &builder) {
  vISA_ASSERT(dst->getRegAccess() == Direct,
              "Send dst must be a direct operand");

  vISA_ASSERT(dst->getSubRegOff() == 0,
              "dst may not have a non-zero subreg offset");

  // normally we should create a new alias for dst's declare, but since it's a
  // send type mismatch between operand and decl should not matter
  if (execSize == g4::SIMD16 && dst->getType() != Type_W &&
      dst->getType() != Type_UW) {
    dst->setType(builder, Type_W);
  }

  if (dst->getType() == Type_HF) {
    dst->setType(builder, Type_W);
  }
}

G4_InstSend *IR_Builder::createSendInst(G4_Predicate *pred,
                                        G4_DstRegRegion *postDst,
                                        G4_SrcRegRegion *payload,
                                        G4_ExecSize execsize,
                                        G4_SendDescRaw *msgDesc,
                                        G4_InstOpts option, bool is_sendc) {
  G4_opcode send_opcode = is_sendc ? G4_sendc : G4_send;

  fixSendDstType(postDst, execsize, *this);

  uint32_t desc = msgDesc->getDesc();
  G4_Operand *bti = msgDesc->getBti();
  G4_Operand *sti = msgDesc->getSti();
  G4_Operand *descOpnd = NULL;

  bool needSamplerMove = sti && !sti->isImm() && !isBindlessSampler(sti);

  if ((bti && !bti->isImm()) || needSamplerMove) {
    // use a0.0 directly
    G4_DstRegRegion *addr_dst_opnd = createDstRegRegion(builtinA0, 1);

    if (bti && !bti->isImm()) {
      // add (1) a0.0:ud bti:ud desc:ud
      //  create source for bti
      createBinOp(G4_add, g4::SIMD1, addr_dst_opnd, bti,
                  createImm(desc, Type_UD), InstOpt_WriteEnable, true);
    }

    if (needSamplerMove) {
      G4_Declare *dcl1 = createTempVar(1, Type_UD, Any);
      G4_DstRegRegion *tmp_dst_opnd = createDstRegRegion(dcl1, 1);

      createBinOp(G4_shl, g4::SIMD1, tmp_dst_opnd, sti, createImm(8, Type_UD),
                  InstOpt_WriteEnable, true);

      G4_SrcRegRegion *tmp_src_opnd =
          createSrcRegRegion(dcl1, getRegionScalar());

      if (!bti || bti->isImm()) {
        createBinOp(G4_add, g4::SIMD1, addr_dst_opnd, tmp_src_opnd,
                    createImm(desc, Type_UD), InstOpt_WriteEnable, true);
      } else {
        G4_SrcRegRegion *addr_src_opnd =
            createSrcRegRegion(builtinA0, getRegionScalar());

        createBinOp(G4_add, g4::SIMD1, duplicateOperand(addr_dst_opnd),
                    addr_src_opnd, tmp_src_opnd, InstOpt_WriteEnable, true);
      }
    }

    if (getOption(vISA_CopyA0ToDBG0)) {
      G4_SrcRegRegion *srcA0 = createSrcRegRegion(builtinA0, getRegionScalar());
      G4_DstRegRegion *dstDbg0 =
          createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
      createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
    }
    descOpnd = createSrcRegRegion(builtinA0, getRegionScalar());
  } else {
    descOpnd = createImm(desc, Type_UD);
  }

  return createSendInst(pred, send_opcode, execsize, postDst, payload, descOpnd,
                        option, msgDesc, true);
}

/*
 * Create split send instruction for specified GenX architecture.
 * bti: surface id
 * sti: sampler id
 * Gen9: sends (execsize)     dst,  src1,  src2,  ex_desc,  desc
 */
G4_InstSend *IR_Builder::createSplitSendInst(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src1,
    unsigned regs2snd1, G4_SrcRegRegion *src2, unsigned regs2snd2,
    unsigned regs2rcv, G4_ExecSize execSize, unsigned fc, SFID tf_id,
    bool header_present, SendAccess access, G4_Operand *bti, G4_Operand *sti,
    G4_InstOpts options, bool is_sendc) {
  G4_SendDescRaw *msgDesc = createSendMsgDesc(fc, regs2rcv, regs2snd1, tf_id,
                                              regs2snd2, 0, access, bti, sti);

  msgDesc->setHeaderPresent(header_present);

  return createSplitSendInst(pred, dst, src1, src2, execSize, msgDesc, options,
                             is_sendc);
}

// desc, if indirect, is constructed from the BTI/STI values in msgDesc and is
// always a0.0
G4_InstSend *
IR_Builder::createSplitSendInst(G4_Predicate *pred, G4_DstRegRegion *dst,
                                G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                                G4_ExecSize execsize, G4_SendDescRaw *msgDesc,
                                G4_InstOpts option, bool is_sendc) {
  G4_opcode send_opcode = is_sendc ? G4_sendsc : G4_sends;

  fixSendDstType(dst, execsize, *this);

  uint32_t desc = msgDesc->getDesc();
  uint32_t exdesc = msgDesc->getExtendedDesc();
  G4_Operand *bti = msgDesc->getBti();
  G4_Operand *sti = msgDesc->getSti();

  G4_Operand *descOpnd = NULL;
  G4_SrcRegRegion *extDescOpnd = nullptr;

  bool doAlignBindlessSampler =
      alignBindlessSampler() && sti && isBindlessSampler(sti);
  bool needsSamplerMove = (sti && !sti->isImm() && !isBindlessSampler(sti)) ||
                          doAlignBindlessSampler;

  bool needsSurfaceMove = false;
  bool needsA0ExDesc = false;

  if (bti && bti->isSrcRegRegion()) {
    if (isBindlessSurface(bti)) {
      needsA0ExDesc = true;
      // set T252 as BTI
      if ((desc & 0xFF) != PREDEF_SURF_252) {
        desc = (desc & ~0xFF) | PREDEF_SURF_252;
      }
    } else if (isScratchSpace(bti)) {
      // use BTI 251
      needsA0ExDesc = true;
      desc = (desc & ~0xFF) | 251;
    } else {
      needsSurfaceMove = true;
    }
  }

  if (needsSurfaceMove) {
    // add (1) a0.0:ud bti:ud desc:ud
    G4_DstRegRegion *addrDstOpnd = createDstRegRegion(builtinA0, 1);

    createBinOp(G4_add, g4::SIMD1, addrDstOpnd, bti, createImm(desc, Type_UD),
                InstOpt_WriteEnable, true);
  }

  if (needsSamplerMove) {
    G4_Declare *dcl1 = createTempVar(1, Type_UD, Any);

    if (doAlignBindlessSampler) {
      // check if address is 32-byte aligned
      // use STI = 0 for 32-byte aligned address, STI = 1 otherwise
      // (W) and (1) (nz)f0.0 null S31 0x10:uw
      G4_Declare *tmpFlag = createTempFlag(1);
      G4_CondMod *condMod = createCondMod(Mod_nz, tmpFlag->getRegVar(), 0);
      createInst(nullptr, G4_and, condMod, g4::NOSAT, g4::SIMD1,
                 createNullDst(Type_UD),
                 createSrcRegRegion(*(sti->asSrcRegRegion())),
                 createImm(0x10, Type_UW), InstOpt_WriteEnable, true);
      // (W) (f0.0) sel (1) tmp:ud 0x100 0x0
      G4_Predicate *pred =
          createPredicate(PredState_Plus, tmpFlag->getRegVar(), 0);
      createInst(pred, G4_sel, nullptr, g4::NOSAT, g4::SIMD1,
                 createDstRegRegion(dcl1, 1), createImm(0x100, Type_UW),
                 createImm(0x0, Type_UW), InstOpt_WriteEnable, true);
    } else {
      // shl (1) tmp:ud sti:ud 0x8:uw
      G4_DstRegRegion *tmpDstOpnd = createDstRegRegion(dcl1, 1);
      createBinOp(G4_shl, g4::SIMD1, tmpDstOpnd, sti, createImm(8, Type_UD),
                  InstOpt_WriteEnable, true);
    }

    G4_SrcRegRegion *tmpSrcOpnd = createSrcRegRegion(dcl1, getRegionScalar());
    G4_DstRegRegion *addrDstOpnd = createDstRegRegion(builtinA0, 1);
    if (!needsSurfaceMove) {
      // add (1) a0.0 tmp:ud desc:ud
      createBinOp(G4_add, g4::SIMD1, addrDstOpnd, tmpSrcOpnd,
                  createImm(desc, Type_UD), InstOpt_WriteEnable, true);
    } else {
      // add (1) a0.0 a0.0:ud tmp:ud
      G4_SrcRegRegion *addrSrcOpnd =
          createSrcRegRegion(builtinA0, getRegionScalar());
      createBinOp(G4_add, g4::SIMD1, addrDstOpnd, addrSrcOpnd, tmpSrcOpnd,
                  InstOpt_WriteEnable, true);
    }
  }

  if (needsSurfaceMove || needsSamplerMove) {
    if (getOption(vISA_CopyA0ToDBG0)) {
      G4_SrcRegRegion *srcA0 = createSrcRegRegion(builtinA0, getRegionScalar());
      G4_DstRegRegion *dstDbg0 =
          createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
      createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
    }

    descOpnd = createSrcRegRegion(builtinA0, getRegionScalar());
  } else {
    descOpnd = createImm(desc, Type_UD);
  }

  if (needsA0ExDesc) {
    extDescOpnd = isBindlessSurface(bti) ? createBindlessExDesc(exdesc)
                                         : createScratchExDesc(exdesc);
  } else {
    // do nothing as the extended msg desc will just be a null operand
  }

  return createSplitSendInst(pred, send_opcode, execsize, dst, src1, src2,
                             descOpnd, option, msgDesc, extDescOpnd, true);
}

G4_SendDescRaw *IR_Builder::createLscMsgDesc(
    LSC_OP op, LSC_SFID lscSfid, VISA_Exec_Size execSizeEnum,
    LSC_CACHE_OPTS cacheOpts, LSC_ADDR addr, LSC_DATA_SHAPE shape,
    G4_Operand *surface, uint32_t dstLen, uint32_t addrRegs,
    LdStAttrs otherAttrs) {
  //   Desc[5:0] = OPCODE {LOAD{_BLOCK,_QUAD},STORE{_BLOCK,_QUAD},ATOMIC*}
  //   Desc[8:7] = addr size
  //   Desc[11:9] = data size
  //   Desc[15:12] = data vector size (or cmask if *_QUAD)
  //   Desc[19:17] = caching controls (see the table for allowable combinations)
  //   Desc[30:29] = addr model (BTI = 3, SS = 2, BSS = 1, FLAT = 0)
  int status = VISA_SUCCESS;
  uint32_t desc = 0;
  uint32_t exDesc = 0;
  const auto opInfo = LscOpInfoGet(op);
  vISA_ASSERT(!opInfo.isBlock2D(), "block2d has a different layout");
  desc |= opInfo.encoding; // Desc[5:0]

  lscEncodeAddrSize(addr.size, desc, status); // Desc[8:7]

  int dataSizeBits = lscEncodeDataSize(shape.size, desc, status); // Desc[11:9]

  // Desc[15:12]
  int vecSize; // definitely assigned
  if (!opInfo.hasChMask()) {
    vecSize = lscEncodeDataElems(shape.elems, desc, status);
    lscEncodeDataOrder(shape.order, desc, status);
  } else {
    vISA_ASSERT(shape.chmask != 0, "channel mask must not be empty");
    vecSize = 0;
    if (shape.chmask & LSC_DATA_CHMASK_X) {
      desc |= 1 << 12;
      vecSize++;
    }
    if (shape.chmask & LSC_DATA_CHMASK_Y) {
      desc |= 1 << 13;
      vecSize++;
    }
    if (shape.chmask & LSC_DATA_CHMASK_Z) {
      desc |= 1 << 14;
      vecSize++;
    }
    if (shape.chmask & LSC_DATA_CHMASK_W) {
      desc |= 1 << 15;
      vecSize++;
    }
  }

  lscEncodeCachingOpts(opInfo, cacheOpts, desc, status); // Desc[19:17]
  lscEncodeAddrType(addr.type, desc, status);            // Desc[30:29]

  desc |= dstLen << 20;   // Desc[24:20]  dst len
  desc |= addrRegs << 25; // Desc[29:25]  src0 len

  // promote any immediate surface to the extended descriptor
  // ExDesc[31:12]
  if (surface && surface->isImm()) {
    auto surfaceImm = (uint32_t)surface->asImm()->getImm();
    if (addr.type == LSC_ADDR_TYPE_BTI) {
      // promote the immediate BTI to the descriptor
      exDesc |= surfaceImm << 24;
      surface = nullptr;
    } else if (addr.type == LSC_ADDR_TYPE_ARG) {
      vISA_ASSERT(false,
                  "caller should have converted LSC_ADDR_TYPE_ARG to ...BTI");
    } else if (addr.type == LSC_ADDR_TYPE_BSS ||
               addr.type == LSC_ADDR_TYPE_SS) {
      if ((surfaceImm & 0x3FF) == 0) {
        exDesc |= surfaceImm;
        surface = nullptr;
      }
    } else {
      // flat address type
      vISA_ASSERT(
          surface->isNullReg() || surfaceImm == PREDEFINED_SURFACE_SLM ||
              surfaceImm == PREDEFINED_SURFACE_T255, // not sure what's up here
          "flat address type must have null reg (or 0)");
      surface = nullptr;
    }
  }

  uint32_t exDescImmOff = 0;
  if (lscTryPromoteImmOffToExDesc(op, lscSfid, addr, dataSizeBits / 8, surface,
                                  exDesc, exDescImmOff)) {
    addr.immOffset = 0;
  }

  vISA_ASSERT(addr.immOffset == 0, "invalid address immediate offset");

  SFID sfid = LSC_SFID_To_SFID(lscSfid);

  const unsigned execSize = Get_VISA_Exec_Size(execSizeEnum);
  int src1Len = 0;
  uint32_t dataRegs = 1;
  [[maybe_unused]] bool isBlock2D =
      op == LSC_OP::LSC_LOAD_BLOCK2D || op == LSC_OP::LSC_STORE_BLOCK2D;
  vISA_ASSERT(!isBlock2D, "block2d not implemented yet");

  if (shape.order == LSC_DATA_ORDER_NONTRANSPOSE) {
    // Non-transpose case is the typical case.
    //
    // ceil[ SIMT32*dataSize(b)/512(b/REG) ] * vecSize
    //   units = (b/b*REG) = REG
    dataRegs =
        std::max<uint32_t>(1, execSize * dataSizeBits / 8 / getGRFSize()) *
        vecSize;
  } else { // if (shape.transpose == LSC_DATA_TRANSPOSE) {
           // The transpose case is a little odder
           // So the data size is the SIMD size (ExecSize) times the number of
           // registers consumed by each vector sequence (always a full
           // register number per seq).
    uint32_t regsPerVec = vecSize * dataSizeBits / 8 / getGRFSize();
    if (vecSize * dataSizeBits / 8 % getGRFSize())
      regsPerVec++; // pad out to full reg
    dataRegs = regsPerVec * execSize;
  }

  // override sizes for special cases
  if (op == LSC_OP::LSC_LOAD_STATUS) {
    dataRegs = 1; // just returns a bitset
  }

  if (opInfo.isLoad()) {
    src1Len = 0;
  } else if (opInfo.isStore()) {
    src1Len = (int)dataRegs;
  }

  SendAccess access =
      opInfo.isLoad() && opInfo.isStore()
          ? SendAccess::READ_WRITE
          : (opInfo.isLoad() ? SendAccess::READ_ONLY : SendAccess::WRITE_ONLY);

  G4_SendDescRaw *g4desc =
      createSendMsgDesc(sfid, desc, exDesc, src1Len, access, surface);
  if (exDescImmOff) {
    g4desc->setExDescImmOff(exDescImmOff);
  }
  g4desc->setLdStAttr(otherAttrs);
  return g4desc;
}

// render target descriptor construction
G4_SendgDesc *IR_Builder::createRenderTargetDesc(
    MsgOp op, int chMask, bool isAlphaPresent, bool isStencilPresent,
    bool isDepthPresent, bool isOutputMaskPresent, bool nullRenderTarget,
    bool lastRenderTarget) {

  // This function constructs the descriptor for render target operations
       // descriptor format
       // [5:0] message type
       // [10:7] cmask
       // [14] last render target select
       // [21] null render target
       // [34] output mask to render target
       // [35] source depth present to render target
       // [36] stencil present to render target
       // [37] source0 alpha present

  uint64_t desc = 0;
  // get msg op encoding
  desc |= MsgOpEncode(op);

  // channel mask
  desc |= (uint64_t)chMask << 7;

  // last render target select
  desc |= (uint64_t)lastRenderTarget << 14;

  // null render target
  desc |= (uint64_t)(nullRenderTarget ? 1 : 0) << 21;

  // output mask to render target
  desc |= (uint64_t)(isOutputMaskPresent ? 1 : 0) << 34;

  // source depth present to render target
  desc |= (uint64_t)(isDepthPresent ? 1 : 0) << 35;

  // stencil present to render target
  desc |= (uint64_t)(isStencilPresent ? 1 : 0) << 36;

  // source0 alpha present
  desc |= (uint64_t)(isAlphaPresent ? 1 : 0) << 37;

  auto msgDesc = new (mem) G4_SendgDesc(SFID::DP_RC, desc, *this);

  // return the constructed descriptor
  return msgDesc;
}

// sampler descriptor construction
G4_SendgDesc *IR_Builder::createSamplerDesc(
    MsgOp op, ChannelMask chMask, bool FP16Return, bool FP16Input,
    bool pixelNullMask,bool feedbackMessage, uint32_t surfaceImmIndex,
    uint32_t samplerImmIndex, int64_t aoffimmiVal, bool isGather4) {

  // This function constructs the descriptor for sampler operations
  //
  // descriptor format
  // [5:0] - sampler message type
  // [6:6] - Enable LSCBacking
  // [10:9] - gather4 source channel select (if gather4)
  // [10:7] - write channel mask (if !gather4)
  // [13:11] - surface type hint
  // [14:14] - address input format
  // [15:15] - data return format
  // [19:16] - cache mode (MBZ)
  // [20] - TRTT_NULL
  // [21] - feedback message
  // [26:22] - surface state index
  // [29:27] - sampler state index
  // [33:30] - r offset from aoffimmi
  // [37:34] - v offset from aoffimmi
  // [41:38] - u offset from aoffimmi

  // for sampler, data size is restricted to D16/D32 and address size is
  // restricted to A16/A32
  // For now, A32 is only supported (STATEFUL_A32)
  uint64_t desc = 0;
  const uint32_t LSCBacking = 6;

  if (getOption(vISA_enableSamplerLSCCaching))
    desc |= (1ull << LSCBacking);

  // op encoding
  desc |= MsgOpEncode(op);

  // channel mask
  if (!isGather4) {
    // sampler
    desc |= (uint64_t)chMask.getHWEncoding(false) << 7;
  } else {
    desc |= (uint64_t)chMask.getSingleChannel() << 9;
  }

  // surface type hint
  // For now setting up to 0 as hardware optimizations that use this hint
  // are not implemented
  // desc |= (uint64_t)0 << 11;

  // address input format
  desc |= (FP16Input) ? 1 << 14 : 0 << 14;

  // data return type
  desc |= (FP16Return) ? 1 << 15 : 0 << 15;

  // cache options are MBZ
  desc |= (uint64_t)0 << 16;

  // trtt_null
  desc |= (uint64_t)pixelNullMask << 20;

  // feedback message
  desc |= (uint64_t)feedbackMessage << 21;

  // surface and sampler state index
  vISA_ASSERT((surfaceImmIndex & ~0x1F) == 0x0, "surface index too large");
  vISA_ASSERT((samplerImmIndex & ~0x7) == 0x0, "sampler index too large");
  desc |= (uint64_t)surfaceImmIndex << 22;
  desc |= (uint64_t)samplerImmIndex << 27;

  // r,v,u offsets in aoffimmi
  desc |= aoffimmiVal << 30;

  // generate the G4_SendgDesc object
  auto msgDesc = new (mem) G4_SendgDesc(SFID::SAMPLER, desc, *this);

  if (desc & (1ull << LSCBacking))
    msgDesc->setSamplerLSCCacheBit(LSCBacking);

  // return the constructed descriptor
  return msgDesc;
}

/// computeOperandLengthsUntypedLscLdSt - Compute the src and destination
/// operands' length (in registers) for untyped ld st atomics --
/// these values will be used when encoding the send instruction
///
/// \param msgDesc          Message descriptor
///
/// \param execSize         execution size
///
/// \param grfSize          # GRF size in bytes
///
/// \param extraOperands    some atomic operations require extra operands
///                         such as atomic add, sub etc.
///
/// \param isDstNullReg     flag to denote whether destination is a null
///                          register or not
static void computeOperandLengthsUntypedLscLdSt(G4_SendgDesc* msgDesc,
  uint32_t grfSize,
  G4_ExecSize execSize,
  bool isDstNullReg)
{
  // compute number of address and data registers based on data order
  uint32_t addrRegs = 1;
  uint32_t dataRegs = 1;

  auto elemsPerAddr = msgDesc->getElemsPerAddr();
  if (msgDesc->isDataOrderNonTranspose()) {
    // Minimum exec size should be SIMD16 for non transpose messages
    // For atomic messages that have execution size = SIMD1, the address
    // payload must be 1 for a32 and 2 for a64
    uint32_t width = std::max(execSize, g4::SIMD16);
    addrRegs = std::max<uint32_t>(1, (uint32_t)width *
                                        msgDesc->getAddrSizeBytes() / grfSize);
    dataRegs = std::max<uint32_t>(1, (uint32_t)width *
                                        msgDesc->getDataSizeBytesReg() / grfSize) *
                                        elemsPerAddr;
  } else {
    uint32_t regsPerVec =
      (elemsPerAddr * msgDesc->getDataSizeBytesReg()) / grfSize;
    if ((elemsPerAddr * msgDesc->getDataSizeBytesReg()) % grfSize) {
      regsPerVec++;
    }
    vISA_ASSERT(execSize == 1, "execSize in transpose must be 1");
    dataRegs = regsPerVec;
  }
  auto op = msgDesc->getOp();
  if (MsgOpIsLoad(op)) {
    msgDesc->setSrc0Len(addrRegs);
    msgDesc->setSrc1Len(0);
    msgDesc->setDstLen((isDstNullReg) ? 0 : dataRegs);
  } else if (MsgOpIsStore(op)) {
    msgDesc->setSrc0Len(addrRegs);
    msgDesc->setSrc1Len(dataRegs);
    msgDesc->setDstLen(0);
  } else if (MsgOpIsExtendedCacheCtrl(op)) {
    msgDesc->setSrc0Len(addrRegs);
    msgDesc->setSrc1Len(0);
    msgDesc->setDstLen(0);
  } else { // atomics
    vISA_ASSERT(MsgOpIsAtomic(op), "unexpected message");
    // For append counter atomics, src0 length is 0 as there are no per lane
    // address offsets. IND0 serves as the surface address.
    if (MsgOpIsApndCtrAtomic(op)) {
      msgDesc->setSrc0Len(dataRegs);
      msgDesc->setSrc1Len(0);
      msgDesc->setDstLen((isDstNullReg) ? 0 : dataRegs);
    } else {
      uint32_t extraOperands = MsgOpAtomicExtraArgs(op);
      msgDesc->setSrc0Len(addrRegs);
      msgDesc->setSrc1Len(dataRegs * extraOperands);
      msgDesc->setDstLen((isDstNullReg) ? 0 : dataRegs);
    }
  }
}

G4_SendgDesc *IR_Builder::createUntypedCMaskDesc(
    SFID sfid, MsgOp op,
    G4_ExecSize execSize, bool isDstNull,
    DataSize ds, DataChMask chMask, AddrSizeType ast,
    int &addrScale, int &addrOffset, unsigned surfaceIndex,
    std::tuple<Caching, Caching, Caching> cacheOpts, bool overfetch) {
  // This function constructs the descriptor for untyped cmask ld/st
  // operations based on the new descriptor format
  // descriptor format
  // [5:0] opcode
  // [10:7] cmask
  // [13:11] data size
  // [15:14] address type and size
  // [19:16] caching
  // [21]    overfetch
  // [26:22] surface state index (stateful)
  // [43:27] global offset (stateful)
  // [43:22] global offset (stateless)
  // [45:44] offset scaling

  // 64bit desc value to populate
  uint64_t desc = 0;

  // op encoding
  uint32_t opEnc = MsgOpEncode(op);
  vISA_ASSERT(opEnc <= 0x1F, "unrecognized op");
  desc |= opEnc;

  // channel mask
  vISA_ASSERT(int(chMask) != 0 && (int(chMask) & ~0xF) == 0x0,
    "invalid data channel mask");
  desc |= uint64_t(chMask) << 7;

  // data size
  desc |= (uint64_t)GetDataSizeEncoding(ds) << 11;

  // addr size type
  desc |= (uint64_t)GetAddrSizeTypeEncoding(ast) << 14;

  uint32_t cacheEnc = MsgOpIsLoad(op)
      ? LSCComputeCachingEncodingLoad(cacheOpts)
      : LSCComputeCachingEncodingStore(cacheOpts);
  desc |= (uint64_t)cacheEnc << 16;

  // overfetch
  if (overfetch) {
    vISA_ASSERT(std::get<0>(cacheOpts) == Caching::CA, "needs L1 caching option to be cached");
    desc |= (uint64_t)1 << 21;
  }

  // surface state index (applicable for stateful addressing mode)
  if (ast == AddrSizeType::GLB_STATE_A32) {
    vISA_ASSERT((surfaceIndex & ~0x1F) == 0x0, "surface index too large");
    desc |= (uint64_t)surfaceIndex << 22;
  } else {
    vISA_ASSERT(surfaceIndex == 0, "surface index invalid on stateless");
  }

  // generate the G4_SendgDesc object
  auto msgDesc = new (mem) G4_SendgDesc(sfid, desc, *this);
  if (sfid != SFID::URB) {
    // offset scaling is not supported for URB and TGM SFID
    // TGM is handled in a different code path; URB is handled here
    if (supportSendAddrScale(msgDesc->isSLM(), msgDesc->isBTS()) &&
        msgDesc->encodeAddrScale(addrScale))
      addrScale = 1;
  }
  if (msgDesc->encodeAddrGlobalOffset(addrOffset))
    addrOffset = 0;
  computeOperandLengthsUntypedLscLdSt(msgDesc, getGRFSize(), execSize,
                                      isDstNull);
  return msgDesc;
}

G4_SendgDesc *IR_Builder::createUntypedVecDesc(
    SFID sfid, MsgOp op,
    G4_ExecSize execSize, bool isDstNull,
    DataSize ds, VecElems ve, DataOrder dord,
    AddrSizeType ast, int &addrScale, int &addrOffset,
    unsigned surfaceIndex, std::tuple<Caching, Caching, Caching> cacheOpts,
    bool overfetch) {
  // This function constructs the descriptor for untyped LSC ld/st operations
  // based on the new descriptor format
  // descriptor format
  // [5:0] opcode
  // [9:7] vector size
  // [10:10] transpose
  // [13:11] data size
  // [15:14] address type and size
  // [19:16] caching
  // [21]    overfetch
  // STATEFUL
  // [26:22] surface state index
  // [43:27] global offset
  // STATELESS
  // [43:22] global offset
  // [45:44] offset scaling

  // 64-bit desc value
  uint64_t desc = 0;

  // op encoding
  desc |= MsgOpEncode(op);

  // vector size or channel mask
  vISA_ASSERT(!MsgOpIsAtomic(op) || ve == VecElems::V1,
             "atomics data vec size must be v1");

  desc |= (uint64_t)GetVecElemsEncoding(ve) << 7;

  // data order
  vISA_ASSERT(!MsgOpIsAtomic(op) || dord == DataOrder::NONTRANSPOSE,
              "atomics must be non-transpose");
  desc |= (uint64_t)GetDataOrderEncoding(dord) << 10;

  // data size
  vISA_ASSERT(!MsgOpIsBFAtomic(op) || ds == DataSize::D16U32,
             "bf atomics must use d16u32 data type");
  desc |= (uint64_t)GetDataSizeEncoding(ds) << 11;

  // addr size type
  desc |= (uint64_t)GetAddrSizeTypeEncoding(ast) << 14;

  // caching
  uint32_t cacheEnc = MsgOpIsLoad(op)
                        ? LSCComputeCachingEncodingLoad(cacheOpts)
                        : LSCComputeCachingEncodingStore(cacheOpts);
  desc |= (uint64_t)cacheEnc << 16;

  // overfetch
  if (overfetch) {
    vASSERT(std::get<0>(cacheOpts) == Caching::CA);
    desc |= (uint64_t)1 << 21;
  }

  // surface state index (applicable for stateful addressing mode)
  if (ast == AddrSizeType::GLB_STATE_A32) {
    vISA_ASSERT((surfaceIndex & ~0x1F) == 0x0, "surface index too large");
    desc |= (uint64_t)surfaceIndex << 22;
  } else {
    vISA_ASSERT(surfaceIndex == 0, "surface index invalid in stateless");
  }

  // generate the G4_SendgDesc object
  auto msgDesc = new (mem) G4_SendgDesc(sfid, desc, *this);
  if (sfid != SFID::URB) {
    // offset scaling is not supported for URB and TGM SFID
    // TGM is handled in a different code path; URB is handled here
    if (supportSendAddrScale(msgDesc->isSLM(), msgDesc->isBTS()) &&
        msgDesc->encodeAddrScale(addrScale))
      addrScale = 1;
  }
  if (msgDesc->encodeAddrGlobalOffset(addrOffset))
    addrOffset = 0;
  computeOperandLengthsUntypedLscLdSt(msgDesc, getGRFSize(), execSize,
                                      isDstNull);
  return msgDesc;
}

/// computeOperandLengthsUntyped2DLscLdSt - Compute the src and destination
/// operands' length (in registers) for untyped 2D ld st --
/// these values will be used when encoding the send instruction
///
/// \param msgDesc          Message descriptor
///
/// \param width2D          data shape width
///
/// \param height2D         data shape height
///
/// \param blocks2D         data shape blocks
///
/// \param grfSize          # GRF size in bytes
///
/// \param isDstNullReg     flag to denote whether destination is a null
///                          register or not
///
static void computeOperandLengthsUntyped2DLscLdSt(G4_SendgDesc *msgDesc,
                                                  uint32_t width2D,
                                                  uint32_t height2D,
                                                  uint32_t blocks2D,
                                                  uint32_t grfSize,
                                                  bool isDstNullReg)
{
  auto alignUp = [](int a, int n) { return n + a - 1 - ((n + a - 1) % a); };

  if (msgDesc) {
    // src0len = 1 for load/store 2d block
    msgDesc->setSrc0Len(1);

    int grfRowPitchesElems = RoundUpToPowerOf2(
        (msgDesc->isDataOrderNonTranspose()) ? width2D : height2D);

    int blockRows = (msgDesc->isDataOrderNonTranspose()) ? height2D : width2D;
    int divisor = msgDesc->getDataSizeBytesReg();
    if (divisor == 0) {
      divisor = 1; // avoid division by 0
      vISA_ASSERT_INPUT(0, "invalid message descriptor");
    }
    int elemsPerGrf = grfSize / divisor;
    int regsPerBlock =
        alignUp(elemsPerGrf, blockRows * grfRowPitchesElems) / elemsPerGrf;

    int dataRegs = blocks2D * regsPerBlock;
    if (msgDesc->isOp(MsgOp::LOAD_BLOCK2D)) {
      msgDesc->setSrc1Len(0);
      msgDesc->setDstLen(isDstNullReg ? 0 : dataRegs);
    } else if (msgDesc->isOp(MsgOp::STORE_BLOCK2D)) {
      msgDesc->setSrc1Len(dataRegs);
      msgDesc->setDstLen(0);
    }
  }
}

/// computeOperandLengthsTyped2DLscLdSt - Compute the src and destination
/// operands' length (in registers) for typed 2D ld st --
/// these values will be used when encoding the send instruction
///
/// \param msgDesc               Message descriptor
///
/// \param width2D          data shape width
///
/// \param height2D         data shape height
///
/// \param grfSize          # GRF size in bytes
///
/// \param isDstNullReg     flag to denote whether destination is a null
///                          register or not
static void computeOperandLengthsTyped2DLscLdSt(
    G4_SendgDesc *msgDesc, uint32_t width2D, uint32_t height2D,
    uint32_t BYTES_PER_REG, bool isDstNullOperand) {
  // If Block Width is not a power of 2 number, the allocated GRF space for
  // Width is padded up to the next power-of-two value . E.g. if the 2D block
  // width (in  bytes) is 12, the space allocated in the GRF will be 16 bytes
  // per row (next power-of-two number)
  int alignWidthInBytes = 0;
  int widthInBytes = width2D;
  if (widthInBytes <= 0 || widthInBytes > 64) {
    vISA_ASSERT_INPUT(false, "block width must be [1,64]");
  } else if (widthInBytes <= 4) {
    // Minimum bytes per row in the GRF is 4 bytes.
    alignWidthInBytes = 4;
  } else {
    // round up to power of 2
    alignWidthInBytes = RoundUpToPowerOf2(widthInBytes);
  }

  int blockSizeInBytes = alignWidthInBytes * height2D;
  vISA_ASSERT_INPUT(blockSizeInBytes <= 256, "block size can not exceed 256");
  int dataRegs = (int)std::ceil((double)blockSizeInBytes / BYTES_PER_REG);

  if (msgDesc->isOp(MsgOp::LOAD_BLOCK2D)) {
    if (isDstNullOperand) {
      msgDesc->setDstLen(0);
    } else {
      msgDesc->setDstLen(dataRegs);
    }
    msgDesc->setSrc1Len(0);
    msgDesc->setSrc0Len(1);
  } else {
    msgDesc->setDstLen(0);
    msgDesc->setSrc0Len(1);
    msgDesc->setSrc1Len(dataRegs);
  }
}

// 2D descriptor construction
G4_SendgDesc *
IR_Builder::create2DLSCDesc(SFID sfid, bool dstIsNull, MsgOp op,
                            int blockWidth, int blockHeight, int blockCount,
                            DataSize ds,
                            DataOrder dord, AddrSizeType as,
                            std::tuple<Caching, Caching, Caching> cacheOpts,
                            int xOffset, int yOffset) {
  // This function constructs the descriptor for 2D LSC ld/st
  // operations based on the new descriptor format
  // descriptor format
  // [5:0] opcode
  // [9:9] vnni (if ugm)
  // [10:10] transpose block
  // [13:11] data size
  // [15:14] address type and size
  // [19:16] caching
  // [33:22] global x offset (UGM only)
  // [45:34] global y offset (UGM only)

  // 64bit desc value to populate
  uint64_t desc = 0;

  // op encoding
  desc |= MsgOpEncode(op);

  // vnni and transpose encoding [10:9]
  desc |= (uint64_t)GetDataOrderEncoding2D(dord) << 9;

  // data size
  desc |= (uint64_t)GetDataSizeEncoding(ds) << 11;

  // address type and size
  desc |= (uint64_t)GetAddrSizeTypeEncoding(as) << 14;

  // caching
  uint32_t cacheEnc = (op == MsgOp::STORE_BLOCK2D)
                          ? LSCComputeCachingEncodingStore(cacheOpts)
                          : LSCComputeCachingEncodingLoad(cacheOpts);
  desc |= (uint64_t)cacheEnc << 16;

  // block2d immediate xOffset, yOffset
  //  - 12b X and 12b Y supported for ugm only
  if (sfid == SFID::UGM) {
    vISA_ASSERT_INPUT(xOffset >= -(1 << 11) && xOffset <= (1 << 11) - 1,
                      "UGM block2d offset out of bounds");
    vISA_ASSERT_INPUT(yOffset >= -(1 << 11) && yOffset <= (1 << 11) - 1,
                      "UGM block2d offset out of bounds");
  } else {
    vISA_ASSERT_INPUT(xOffset == 0 && yOffset == 0,
                      "global offsets only allowed in UGM");
  }
  desc |= ((uint64_t)(xOffset & 0xFFF) << 22);
  desc |= ((uint64_t)(yOffset & 0xFFF) << 34);

  // we now have the desc constructed
  // generate the G4_SendgDesc object
  auto msgDesc = new (mem) G4_SendgDesc(sfid, desc, *this);

  // compute operand lengths
  if (sfid == SFID::TGM) {
    computeOperandLengthsTyped2DLscLdSt(msgDesc, blockWidth, blockHeight,
                                        getGRFSize(), dstIsNull);
  } else {
    computeOperandLengthsUntyped2DLscLdSt(msgDesc,
                                          blockWidth, blockHeight, blockCount,
                                          getGRFSize(), dstIsNull);
  }

  return msgDesc;
}

G4_SendgDesc *IR_Builder::createTypedChMaskDesc(
    MsgOp op, DataSize ds, DataChMask chMask, AddrSizeType as,
    unsigned surfaceIndex, int uOffset, int vOffset, int rOffset,
    std::tuple<Caching, Caching, Caching> cacheOpts) {
  // This function constructs the descriptor for typed LSC cmask
  // operations based on the new descriptor format

  // descriptor format
  // [5:0] opcode
  // [6:6] Enable LSCBacking
  // [10:7] component mask
  // [13:11] reserved
  // [14] address input format
  // [15] data return format
  // [19:16] caching
  // [26:22] surface state index
  // [33:30] r offset
  // [37:34] v offset
  // [41:38] u offset

  uint64_t desc = 0;
  const uint32_t LSCBacking = 6;

  if (getOption(vISA_enableSamplerLSCCaching))
    desc |= (1ull << LSCBacking);

  // op encoding
  desc |= MsgOpEncode(op);

  // channel mask encoding
  desc |= uint64_t(chMask) << 7;

  // address input format
  vISA_ASSERT(as == AddrSizeType::GLB_STATE_A32, "must be 32-bit");
  desc |= (0ull << 14);

  // data return format
  vISA_ASSERT(ds == DataSize::D32, "must be 32-bit");
  desc |= (0ull << 15);

  // caching
  uint32_t cacheEnc = MsgOpIsLoad(op)
                          ? LSCComputeCachingEncodingLoad(cacheOpts)
                          : LSCComputeCachingEncodingStore(cacheOpts);
  desc |= (uint64_t)cacheEnc << 16;

  // surface index
  desc |= (uint64_t)surfaceIndex << 22;

  // u, v, r offsets
  vISA_ASSERT(uOffset >= -8 && uOffset <= 7, "u-coord immoff");
  vISA_ASSERT(vOffset >= -8 && vOffset <= 7, "v-coord immoff");
  vISA_ASSERT(rOffset >= -8 && rOffset <= 7, "r-coord immoff");
  desc |= (uint64_t)(rOffset & 0xF) << 30;
  desc |= (uint64_t)(vOffset & 0xF) << 34;
  desc |= (uint64_t)(uOffset & 0xF) << 38;

  auto msgDesc = new (mem) G4_SendgDesc(SFID::TGM, desc, *this);

  if (desc & (1ull << LSCBacking))
    msgDesc->setSamplerLSCCacheBit(LSCBacking);

  return msgDesc;
}

G4_SendgDesc *
IR_Builder::createTypedAtomicDesc(MsgOp op, DataSize ds,
                                  AddrSizeType as, unsigned surfaceIndex,
                                  int uOffset, int vOffset, int rOffset,
                                  std::tuple<Caching, Caching, Caching> cacheOpts) {
  // This function constructs the descriptor for typed LSC atomic
  // operations based on the new descriptor format

  // descriptor format
  // [5:0] opcode
  // [9:7] vector size
  // [13:11] data size
  // [15:14] address type and size
  // [19:16] caching
  // [26:22] surface state index
  // [33:30] r offset
  // [37:34] v offset
  // [41:38] u offset

  uint64_t desc = 0;
  // op encoding
  desc |= MsgOpEncode(op);

  // number of vector elements encoding
  desc |= (uint64_t)GetVecElemsEncoding(VecElems::V1) << 7;

  // data size
  desc |= (uint64_t)GetDataSizeEncoding(ds) << 11;

  // addr size type
  desc |= (uint64_t)GetAddrSizeTypeEncoding(as) << 14;

  // caching
  uint32_t cacheEnc = LSCComputeCachingEncodingAtomic(cacheOpts);
  desc |= (uint64_t)cacheEnc << 16;

  // surface index
  desc |= (uint64_t)surfaceIndex << 22;

  // u, v, r offsets
  desc |= (uint64_t)(rOffset & 0xF) << 30;
  desc |= (uint64_t)(vOffset & 0xF) << 34;
  desc |= (uint64_t)(uOffset & 0xF) << 38;

  auto msgDesc = new (mem) G4_SendgDesc(SFID::TGM, desc, *this);
  return msgDesc;
}

uint32_t IR_Builder::LSCComputeCachingEncodingAtomic(
    const std::tuple<Caching, Caching, Caching>& cacheOpts) const {
  uint32_t cacheEnc = 0;

  auto matches = [&](Caching l1, Caching l2, Caching l3) {
    return (std::get<0>(cacheOpts) == l1 &&
      std::get<1>(cacheOpts) == l2) &&
      std::get<2>(cacheOpts) == l3;
  };

  if (matches(Caching::DF, Caching::DF, Caching::DF)) {
    cacheEnc = 0x0;
  } else if (matches(Caching::UC, Caching::UC, Caching::UC)) {
    cacheEnc = 0x2;
  } else if (matches(Caching::UC, Caching::UC, Caching::WB)) {
    cacheEnc = 0x3;
  } else if (matches(Caching::UC, Caching::WB, Caching::UC)) {
    cacheEnc = 0x4;
  } else {
    vISA_ASSERT_INPUT(false, "invalid caching options");
  }
  return cacheEnc;
}

uint32_t IR_Builder::LSCComputeCachingEncodingLoad(
  const std::tuple<Caching, Caching, Caching>& cacheOpts) const {
  uint32_t cacheEnc = 0;

  auto matches = [&](Caching l1, Caching l2, Caching l3) {
    return (std::get<0>(cacheOpts) == l1 &&
      std::get<1>(cacheOpts) == l2) &&
      std::get<2>(cacheOpts) == l3;
  };

  if (matches(Caching::DF, Caching::DF, Caching::DF)) {
    cacheEnc = 0x0;
  } else if (matches(Caching::UC, Caching::UC, Caching::UC)) {
    cacheEnc = 0x2;
  } else if (matches(Caching::UC, Caching::UC, Caching::CA)) {
    cacheEnc = 0x3;
  } else if (matches(Caching::UC, Caching::CA, Caching::UC)) {
    cacheEnc = 0x4;
  } else if (matches(Caching::UC, Caching::CA, Caching::CA)) {
    cacheEnc = 0x5;
  } else if (matches(Caching::CA, Caching::UC, Caching::UC)) {
    cacheEnc = 0x6;
  } else if (matches(Caching::CA, Caching::UC, Caching::CA)) {
    cacheEnc = 0x7;
  } else if (matches(Caching::CA, Caching::CA, Caching::UC)) {
    cacheEnc = 0x8;
  } else if (matches(Caching::CA, Caching::CA, Caching::CA)) {
    cacheEnc = 0x9;
  } else if (matches(Caching::ST, Caching::UC, Caching::UC)) {
    cacheEnc = 0xA;
  } else if (matches(Caching::ST, Caching::UC, Caching::CA)) {
    cacheEnc = 0xB;
  } else if (matches(Caching::ST, Caching::CA, Caching::UC)) {
    cacheEnc = 0xC;
  } else if (matches(Caching::ST, Caching::CA, Caching::CA)) {
    cacheEnc = 0xD;
  } else if (matches(Caching::RI, Caching::RI, Caching::RI)) {
    cacheEnc = 0xE;
  } else {
    vISA_ASSERT_INPUT(false, "invalid caching options");
  }
  return cacheEnc;
}

uint32_t IR_Builder::LSCComputeCachingEncodingStore(
    const std::tuple<Caching, Caching, Caching>& cacheOpts) const {
  uint32_t cacheEnc = 0;

  auto matches = [&](Caching l1, Caching l2, Caching l3) {
    return (std::get<0>(cacheOpts) == l1 &&
      std::get<1>(cacheOpts) == l2) &&
      std::get<2>(cacheOpts) == l3;
  };

  if (matches(Caching::DF, Caching::DF, Caching::DF)) {
    cacheEnc = 0x0;
  } else if (matches(Caching::UC, Caching::UC, Caching::UC)) {
    cacheEnc = 0x2;
  } else if (matches(Caching::UC, Caching::UC, Caching::WB)) {
    cacheEnc = 0x3;
  } else if (matches(Caching::UC, Caching::WB, Caching::UC)) {
    cacheEnc = 0x4;
  } else if (matches(Caching::UC, Caching::WB, Caching::WB)) {
    cacheEnc = 0x5;
  } else if (matches(Caching::WT, Caching::UC, Caching::UC)) {
    cacheEnc = 0x6;
  } else if (matches(Caching::WT, Caching::UC, Caching::WB)) {
    cacheEnc = 0x7;
  } else if (matches(Caching::WT, Caching::WB, Caching::UC)) {
    cacheEnc = 0x8;
  } else if (matches(Caching::WT, Caching::WB, Caching::WB)) {
    cacheEnc = 0x9;
  } else if (matches(Caching::ST, Caching::UC, Caching::UC)) {
    cacheEnc = 0xA;
  } else if (matches(Caching::ST, Caching::UC, Caching::WB)) {
    cacheEnc = 0xB;
  } else if (matches(Caching::ST, Caching::WB, Caching::UC)) {
    cacheEnc = 0xC;
  } else if (matches(Caching::WB, Caching::UC, Caching::UC)) {
    cacheEnc = 0xD;
  } else if (matches(Caching::WB, Caching::WB, Caching::UC)) {
    cacheEnc = 0xE;
  } else if (matches(Caching::WB, Caching::UC, Caching::WB)) {
    cacheEnc = 0xF;
  } else {
    vISA_ASSERT_INPUT(false, "invalid caching options");
  }
  return cacheEnc;
}
G4_SendgDesc *IR_Builder::createExtendedCacheCtrlDesc(
    G4_ExecSize execSize, CacheControlOperation ccop, CacheControlSize cs,
    std::tuple<Caching, Caching, Caching> cacheOpts) {
  // [19:16] Cache policy
  // [12:11] cache control size
  // [10:7] cache control operation
  // [5:0] opcode

  // 64bit desc value to populate
  uint64_t desc = 0;

  // op encoding
  uint32_t opEnc = MsgOpEncode(MsgOp::EXTENDED_CACHE_CTRL);
  desc |= opEnc;

  // cache control operation
  desc |= (uint64_t)GetCacheControlOpEncoding(ccop) << 7;

  // cache control size
  desc |= (uint64_t)GetCacheControlSizeEncoding(cs) << 11;

  // cache policy
  desc |= (uint64_t)LSCComputeCachingEncodingLoad(cacheOpts) << 16;

  auto msgDesc = new (mem) G4_SendgDesc(SFID::UGM, desc, *this);
  computeOperandLengthsUntypedLscLdSt(msgDesc, getGRFSize(), execSize, true);
  return msgDesc;
}

G4_SendDescRaw *IR_Builder::createLscDesc(SFID sfid, uint32_t desc,
                                          uint32_t extDesc, int src1Len,
                                          SendAccess access, G4_Operand *bti,
                                          LdStAttrs otherAttrs) {
  auto msgDesc = new (mem)
      G4_SendDescRaw(sfid, desc, extDesc, src1Len, access, bti, true, *this);
  msgDesc->setLdStAttr(otherAttrs);
  return msgDesc;
}

G4_InstSend *IR_Builder::createSamplerSendgInst(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src1,
    G4_SrcRegRegion *src2, G4_ExecSize execsize, G4_SendgDesc *msgDesc,
    G4_InstOpts option,
    G4_SrcRegRegion *ind0, G4_SrcRegRegion *ind1, bool is_sendc) {
  G4_opcode send_opcode = is_sendc ? G4_sendgc : G4_sendg;

  fixSendDstType(dst, execsize, *this);

  return createSendgInst(pred, send_opcode, execsize, dst, src1, src2,
                             ind0, ind1, msgDesc, option, true);
}

G4_InstSend *IR_Builder::createLscSendgInst(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
    G4_SrcRegRegion *src1, G4_ExecSize execSize, G4_SendgDesc *msgDesc,
    G4_InstOpts option, G4_SrcRegRegion *ind0, bool append) {
  return createSendgInst(pred, G4_sendg, execSize, dst, src0, src1,
                         ind0, nullptr, msgDesc, option, append);
}

G4_InstSend *IR_Builder::createLscVecSendgInstSeq(
  G4_Predicate *pred, MsgOp op, SFID sfid, G4_ExecSize execSize,
  DataSize dataSize, VecElems dataVecSize, DataOrder dataOrd,
  AddrSizeType addrSizeType,
  std::tuple<Caching,Caching,Caching> caching,
  LdStAttrs attrs,
  G4_DstRegRegion *loadData,
  G4_SrcRegRegion *addrBase, int addrBaseIndex,
  int addrScale, G4_SrcRegRegion *addrReg, int addrOff,
  G4_SrcRegRegion *storeData,
  G4_InstOpts option) {
  vISA_ASSERT(op == MsgOp::LOAD || op == MsgOp::STORE,
              "unexpected op for helper function");

  if (!loadData) {
    loadData = createNullDst(Type_UD);
  }
  if (!storeData) {
    storeData = createNullSrc(Type_UD);
  }

  G4_SendgDesc *desc = createUntypedVecDesc(
      sfid, op, execSize, loadData->isNullReg(), dataSize, dataVecSize, dataOrd,
      addrSizeType, addrScale, addrOff, addrBaseIndex, caching,
      std::get<0>(caching) == Caching::CA &&
          m_options->getOption(vISA_enableOverfetch)
  );

  desc->addAttributes(attrs);
  // need an extensible way of allocating variables
  // (e.g. spill/fill and payload loading has a var they want to use;
  // others might want us to allocate and produce emulation sequences)
  vISA_ASSERT(addrScale == 1,
              "address scale emulation unsupported here currently");
  vISA_ASSERT(addrOff == 0,
              "address offset emulation unsupported here currently");

  G4_InstSend *sendg =
    createLscSendgInst(pred, loadData, addrReg, storeData, execSize,
                       desc, option, addrBase, true);
  return sendg;
}

G4_InstSend *IR_Builder::createLscSendInst(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
    G4_SrcRegRegion *src1, G4_ExecSize execSize, G4_SendDescRaw *msgDesc,
    G4_InstOpts option, LSC_ADDR_TYPE addrType, unsigned ssIdx,
    bool emitA0RegDef) {

  uint32_t exDesc = msgDesc->getExtendedDesc();
  G4_Operand *surface = msgDesc->getBti(); // BTI or SS/BSS
  G4_Operand *exDescOpnd = nullptr;
  G4_Declare *addrDecl = builtinA0Dot2;

  if (surface && surface->isSrcRegRegion()) {
    if (emitA0RegDef) {
      // This path is taken when caller hasn't defined a0.2 register for use
      // as ext msg descriptor of lsc. Currently, spill module defines a0.2
      // once per BB and reuses it in all spill msgs for that BB. Without this
      // check, each spill/fill msg would get its own computation of a0.2
      // which is wasteful.
      if (addrType == LSC_ADDR_TYPE_BTI) {
        // .declare shifted_bti v_type=T num_elts=1
        // ...
        // (surface is the BTI)
        //   shl    tmp        surface      24
        G4_Declare *tmpDecl = createTempVar(1, Type_UD, Any);
        G4_DstRegRegion *tmpDst = createDstRegRegion(tmpDecl, 1);
        createBinOp(G4_shl, g4::SIMD1, tmpDst, surface, createImm(24, Type_UD),
                    InstOpt_WriteEnable, true);
        auto tmpSrc = createSrcRegRegion(tmpDecl, getRegionScalar());
        // set src1.length into exDesc. BTI message is required to be on ExBSO=0
        // mode, so the src.length is part of exDesc
        exDesc = (exDesc & (~0x7FF)) | ((uint32_t)msgDesc->extMessageLength() << 6);
        G4_DstRegRegion *addrDstOpnd = createDstRegRegion(addrDecl, 1);
        // add a0.2 tmpSrc exdesc
        createBinOp(G4_add, g4::SIMD1, addrDstOpnd, tmpSrc,
                    createImm(exDesc, Type_UD), InstOpt_WriteEnable, true);
      } else {
        // SS or BSS
        // The payload keeps reusing fixed a0.2 serializes loads. To improve the
        // the performance issue caused by this, we create temp address
        // variables and let RA to assign to spread out a0 usage.
        if (useDynamicAddrForExDesc()) {
          addrDecl = createTempAddress(1);
          // Due to encoding restriction, address sub register number must be
          // even value if it's used as extended message descriptor.
          addrDecl->setSubRegAlign(Four_Word);
        }
        G4_DstRegRegion *addrDstOpnd = createDstRegRegion(addrDecl, 1);
        if ((addrType == LSC_ADDR_TYPE_BSS) || (addrType == LSC_ADDR_TYPE_SS)) {
          if (ssIdx == 0x0) {
            //   (W) mov (1)  a0.2  surface
            createMov(g4::SIMD1, addrDstOpnd, surface, InstOpt_WriteEnable, true);
          } else {
            //   (W) add (1)  a0.2  surface   (0x40 * ssIdx):ud
            const unsigned SIZEOF_SURFACE_STATE = 0x40;
            auto *surfOffImm = createImm(SIZEOF_SURFACE_STATE * ssIdx, Type_UD);
            createBinOp(G4_add, g4::SIMD1, addrDstOpnd, surface, surfOffImm,
                        InstOpt_WriteEnable, true);
          }
        } else {
          vISA_ASSERT(false, "FLAT have surface == nullptr here");
        }
      }
    }

    if (getOption(vISA_CopyA0ToDBG0)) {
      G4_SrcRegRegion *srcA0 = createSrcRegRegion(addrDecl, getRegionScalar());
      G4_DstRegRegion *dstDbg0 =
          createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
      createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
    }
    exDescOpnd = createSrcRegRegion(addrDecl, getRegionScalar());
    msgDesc->setSurface(exDescOpnd); // link a0.2 to the send descriptor
  } else if (surface && surface->isImm()) {
    // If by some chance the surface is an immediate value that didn't fold
    // to ExDesc (c.f. lscTryPromoteSurfaceImmToExDesc),
    // we can still possibly move it to a0.2 and use that way.
    // This enables us to access the full ExDesc[31:5] rather than
    // ExDesc[31:12] (the send instruction lacks room encode [11:6])
    // This can happen for BSS/SS, for example, with a small
    // surface state offset.
    //
    // Callers that fold the ExDesc value into an immediate descriptor
    // should pass nullptr as the surface.
    G4_DstRegRegion *addrDstOpnd = createDstRegRegion(addrDecl, 1);
    if (addrType == LSC_ADDR_TYPE_BSS || addrType == LSC_ADDR_TYPE_SS) {
      //   mov    a0.2   SurfaceAddrImm
      auto imm = surface->asImm()->getImm();
      vISA_ASSERT((imm & 0x1F) == 0 && (imm & 0xFFFFFFFF00000000LL) == 0,
                  "ExDesc can only access [31:5]");
      createMov(g4::SIMD1, addrDstOpnd, createImm(imm, Type_UD),
                InstOpt_WriteEnable, true);

      if (getOption(vISA_CopyA0ToDBG0)) {
        G4_SrcRegRegion *srcA0 =
            createSrcRegRegion(addrDecl, getRegionScalar());
        G4_DstRegRegion *dstDbg0 =
            createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
        createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
      }
      exDescOpnd = createSrcRegRegion(addrDecl, getRegionScalar());
      msgDesc->setSurface(exDescOpnd); // link a0.2 to the send descriptor
    } else {
      // BTI is in ExDesc[31:24] and that is always available.
      vISA_ASSERT(false,
                  "BTI/FLAT should not reach this. Flat should have surface\
              == nullptr and BTI should either use a register for a variable BTI \
              or have folded the immediate vlaue into ExDesc (and thus \
                surface==nullptr here)");
    }
  } else {
    exDescOpnd = createImm(exDesc, Type_UD);
  }

  uint32_t descVal = (uint32_t)msgDesc->getDesc();
  return createSplitSendInst(pred, G4_sends, execSize, dst, src0, src1,
                             createImm(descVal, Type_UD), option, msgDesc,
                             exDescOpnd, true);
}

// Using r0.8:ud to save and restore a0.2
G4_SrcRegRegion *IR_Builder::getScratchSurfaceStatusIndex() {
  vISA_ASSERT(!isEfficient64bEnabled(),
    "Efficient64b needs support here");
  auto dst = createDst(builtinR0->getRegVar(), 0, 8, 1, Type_UD);
  auto src0 = createSrcRegRegion(builtinA0Dot2, getRegionScalar());
  createMov(g4::SIMD1, dst, src0, InstOpt_WriteEnable, true);

  G4_SrcRegRegion *R0_5 =
      createSrc(builtinR0->getRegVar(), 0, 5, getRegionScalar(), Type_UD);
  G4_DstRegRegion *A02Dst = createDstRegRegion(builtinA0Dot2, 1);
  createMov(g4::SIMD1, A02Dst, R0_5, InstOpt_WriteEnable, true);
  return createSrcRegRegion(builtinA0Dot2, getRegionScalar());
}

void IR_Builder::RestoreA0() {
  auto dst = createDstRegRegion(builtinA0Dot2, 1);
  auto src0 =
      createSrc(builtinR0->getRegVar(), 0, 8, getRegionStride1(), Type_UD);
  createMov(g4::SIMD1, dst, src0, InstOpt_WriteEnable, true);
}

G4_InstSend *IR_Builder::createLscSendInstToScratch(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
    G4_SrcRegRegion *src1, G4_ExecSize execSize, G4_SendDescRaw *msgDesc,
    G4_InstOpts options, bool usesBti) {
  uint32_t desc = msgDesc->getDesc();
  G4_Operand *surface = msgDesc->getBti(); // BTI or SS/BSS
  G4_Operand *exDescOpnd = nullptr;

  if (isScratchSpace(surface)) {
    desc = (desc & ~0xFF) | 251;
  }
  exDescOpnd = getScratchSurfaceStatusIndex();

  G4_InstSend *inst = createSplitSendInst(pred, G4_sends, execSize, dst, src0,
                                          src1, createImm(desc, Type_UD),
                                          options, msgDesc, exDescOpnd, true);
  RestoreA0();

  return inst;
}

G4_InstSend *IR_Builder::createRenderTargetSendg(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src1,
    G4_SrcRegRegion *src2, G4_ExecSize execSize, G4_SendgDesc *msgDesc,
    G4_SrcRegRegion *ind0Opnd, G4_InstOpts option) {

  fixSendDstType(dst, execSize, *this);

  return createSendgInst(pred, G4_sendgc, execSize, dst, src1, src2,
                             ind0Opnd, nullptr,
                             msgDesc, option, true);
}

// for render target messages,
// desc has a constant BTI value (i.e., no bindless) and no STI
// extDesc may be indirect (MRT and other bits) and is passed in
G4_InstSend *IR_Builder::createSplitSendToRenderTarget(
    G4_Predicate *pred, G4_DstRegRegion *dst, G4_SrcRegRegion *src1,
    G4_SrcRegRegion *src2, G4_SrcRegRegion *extDescOpnd, G4_ExecSize execSize,
    G4_SendDescRaw *msgDesc, G4_InstOpts option) {
  G4_opcode send_opcode = G4_sendsc;

  fixSendDstType(dst, execSize, *this);

  uint32_t desc = msgDesc->getDesc();
  G4_Operand *descOpnd = nullptr;
  G4_Operand *bti = msgDesc->getBti();

  if (bti && bti->isSrcRegRegion()) {
    // add (1) a0.0:ud bti:ud desc:ud
    G4_DstRegRegion *addrDstOpnd = createDstRegRegion(builtinA0, 1);
    createBinOp(G4_add, g4::SIMD1, addrDstOpnd, bti, createImm(desc, Type_UD),
                InstOpt_WriteEnable, true);
    if (getOption(vISA_CopyA0ToDBG0)) {
      G4_SrcRegRegion *srcA0 = createSrcRegRegion(builtinA0, getRegionScalar());
      G4_DstRegRegion *dstDbg0 =
          createDst(phyregpool.getDbgReg(), 0, 0, 1, Type_UD);
      createMov(g4::SIMD1, dstDbg0, srcA0, InstOpt_WriteEnable, true);
    }
    descOpnd = createSrcRegRegion(builtinA0, getRegionScalar());
  } else {
    descOpnd = createImm(desc, Type_UD);
  }

  G4_InstSend *sendInst =
      createSplitSendInst(pred, send_opcode, execSize, dst, src1, src2,
                          descOpnd, option, msgDesc, extDescOpnd, true);

  if (getOption(vISA_renderTargetWriteSendReloc)) {
    std::string symbolName{"RTW_SEND"};
    RelocationEntry::createRelocation(kernel, *sendInst, 0, symbolName,
                                      GenRelocType::R_SEND);
  }
  return sendInst;
}

// create a declare for send payload
G4_Declare *IR_Builder::createSendPayloadDcl(unsigned num_elt, G4_Type type) {
  const char *name = getNameString(16, "M%u", ++num_temp_dcl);
  const uint16_t sizeOfType = TypeSize(type);
  unsigned short numRow =
      (num_elt * sizeOfType - 1) / numEltPerGRF<Type_UB>() + 1;
  unsigned short numElt =
      (numRow == 1) ? num_elt : (numEltPerGRF<Type_UB>() / sizeOfType);
  G4_Declare *dcl = createDeclare(name, G4_GRF, numElt, numRow, type);
  return dcl;
}

void IR_Builder::createMovR0Inst(G4_Declare *dcl, short regOff, short subregOff,
                                 bool use_nomask, G4_InstOpts options) {
  G4_DstRegRegion *dst1_opnd =
      createDst(dcl->getRegVar(), regOff, subregOff, 1, dcl->getElemType());

  // create r0 src
  G4_SrcRegRegion *r0_src_opnd =
      createSrcRegRegion(builtinR0, getRegionStride1());
  // create inst
  createMov(G4_ExecSize(getGenxDataportIOSize()), dst1_opnd, r0_src_opnd,
            (use_nomask ? InstOpt_WriteEnable | options : options), true);
}

void IR_Builder::createAddInst(G4_Declare *dcl, short regOff, short subregOff,
                               G4_ExecSize execsize, G4_Predicate *pred,
                               G4_CondMod *condMod, G4_Operand *src0_opnd,
                               G4_Operand *src1_opnd, G4_InstOption options) {
  auto dst =
      createDst(dcl->getRegVar(), regOff, subregOff, 1, dcl->getElemType());

  if (src0_opnd->isImm() && src0_opnd->asImm()->isZero()) {
    createInst(pred, G4_mov, condMod, g4::NOSAT, execsize, dst, src1_opnd, NULL,
               options, true);
  } else if (src1_opnd->isImm() && src1_opnd->asImm()->isZero()) {
    createInst(pred, G4_mov, condMod, g4::NOSAT, execsize, dst, src0_opnd, NULL,
               options, true);
  } else if (src0_opnd->isImm() && !src1_opnd->isImm()) {
    createInst(pred, G4_add, condMod, g4::NOSAT, execsize, dst, src1_opnd,
               src0_opnd, options, true);
  } else {
    createInst(pred, G4_add, condMod, g4::NOSAT, execsize, dst, src0_opnd,
               src1_opnd, options, true);
  }
}

// Currently this function is mostly used in dataport intrinsic translation
// functions. If it is used in some other places, Qtrctrl should be added in
// options if needed.
void IR_Builder::createMovInst(G4_Declare *dcl, short regOff, short subregOff,
                               G4_ExecSize execSize, G4_Predicate *pred,
                               G4_CondMod *condMod, G4_Operand *src_opnd,
                               bool use_nomask, G4_InstOpts options) {
  G4_DstRegRegion *dst2_opnd =
      createDst(dcl->getRegVar(), regOff, subregOff, 1, dcl->getElemType());

  createInst(pred, G4_mov, condMod, g4::NOSAT, execSize, dst2_opnd, src_opnd,
             NULL, use_nomask ? (InstOpt_WriteEnable | options) : options,
             true);
}

// send payload preparation.
// dcl: decl for send payload
// num_dword: number of DW to send
// src_opnd: send src, its size may be several GRFs
void IR_Builder::createMovSendSrcInst(G4_Declare *dcl, short regoff,
                                      short subregoff, unsigned num_dword,
                                      G4_Operand *src_opnd,
                                      G4_InstOpts options) {
  // since src_opnd is raw source in CISA, it is aligned to GRF, so there is no
  // subRegOff.
  unsigned remained_dword = num_dword;
  // if data type of src_opnd is not UD, change it to UD
  // assumption: size of src_opnd is multiple of UD
  short dst_regoff = regoff, dst_subregoff = subregoff;
  G4_ExecSize execsize = g4::SIMD1;
  G4_DstRegRegion *dst = NULL;
  // G4_SrcRegRegion* src = NULL;
  G4_Operand *src = NULL;
  const RegionDesc *rd = NULL;
  G4_Declare *dst_dcl = dcl;
  short src_regoff = 0, src_subregoff = 0;
  bool non_ud_scalar = false;
  bool scalar_src = (src_opnd->isImm() || num_dword == 1);

  if (scalar_src && src_opnd->getType() != Type_UD) {
    // change the type of dst dcl to src type
    remained_dword = num_dword * (TypeSize(Type_UD) / src_opnd->getTypeSize());
    dst_dcl = createSendPayloadDcl(remained_dword, src_opnd->getType());
    dst_dcl->setAliasDeclare(dcl, regoff * numEltPerGRF<Type_UB>() +
                                      subregoff * TypeSize(Type_UD));
    dst_regoff = 0;
    dst_subregoff = 0;
    non_ud_scalar = true;
  }

  if (!src_opnd->isImm()) {
    // for operands that are not immediate values, get the appropriate register offsets
    src_regoff = src_opnd->asSrcRegRegion()->getRegOff();
    src_subregoff = src_opnd->asSrcRegRegion()->getSubRegOff();
    src_subregoff =
        src_subregoff * src_opnd->getTypeSize() / dst_dcl->getElemSize();
  }

  auto getMaxEsize = [](uint32_t opt) {
    unsigned maskOption = (opt & InstOpt_QuarterMasks);
    switch (maskOption) {
    case InstOpt_M4:
    case InstOpt_M12:
    case InstOpt_M20:
    case InstOpt_M28:
      return 4;
    case InstOpt_M8:
    case InstOpt_M24:
      return 8;
    case InstOpt_M16:
      return 16;
    default:
      return 32;
    }
  };
  G4_ExecSize maxEsize(getMaxEsize(options));

  // here remained_dword is not the number of DW, but the number of dst data
  // type.
  while (remained_dword) {
    if (non_ud_scalar && src_opnd->getTypeSize() != TypeSize(Type_UD)) {
      if (remained_dword >= 32) {
        execsize = g4::SIMD32;
      } else if (remained_dword >= 16) {
        execsize = g4::SIMD16;
      } else {
        execsize = G4_ExecSize((uint8_t)Round_Down_Pow2(remained_dword));
      }

      execsize = (execsize > maxEsize) ? maxEsize : execsize;
      if (execsize == g4::SIMD1) {
        rd = getRegionScalar();
      } else {
        rd = getRegionStride1();
      }
    } else {
      if (remained_dword >= 16) {
        execsize = g4::SIMD16;
      } else if (remained_dword >= 8) {
        execsize = g4::SIMD8;
      } else {
        execsize = G4_ExecSize(Round_Down_Pow2(remained_dword));
      }
      execsize = (execsize > maxEsize) ? maxEsize : execsize;
      if (execsize == g4::SIMD1) {
        rd = getRegionScalar();
      } else {
        rd = getRegionStride1();
      }
    }

    dst = createDst(dst_dcl->getRegVar(), dst_regoff, dst_subregoff, 1,
                    dst_dcl->getElemType());

    if (scalar_src && src_opnd->isImm()) {
      src = src_opnd->asImm();
    } else {
      src = createSrc(src_opnd->asSrcRegRegion()->getBase(), src_regoff,
                      src_subregoff, rd, dst_dcl->getElemType());
    }

    createMov(execsize, dst, src, options, true);

    // update offset in decl
    if (remained_dword >= execsize) {
      remained_dword -= execsize;
      if (execsize * dst_dcl->getElemSize() == 2 * numEltPerGRF<Type_UB>()) {
        dst_regoff += 2;
        if (!scalar_src) {
          src_regoff += 2;
        }
      } else if (execsize * dst_dcl->getElemSize() == numEltPerGRF<Type_UB>()) {
        dst_regoff += 1;
        if (!scalar_src) {
          src_regoff += 1;
        }
      } else {
        dst_subregoff += execsize;
        if (dst_subregoff >
            ((int)numEltPerGRF<Type_UB>() / dst_dcl->getElemSize())) {
          dst_regoff++;
          dst_subregoff -= numEltPerGRF<Type_UB>() / dst_dcl->getElemSize();
        }
        if (!scalar_src) {
          src_subregoff += execsize;
          if (src_subregoff >
              (short)(numEltPerGRF<Type_UB>() / TypeSize(Type_UD))) {
            src_regoff++;
            src_subregoff -= numEltPerGRF<Type_UB>() / TypeSize(Type_UD);
          }
        }
      }
    }
  }
}

// Shfl instruction is like a generic one except:
// -- it takes a G4_ShflOp to specify the function control
// -- conditional modifier is not allowed
// -- source modifier is not allowed
G4_INST *IR_Builder::createShflInst(G4_Predicate *prd, G4_Sat sat,
                                    G4_ExecSize execSize, G4_DstRegRegion *dst,
                                    G4_Operand *src0, G4_Operand *src1,
                                    G4_InstShfl::G4_ShflOp shflOp,
                                    G4_InstOpts options, bool addToInstList) {
  G4_INST *i =
      new (mem) G4_InstShfl(*this, prd, G4_shfl, nullptr, sat, execSize, dst,
                            src0, src1, options, shflOp);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createLfsrInst(G4_Predicate *prd, G4_ExecSize execSize,
                                    G4_DstRegRegion *dst, G4_Operand *src0,
                                    G4_Operand *src1, LFSR_FC funcCtrl,
                                    G4_InstOpts options, bool addToInstList) {
  G4_INST *i = new (mem)
      G4_InstLfsr(*this, prd, execSize, dst, src0, src1, options, funcCtrl);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

G4_INST *IR_Builder::createDnsclInst(G4_Predicate *prd, G4_ExecSize execSize,
                                     G4_DstRegRegion *dst, G4_Operand *src0,
                                     G4_Operand *src1, G4_Operand *src2,
                                     DNSCL_CONVERT_TYPE type, DNSCL_MODE mode,
                                     DNSCL_RND_MODE rndMode,
                                     G4_InstOpts options, bool addToInstList) {
  G4_INST *i = new (mem) G4_InstDnscl(*this, prd, execSize, dst, src0, src1,
                                      src2, options, type, mode, rndMode);

  if (addToInstList) {
    i->setVISAId(curCISAOffset);

    if (m_options->getOption(vISA_EmitLocation)) {
      i->setLocation(allocateMDLocation(curLine, curFile));
    }
    instList.push_back(i);
  }

  instAllocList.push_back(i);

  return i;
}

// create an opnd without regpoff and subregoff
G4_DstRegRegion *IR_Builder::createDstRegRegion(G4_Declare *dcl,
                                                unsigned short hstride) {
  return createDst(dcl->getRegVar(), 0, 0, hstride, dcl->getElemType());
}
// create an opnd without regpoff and subregoff
G4_SrcRegRegion *IR_Builder::createSrcRegRegion(G4_Declare *dcl,
                                                const RegionDesc *rd) {
  return createSrcRegRegion(Mod_src_undef, Direct, dcl->getRegVar(), 0, 0, rd,
                            dcl->getElemType());
}

G4_DstRegRegion *IR_Builder::createNullDst(G4_Type dstType) {
  return createDst(phyregpool.getNullReg(), 0, 0, 1, dstType);
}

G4_SrcRegRegion *IR_Builder::createNullSrc(G4_Type srcType) {
  return createSrcRegRegion(Mod_src_undef, Direct, phyregpool.getNullReg(), 0,
                            0, getRegionScalar(), srcType);
}

// check if the dst opnd align to GRF.
// if it is not aligned to GRF
// 1. change align of var dcl to GRF if the dst size is smaller than GRF size,
//    no alias or alias offset is 0.
// 2. otherwise, create a temp operand and return it.
G4_DstRegRegion *IR_Builder::checkSendDst(G4_DstRegRegion *dst_opnd) {
  // FIXME: This function seems to be bogus
  G4_DstRegRegion *d;
  // check if dst is align to GRF

  const unsigned short SIZEOF_DW = 4;
  if (dst_opnd->getTypeSize() > 1) {
    d = dst_opnd;
  } else {
    // change type of dcl and offset in it
    short new_SubRegOff = dst_opnd->getSubRegOff();
    if (dst_opnd->getRegAccess() == Direct) {
      new_SubRegOff = dst_opnd->getSubRegOff() / SIZEOF_DW;
    }
    G4_DstRegRegion new_dst(*this, dst_opnd->getRegAccess(),
                            dst_opnd->getBase(), dst_opnd->getRegOff(),
                            new_SubRegOff, 1, Type_UD);
    d = createDstRegRegion(new_dst);
  }

  return d;
}

void IR_Builder::addInputArg(input_info_t *inpt) {
  m_inputVect.push_back(inpt);
}

input_info_t *IR_Builder::getInputArg(unsigned int index) const {
  return m_inputVect[index];
}

unsigned int IR_Builder::getInputCount() const {
  return (uint32_t)m_inputVect.size();
}

input_info_t *IR_Builder::getRetIPArg() const {
  // TODO: So far, we assume the last argument of caller of callable kernel
  // or callable kernel is the RetIP argument. If required, extra attribute
  // will be added to specify which QWORD argument is used as RetIP argument
  // and the code will traverse all argument to find that one.
  input_info_t *RetIP = getInputArg(getInputCount() - 1);
  // More sanity check on the argument.
  vISA_ASSERT(IS_QTYPE(RetIP->dcl->getElemType()), "RetIP needs to be QWORD!");
  vISA_ASSERT(RetIP->dcl->getNumElems() == 1,
              "Number of RetIP elements should be 1");
  return RetIP;
}

G4_Predicate_Control
IR_Builder::vISAPredicateToG4Predicate(VISA_PREDICATE_CONTROL control,
                                       G4_ExecSize execSize) {
  switch (control) {
  case PRED_CTRL_NON:
    return PRED_DEFAULT;
  case PRED_CTRL_ANY: {
    if (!predCtrlHasWidth()) {
      return PRED_ANY_WHOLE;
    }
    switch (execSize) {
    case 1:
      return PRED_DEFAULT;
    case 2:
      return PRED_ANY2H;
    case 4:
      return PRED_ANY4H;
    case 8:
      return PRED_ANY8H;
    case 16:
      return PRED_ANY16H;
    case 32:
      return PRED_ANY32H;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid predicate control group size.");
      return PRED_DEFAULT;
    }
  }
  case PRED_CTRL_ALL: {
    if (!predCtrlHasWidth()) {
      return PRED_ALL_WHOLE;
    }
    switch (execSize) {
    case 1:
      return PRED_DEFAULT;
    case 2:
      return PRED_ALL2H;
    case 4:
      return PRED_ALL4H;
    case 8:
      return PRED_ALL8H;
    case 16:
      return PRED_ALL16H;
    case 32:
      return PRED_ALL32H;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid predicate control group size.");
      return PRED_DEFAULT;
    }
  }
  default:
    vISA_ASSERT_UNREACHABLE("Invalid predicate control.");
    return PRED_DEFAULT;
  }
}

// Helper function to fold Unary Op with immediate operand
// Currently it only supports inv/log/exp/sqrt math functions
// Returns nullptr if the instruction cannot be optimized
G4_Imm *IR_Builder::foldConstVal(G4_Imm *opnd, G4_INST *inst) {
  if (!(inst->isMath() && inst->asMathInst()->isOneSrcMath())) {
    return nullptr;
  }

  G4_Type srcT = opnd->getType();
  if (srcT != Type_F) {
    return nullptr;
  }

  float res;
  G4_MathOp mathOp = inst->asMathInst()->getMathCtrl();
  switch (mathOp) {
  case MATH_INV:
    res = 1.0f / opnd->getFloat();
    break;
  case MATH_LOG:
    res = std::log2(opnd->getFloat());
    break;
  case MATH_EXP:
    res = std::exp2(opnd->getFloat());
    break;
  case MATH_SQRT:
    res = std::sqrt(opnd->getFloat());
    break;
  default:
    return nullptr;
  }

  return createImm(res);
}

// helper function to fold BinOp with two immediate operands
// supported opcodes are given below in doConsFolding
// returns nullptr if the two constants may not be folded
G4_Imm *IR_Builder::foldConstVal(G4_Imm *const1, G4_Imm *const2, G4_opcode op,
                                 bool qwordMode) {
  bool isNonQInt = IS_TYPE_INT(const1->getType()) &&
                   IS_TYPE_INT(const2->getType()) &&
                   !IS_QTYPE(const1->getType()) && !IS_QTYPE(const2->getType());

  if (!isNonQInt) {
    return nullptr;
  }

  G4_Type src0T = const1->getType(), src1T = const2->getType(),
          resultType = src0T;

  if (op == G4_mul || op == G4_add || op == G4_and || op == G4_xor ||
      op == G4_mullh || op == G4_or) {
    resultType = findConstFoldCommonType(src0T, src1T);
    if (resultType == Type_UNDEF) {
      return nullptr;
    }

    int64_t res;
    switch (op) {
    case G4_and:
      res = (int64_t)(const1->getInt()) & (int64_t)(const2->getInt());
      break;

    case G4_xor:
      res = (int64_t)(const1->getInt()) ^ (int64_t)(const2->getInt());
      break;

    case G4_or:
      res = (int64_t)(const1->getInt()) | (int64_t)(const2->getInt());
      break;

    case G4_add:
      res = (int64_t)(const1->getInt()) + (int64_t)(const2->getInt());
      break;

    case G4_mul:
    case G4_mullh:
      res = (int64_t)(const1->getInt()) * (int64_t)(const2->getInt());
      break;

    default:
      return nullptr;
    }

    // result type is either D or UD
    // don't fold if the value overflows D/UD
    if (!G4_Imm::isInTypeRange(res, resultType)) {
      return nullptr;
    }
    return createImmWithLowerType(res, resultType);
  } else {
    // For shl/shr/asr instructions, in qword mode the shift count is taken from
    // the low six bits of src1 regardless of the src1 type and treated as an
    // unsigned integer in the range 0 to 63. Otherwise the shift count is taken
    // from the low five bits of src1 regardless of the src1 type and treated as
    // an unsigned integer in the range 0 to 31.
    uint32_t shift = ((uint64_t)const2->getInt()) & (qwordMode? 0x3f : 0x1f);

    if (op == G4_shl || op == G4_shr) {
      uint32_t value = (uint32_t)const1->getInt();
      // set result type to D/UD as it may overflow W. If the value fits the
      // type will be lowered later source type matters here since it affects
      // sign extension
      resultType = IS_SIGNED_INT(resultType) ? Type_D : Type_UD;
      int64_t res = op == G4_shl ? ((int64_t)value) << shift : value >> shift;
      if (!G4_Imm::isInTypeRange(res, resultType)) {
        return nullptr;
      }

      return createImmWithLowerType(res, resultType);
    } else if (op == G4_asr) {
      if (IS_SIGNED_INT(resultType)) {
        int64_t value = const1->getInt();
        int64_t res = value >> shift;
        return createImmWithLowerType(res, resultType);
      } else {
        uint64_t value = const1->getInt();
        uint64_t res = value >> shift;
        return createImmWithLowerType(res, resultType);
      }
    }
  }
  return nullptr;
}

// Currently constant folding is done for the following code patterns:
//
// - op v, imm, imm
//    where op is shl, shr, asr, or, xor, and, add, mul
// Restrictions:
// - operand type cannot be float or Q/UQ
// - saturation is not allowed
void IR_Builder::doConsFolding(G4_INST *inst) {
  if (inst->getSaturate())
    return; // TODO: we could do this if we wanted to bad enough

  auto srcIsFoldableImm = [](const G4_Operand *op) {
    return op && op->isImm() && !op->isRelocImm();
  };

  G4_opcode opcode = inst->opcode();

  if (inst->getNumSrc() == 2) {
    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);
    if (srcIsFoldableImm(src0) && srcIsFoldableImm(src1)) {
      // For shr/shl/asr instructions, the operation uses QWord mode if src0 or
      // dst has the Q or UQ type but not if src1 is the only operand with the
      // Q or UQ type.
      bool qwordMode =
          (G4_shr == opcode || G4_shl == opcode || G4_asr == opcode) &&
          (IS_QTYPE(inst->getDst()->getType()) ||
           IS_QTYPE(inst->getSrc(0)->getType()));
      G4_Imm *foldedImm =
          foldConstVal(src0->asImm(), src1->asImm(), opcode, qwordMode);
      if (foldedImm) {
        // change instruction into a MOV
        inst->setOpcode(G4_mov);
        inst->setSrc(foldedImm, 0);
        inst->setSrc(nullptr, 1);
      }
    }
  } else if (inst->getNumSrc() == 3) {
    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);
    G4_Operand *src2 = inst->getSrc(2);
    if (opcode == G4_add3) {
      // always fold the variable into src0
      G4_Imm *foldedImm = nullptr;
      G4_Operand *otherSrc = nullptr;
      if (srcIsFoldableImm(src0) && srcIsFoldableImm(src1)) {
        foldedImm = foldConstVal(src0->asImm(), src1->asImm(), G4_add);
        otherSrc = src2;
      } else if (srcIsFoldableImm(src0) && srcIsFoldableImm(src2)) {
        foldedImm = foldConstVal(src0->asImm(), src2->asImm(), G4_add);
        otherSrc = src1;
      } else if (srcIsFoldableImm(src1) && srcIsFoldableImm(src2)) {
        foldedImm = foldConstVal(src1->asImm(), src2->asImm(), G4_add);
        otherSrc = src0;
      }
      if (foldedImm) {
        // always put the possible register in src0
        inst->setOpcode(G4_add);
        if (otherSrc != src0) {
          inst->setSrc(otherSrc, 0);
          inst->swapDefUse(Opnd_src0, otherSrc == src1 ? Opnd_src1 : Opnd_src2);
        }
        inst->setSrc(foldedImm, 1);
        inst->setSrc(nullptr, 2);
        // recurse for possible fold again
        doConsFolding(inst);
      }
    } // TODO: integer mad, bfn, bfi, bfe
  }
}

// Perform constant folding for math instructions
// op dst, Imm
// =>
// mov dst, Imm'
G4_INST *IR_Builder::doMathConsFolding(INST_LIST_ITER &iter) {
  G4_INST *inst = *iter;

  if (inst->getSaturate())
    return inst;

  auto srcIsFoldableImm = [](const G4_Operand *op) {
    return op && op->isImm() && !op->isRelocImm();
  };

  if (inst->isMath() && inst->asMathInst()->isOneSrcMath()) {
    G4_Operand *src0 = inst->getSrc(0);
    if (srcIsFoldableImm(src0)) {
      G4_Imm *foldedImm = foldConstVal(src0->asImm(), inst);
      if (foldedImm) {
        auto pred = duplicateOperand(inst->getPredicate());
        auto condMod = duplicateOperand(inst->getCondMod());
        auto dst = duplicateOperand(inst->getDst());
        auto execSize = inst->getExecSize();
        auto sat = inst->getSaturate();
        auto options = inst->getOption();

        G4_INST *newInst =
            createInternalInst(pred, G4_mov, condMod, sat, execSize, dst,
                               foldedImm, nullptr, options);
        newInst->inheritDIFrom(inst);
        inst->transferUse(newInst);
        *iter = newInst;

        return newInst;
      }
    }
  }

  return inst;
}

// Do the following algebraic simplification:
// - mul v, src0, 0 ==> 0, commutative
// - and v, src0, 0 ==> 0, commutative
// - mul v, src0, 1 ==> src0, commutative
// - shl v, src0, 0 ==> src0
// - shr v, src0, 0 ==> src0
// - asr v, src0, 0 ==> src0
// - add v, src0, 0 ==> src0, commutative
void IR_Builder::doSimplification(G4_INST *inst) {
  // Just handle following commonly used ops for now.
  if (inst->opcode() != G4_mul && inst->opcode() != G4_and &&
      inst->opcode() != G4_add && inst->opcode() != G4_shl &&
      inst->opcode() != G4_shr && inst->opcode() != G4_asr &&
      inst->opcode() != G4_mullh && inst->opcode() != G4_mov) {
    return;
  }

  // Try movi promotion
  if (canPromoteToMovi(inst)) {
    // we verified types of the operands in canPromoteToMovi
    G4_INST *LEA = getSingleDefInst(inst, Opnd_src0);
    G4_Operand *Op0 = LEA->getSrc(0);
    G4_AddrExp *AE = Op0->asAddrExp();
    G4_Declare *Dcl = AE->getRegVar()->getDeclare();

    // Set this instruction to movi
    inst->setOpcode(G4_movi);

    // Adjust alignments
    unsigned SrcSizeInBytes =
        inst->getExecSize() * inst->getSrc(0)->getTypeSize();
    G4_SubReg_Align SubAlign = getGRFAlign();
    if (SrcSizeInBytes <= numEltPerGRF<Type_UB>() / 2u) {
      SubAlign = (G4_SubReg_Align)(numEltPerGRF<Type_UW>() / 2);
    }
    if (!Dcl->isEvenAlign() && Dcl->getSubRegAlign() != getGRFAlign()) {
      Dcl->setSubRegAlign(SubAlign);
    }

    // Movi requires indirect 1x1 region.
    const RegionDesc *rd = getRegionStride1();
    inst->getSrc(0)->asSrcRegRegion()->setRegion(*this, rd);

    // Set subreg alignment for the address variable.
    Dcl = LEA->getDst()->getBase()->asRegVar()->getDeclare();
    vISA_ASSERT(Dcl->getRegFile() == G4_ADDRESS,
                "Address variable is required.");
    Dcl->setSubRegAlign(Eight_Word);
  }

  auto isInteger = [](G4_Operand *opnd, int64_t val) {
    if (opnd && IS_TYPE_INT(opnd->getType()) && !opnd->isRelocImm()) {
      return opnd->isImm() && opnd->asImm()->getInt() == val;
    }
    return false;
  };

  G4_Operand *src0 = inst->getSrc(0);
  G4_Operand *src1 = inst->getSrc(1);
  G4_Operand *newSrc = nullptr;
  if (inst->opcode() == G4_mul || inst->opcode() == G4_mullh ||
      inst->opcode() == G4_and) {
    if (isInteger(src1, 0)) {
      inst->removeDefUse(Opnd_src0);
      newSrc = createImm(0, Type_W);
    } else if (isInteger(src0, 0)) {
      inst->removeDefUse(Opnd_src1);
      newSrc = createImm(0, Type_W);
    } else if (inst->opcode() == G4_mul || inst->opcode() == G4_mullh) {
      if (isInteger(src1, 1)) {
        newSrc = src0;
      } else if (isInteger(src0, 1)) {
        inst->swapDefUse();
        newSrc = src1;
      }
    }
  } else if (inst->opcode() == G4_shl || inst->opcode() == G4_shr ||
             inst->opcode() == G4_asr || inst->opcode() == G4_add) {
    if (isInteger(src1, 0)) {
      newSrc = src0;
    } else if (inst->opcode() == G4_add && isInteger(src0, 0)) {
      inst->swapDefUse();
      newSrc = src1;
    }
  }

  if (newSrc != nullptr) {
    inst->setOpcode(G4_mov);
    if (newSrc != src0) {
      inst->setSrc(newSrc, 0);
    }
    inst->setSrc(nullptr, 1);
  }
}

//  find a common (integer) type for constant folding.  The rules are:
//  -- both types must be int
//  -- Q and UQ are not folded
//  -- UD if one of the type is UD
//  -- D otherwise
//
//  returns Type_UNDEF if no appropriate type can be found
//
G4_Type IR_Builder::findConstFoldCommonType(G4_Type type1, G4_Type type2) {
  if (IS_TYPE_INT(type1) && IS_TYPE_INT(type2)) {
    if (TypeSize(type1) == 8 || TypeSize(type2) == 8) {
      return Type_UNDEF;
    }
    if (type1 == Type_UD || type2 == Type_UD) {
      return Type_UD;
    } else {
      return Type_D;
    }
  }
  return Type_UNDEF;
}

IR_Builder::R0_ACCESS IR_Builder::getR0AccessFromOptions() const {
  if (getOption(vISA_enablePreemption) ||
      getOption(vISA_enablePreemptionR0Only))
    return R0_ACCESS::NONE;
  if (getOption(vISA_AvoidUsingR0R1))
    return R0_ACCESS::NONE;
  if (getOption(vISA_ReserveR0))
    return R0_ACCESS::READ_ONLY;
  return R0_ACCESS::READ_WRITE;
}

bool IR_Builder::mustReserveR1() const {
  if (getOption(vISA_enablePreemption))
    return true;
  if (getOption(vISA_AvoidUsingR0R1))
    return true;
  return false;
}

G4_INST * IR_Builder::getSingleDefInst(G4_INST *UI, Gen4_Operand_Number OpndNum) const
{
  G4_INST *Def = nullptr;
  for (auto I = UI->def_begin(), E = UI->def_end(); I != E; ++I) {
    if (I->second != OpndNum)
      continue;
    if (Def) {
      // Not single defined, bail out
      Def = nullptr;
      break;
    }
    Def = I->first;
  }
  return Def;
};

bool IR_Builder::canPromoteToMovi(G4_INST *inst) {
  // Perform 'mov' to 'movi' transform when it's a 'mov' of
  // - simd8
  // - it's a raw mov
  // - dst is within a single GRF.
  // - src uses VxH indirect access.
  // - src is within one GRF.
  // - indices to src are all within src.
  // - destination stride in bytes must be equal to the source element size in
  // bytes.

  // - both src and dst are dword data type for PVC+ platforms:

  bool emitMoreMoviCases = getOption(vISA_emitMoreMoviCases);

  bool isMovOpcode = inst->opcode() == G4_mov;
  bool isValidExecSize =
      (inst->getExecSize() == g4::SIMD8) ||
      (emitMoreMoviCases && (inst->getExecSize() <= g4::SIMD16));
  bool isRawMov = inst->isRawMov();

  bool hasDst = inst->getDst() != nullptr;
  bool isNotCrossGRFDst =
      hasDst && !inst->getDst()->asDstRegRegion()->isCrossGRFDst(*this);

  bool hasSrc0 = inst->getSrc(0) != nullptr;
  bool isSrcRegRegion = hasSrc0 && inst->getSrc(0)->isSrcRegRegion();
  bool isIndirectSrc =
      isSrcRegRegion && inst->getSrc(0)->asSrcRegRegion()->isIndirect();
  bool isRegionWH =
      isIndirectSrc &&
      inst->getSrc(0)->asSrcRegRegion()->getRegion()->isRegionWH();
  bool isWidthOne =
      isRegionWH && inst->getSrc(0)->asSrcRegRegion()->getRegion()->width == 1;

  bool isTypeSizeMatch = hasDst && hasSrc0 &&
                         (inst->getSrc(0)->getTypeSize() ==
                          (inst->getDst()->getTypeSize() *
                           inst->getDst()->asDstRegRegion()->getHorzStride()));

  // Combine all checks
  bool canConvertMovToMovi = isMovOpcode && isValidExecSize && isRawMov &&
                             isNotCrossGRFDst && isIndirectSrc && isWidthOne &&
                             isTypeSizeMatch;

  if (getPlatform() >= Xe_PVC) {
    canConvertMovToMovi = canConvertMovToMovi &&
                          IS_DTYPE(inst->getDst()->getType()) &&
                          IS_DTYPE(inst->getSrc(0)->getType());
  }

  if (!canConvertMovToMovi) {
    return false;
  }

  // Convert 'mov' to 'movi' if the following conditions are met.

  unsigned SrcSizeInBytes =
      inst->getExecSize() * inst->getSrc(0)->getTypeSize();

  if (!(emitMoreMoviCases || SrcSizeInBytes == numEltPerGRF<Type_UB>() / 2 ||
        SrcSizeInBytes == numEltPerGRF<Type_UB>())) {
    return false;
  }

  G4_INST *LEA = getSingleDefInst(inst, Opnd_src0);
  if (!(LEA && LEA->opcode() == G4_add &&
        LEA->getExecSize() == inst->getExecSize())) {
    return false;
  }

  G4_Operand *Op0 = LEA->getSrc(0);
  G4_Operand *Op1 = LEA->getSrc(1);
  G4_Declare *Dcl = nullptr;
  int Offset = 0;
  if (Op0->isAddrExp()) {
    G4_AddrExp *AE = Op0->asAddrExp();
    Dcl = AE->getRegVar()->getDeclare();
    Offset = AE->getOffset();
  }

  if (!Dcl) {
    return false;
  }

  // Immediate in 'uv' ensures each element is a
  // byte-offset within half-GRF.
  bool isAlignedImmediateUV = (Offset % SrcSizeInBytes) == 0 && Op1->isImm() &&
                              Op1->getType() == Type_UV;

  if (isAlignedImmediateUV) {
    return true;
  }

  // Op0's root declare size can make sure if all channels of indirect
  // access fall into 1 GRF
  bool isCandidateForSingleGRFPattern =
      emitMoreMoviCases &&
      Dcl->getRootDeclare()->getByteSize() <= numEltPerGRF<Type_UB>() &&
      (Offset % SrcSizeInBytes) == 0 &&
      (!Op1->isImm() || Op1->getType() == Type_UV) && LEA->isWriteEnableInst();

  if (!isCandidateForSingleGRFPattern) {
    return false;
  }
  // check if we match sanitized pattern:
  // (W) and (16)             V1579(0,0)<2>:uw  V1579(0,0)<2;1,0>:uw  0xf:uw
  // (W) shl (16)             ShuffleTmp_157(0,0)<1>:uw V1579(0,0)<2;1,0>:uw 0x1:uw
  // (W) add (16)             A158(0,0)<1>:uw  &V3130+0 ShuffleTmp_157(0,0)<1;1,0>:uw
  //     mov (16)             V3154(0,0)<1>:w  r[A158(0,0), 0]<1,0>:w

  auto maybeShlOrMul = getSingleDefInst(LEA, Opnd_src1);
  bool isMaybeShlOrMul = maybeShlOrMul &&
                         (maybeShlOrMul->opcode() == G4_shl ||
                          maybeShlOrMul->opcode() == G4_mul) &&
                         maybeShlOrMul->getSrc(1)->isImm() &&
                         maybeShlOrMul->isWriteEnableInst();

  if (!isMaybeShlOrMul) {
    return false;
  }

  auto maybeAnd = getSingleDefInst(maybeShlOrMul, Opnd_src0);
  bool isNoMaskAndWithImm = maybeAnd && maybeAnd->opcode() == G4_and &&
                            maybeAnd->isWriteEnableInst() &&
                            maybeAnd->getSrc(1)->isImm();

  if (!isNoMaskAndWithImm) {
    return false;
  }

  // verify execsizes
  bool areExecSizesEqual = inst->getExecSize() == LEA->getExecSize() &&
                           inst->getExecSize() == maybeAnd->getExecSize() &&
                           inst->getExecSize() == maybeShlOrMul->getExecSize();

  if (!areExecSizesEqual) {
    return false;
  }

  // check whether the address calculated is guaranteed to fit single GRF
  // since we support both shl and mul, need to handle them both
  int64_t indexSize = 0;
  int64_t andImm = maybeAnd->getSrc(1)->asImm()->getInt();
  int64_t shlOrMulImm = maybeShlOrMul->getSrc(1)->asImm()->getInt();
  bool andImmIsPowOf2Min1 =
      andImm > 0 &&
      (andImm & (andImm + 1)) == 0; // this should be 1,3,7,15,31 etc;

  if (maybeShlOrMul->opcode() == G4_mul) {
    indexSize = andImm * shlOrMulImm;
  } else {
    indexSize = andImm << shlOrMulImm;
  }

  // Last check is verify that index size guarantees access smaller than GRF
  // size.
  // Note: we use "less" not "less or equal" since andImm value should be
  // in pow2()-1 form which means that highest addressable index is last
  // element within GRF.
  // With "less or equal" it would address second GRF.
  bool isSanitizedSingleGRFIndexing =
      andImmIsPowOf2Min1 && (indexSize < numEltPerGRF<Type_UB>());

  return isSanitizedSingleGRFIndexing;
}

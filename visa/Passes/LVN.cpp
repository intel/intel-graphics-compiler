/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LVN.hpp"
#include "PointsToAnalysis.h"
#include "../Timer.h"

#include <fstream>
#include <map>
#include <sstream>

using namespace vISA;

// #define DEBUG_LVN_ON

#define DUMMY_HSTRIDE_2_2_0 0x8000
#define DUMMY_HSTRIDE_4_4_0 0x4000
#define DUMMY_HSTRIDE_8_8_0 0xc000
#define DUMMY_HSTRIDE_16_16_0 0x2000

bool isSpecialRegion(const RegionDesc *desc, uint16_t &hstride) {
  bool isSpecial = false;

  // Look for regions like <N;N,0> in general,
  // except for <1;1,0> since it is contiguous.
  // DS uses N=4 case.
  if (desc->vertStride == desc->width && desc->vertStride != 1 &&
      desc->horzStride == 0) {
    isSpecial = true;

    // Update hstride
    switch (desc->vertStride) {
    case 2:
      hstride = DUMMY_HSTRIDE_2_2_0;
      break;
    case 4:
      hstride = DUMMY_HSTRIDE_4_4_0;
      break;
    case 8:
      hstride = DUMMY_HSTRIDE_8_8_0;
      break;
    case 16:
      hstride = DUMMY_HSTRIDE_16_16_0;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Unexpected special hstride seen");
    }
  }

  return isSpecial;
}

// This function looks at region description and execution size
// and using API of RegionDesc class, decides what normalized
// hstride is. Overlapping regions or non-uniform regions, ie
// disconnected regions are not expected in LVN.
uint16_t getActualHStride(G4_SrcRegRegion *srcRgn) {
  const RegionDesc *desc = srcRgn->getRegion();
  uint32_t execSize = srcRgn->getInst()->getExecSize();
  uint16_t stride = desc->horzStride;
  bool isSpecialRgn = isSpecialRegion(desc, stride);

  if (!isSpecialRgn) {
    desc->isSingleStride(execSize, stride);
  }

  return stride;
}

bool LVN::getDstData(int64_t srcImm, G4_Type srcType, int64_t &dstImm,
                     G4_Type dstType, bool &canNegate) {
  // mov (...) V..:dstType       0x...:srcType
  // Given srcImm data, compute what dst register will contain
  bool dstImmValid = false;
  int64_t andMask = 0;
  canNegate = (srcImm == 0 ? false : true);
  andMask =
      (0xffffffffffffffff >> ((sizeof(int64_t) * 8) - TypeBitSize(dstType)));

  if (srcType == Type_W || srcType == Type_UW || srcType == Type_D ||
      srcType == Type_UD || srcType == Type_Q || srcType == Type_UQ) {
    dstImm = srcImm;
    if (IS_TYPE_INT(dstType)) {
      dstImm &= andMask;
      dstImmValid = true;
    } else if (srcImm == 0) {
      // Make special case for following type
      // conversion:
      // mov (..) V1:f    0:w/uw/d/ud
      dstImm &= andMask;
      dstImmValid = true;
    } else if (dstType == Type_F) {
      union {
        int32_t i;
        float f;
      } ftod;
      int32_t intSrc = (int32_t)srcImm;
      ftod.f = (float)intSrc;
      dstImm = (int64_t)ftod.i;
      dstImm &= andMask;

      // Now ensure the int is within representable
      // precision range of float. So reverse
      // convert and compare with original data.
      int32_t reverseInt;
      reverseInt = (int32_t)ftod.f;
      if (reverseInt == intSrc) {
        dstImmValid = true;
      }
    }
  } else if (srcType == Type_HF || srcType == Type_F || srcType == Type_DF) {
    dstImm = srcImm;
    if (dstType == srcType || srcImm == 0) {
      dstImm &= andMask;
      dstImmValid = true;
    }
  } else if (IS_VTYPE(srcType)) {
    dstImm = srcImm;
    dstImmValid = true;
    canNegate = false;
  }

  return dstImmValid;
}

bool LVN::getAllUses(G4_INST *def, UseList &uses) {
  bool defFound = false;
  auto it = defUse.find(def);
  if (it != defUse.end()) {
    defFound = true;
    uses = (*it).second;
  }

  return defFound;
}

bool LVN::canReplaceUses(INST_LIST_ITER inst_it, UseList &uses,
                         G4_INST *lvnInst, bool negMatch, bool noPartialUse) {
  // Look up all uses of current instruction.
  // Then check if each use is fully defined by current
  // instruction. Return true if this is true for all
  // uses.
  G4_INST *defInst = (*inst_it);
  G4_DstRegRegion *def = defInst->getDst();
  G4_DstRegRegion *lvnDst = lvnInst->getDst();
  G4_Declare *lvnDstTopDcl = lvnDst->getTopDcl();

  if (def->getBase()->asRegVar()->getDeclare()->getRegFile() !=
      lvnDst->getBase()->asRegVar()->getDeclare()->getRegFile()) {
    return false;
  }

  // The sizes of the flag variables must match
  if (def->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_FLAG) {
    G4_Declare *defTopDcl = def->getTopDcl();
    if (defTopDcl->getNumElems() != lvnDstTopDcl->getNumElems() ||
        defTopDcl->getElemType() != lvnDstTopDcl->getElemType()) {
      return false;
    }
  }

#ifdef DEBUG_LVN_ON
  std::cerr << "Inst with same value in LVN Table:"
            << "\n";
  lvnInst->emit(std::cerr);
  std::cerr << "\n";
  std::cerr << "Current inst:"
            << "\n";
  defInst->emit(std::cerr);
  std::cerr << "\n"
            << "Uses:"
            << "\n";

  char shouldReplace[2];
#endif

  // Check if def defines each use fully
  unsigned int lb = def->getLeftBound();
  unsigned int rb = def->getRightBound();
  unsigned int hs = def->getHorzStride();

  bool canReplace = true;
  for (auto use_it = uses.begin(), uend = uses.end(); use_it != uend;
       use_it++) {
    G4_INST *useInst = (*use_it).first;
    Gen4_Operand_Number opndNum = (*use_it).second;
    G4_Operand *use = useInst->getOperand(opndNum);
    unsigned int use_lb = use->getLeftBound();
    unsigned int use_rb = use->getRightBound();

    // Ensure a single def flows in to the use
    auto it = useDef.find(use);
    if (it == useDef.end() || (*it).second.size() != 1) {
      canReplace = false;
      break;
    }

    if (opndNum == Opnd_pred || opndNum == Opnd_condMod) {
      canReplace = false;
      break;
    }

    if (!bb->isAllLaneActive()) {
      auto defCoversUseEmask =
          defInst->getMaskOffset() <= useInst->getMaskOffset() &&
          (defInst->getMaskOffset() + defInst->getExecSize() >=
           useInst->getMaskOffset() + useInst->getExecSize());
      if (!defInst->isWriteEnableInst() &&
          (!defCoversUseEmask || useInst->isWriteEnableInst())) {
        // if defInst does not fully cover useInst, it's generally unsafe to do
        // LVN
        canReplace = false;
        break;
      }
    }

    // Compute a single positive stride if exists.
    unsigned int use_hs = 0;
    {
      uint16_t stride = 0;
      const RegionDesc *rd = use->asSrcRegRegion()->getRegion();
      if (rd->isSingleStride(useInst->getExecSize(), stride)) {
        use_hs = stride;
      }
    }

    if (noPartialUse) {
      if (lb != use_lb || rb != use_rb ||
          (hs != use_hs && defInst->getExecSize() > g4::SIMD1)) {
        canReplace = false;
        break;
      }
    } else {
      // ok as long as def covers entire use
      if (lb > use_lb || rb < use_rb) {
        canReplace = false;
        break;
      }
    }

    if (hs != 1) {
      auto relation = defInst->getDst()->compareOperand(use, builder);
      if (!(relation == Rel_eq || relation == Rel_gt)) {
        canReplace = false;
        break;
      }
    }

    if (useInst->isSend()) {
      // send operand doesn't take subreg, so the operand has to be GRF-aligned
      if (!builder.tryToAlignOperand(lvnDst, builder.numEltPerGRF<Type_UB>())) {
        canReplace = false;
        break;
      }
    }

    if (useInst->isSplitSend()) {
      // Ensure both src opnds of split are not the same
      if ((opndNum == Opnd_src0 &&
           useInst->getSrc(1)->getTopDcl() == lvnDstTopDcl) ||
          (opndNum == Opnd_src1 &&
           useInst->getSrc(0)->getTopDcl() == lvnDstTopDcl)) {
        canReplace = false;
        break;
      }
    }

    if (negMatch) {
      if (!useInst->canSupportSrcModifier()) {
        // LVN is invalid if instruction does not support negatve modifier
        canReplace = false;
        break;
      }

      if (useInst->isRawMov() && useInst->getDst()->getTypeSize() == 1) {
        // For byte-type raw movs, src modifier '-' is invalid
        canReplace = false;
        break;
      }

      if (IS_UNSIGNED_INT(use->getType())) {
        // Bug fix for following sequence:
        // a:w = 1:w
        // b:uw = 0xffff:uw
        // c:d = x:d + b:uw
        //
        // It is incorrect to replace b:uw above with -a:uw.
        canReplace = false;
        break;
      }
      if (use->getType() != lvnInst->getDst()->getType()) {
        //
        // negate's meaning can change based on type
        //
        canReplace = false;
        break;
      }
    }

    // Iterate to ensure there is no WAR/WAW
    auto fwdInst_it = inst_it;
    fwdInst_it++;
    bool lvnTopDclAddresses = lvnDstTopDcl->getAddressed();
    bool lvnTopDclGRFAssigned =
        (lvnDstTopDcl->getRegVar()->getPhyReg() != NULL);
    while ((*fwdInst_it) != useInst) {
      G4_DstRegRegion *curDst = (*fwdInst_it)->getDst();

      // FIXME: why not use isWAWDep()?
      if (curDst && !curDst->isNullReg()) {
        if (lvnTopDclAddresses && curDst->isIndirect()) {
          canReplace = false;
          break;
        }

        auto dstRgn = curDst;
        auto topdcl = dstRgn->getTopDcl();

        unsigned int lvnlb = lvnDst->getLeftBound();
        unsigned int lvnrb = lvnDst->getRightBound();

        unsigned int curlb = curDst->getLeftBound();
        unsigned int currb = curDst->getRightBound();

        if (topdcl == lvnDstTopDcl && ((lvnlb <= curlb && lvnrb >= curlb) ||
                                       (curlb <= lvnlb && currb >= lvnlb))) {
          canReplace = false;
          break;
        }

        if (lvnTopDclGRFAssigned &&
            curDst->getTopDcl()->getRegVar()->getPhyReg() != NULL) {
          canReplace = sameGRFRef(lvnDstTopDcl, curDst->getTopDcl());
          if (!canReplace) {
            break;
          }
        }
      }

      fwdInst_it++;
    }
  }

  if (canReplace) {
    // Ensure lvnInst's dst type and that of current inst is the same
    G4_Type lvnDstType = lvnDst->getType();
    G4_Type defType = def->getType();
    // For non-integers, they must have the same type to be replaced
    if (lvnDstType != defType &&
        (!IS_TYPE_INT(lvnDstType) || !IS_TYPE_INT(defType))) {
      canReplace = false;
    }
    // For integers, they must have the same type width to be replaced
    if ((IS_TYPE_INT(lvnDstType) && IS_TYPE_INT(defType)) &&
        TypeSize(lvnDstType) != TypeSize(defType)) {
      canReplace = false;
    }

    if (canReplace && lvnDst->getHorzStride() != def->getHorzStride()) {
      // Catch the case:
      // mov (8) V0<1>:d    V3
      // mov (8) V1<2>:d    V3
      // add (8) V2<1>:q    V2    V1<16;8,2>
      //
      // => Don't replace V1<16;8,2> with V0<8;8,1> because it would make
      // code HW non-conformant.
      canReplace = false;
    }
  }

  if (canReplace && lvnInst->getExecSize() < defInst->getExecSize()) {
    // Allow following case:
    // mov (8) V1(0,0) x -- x is an immdiate
    // op (8) ... V1(0,0)
    // mov (1) V2(0,0) x => remove
    // op (1) ... V2(0,0) => op (1) ... V1(0,0)
    //
    // But disallow following case:
    // mov (1) V1(0,0) x <-- lvnInst
    // op (1) ... V1(0,0)
    // mov (8) V2(0,0) x <-- defInst
    // op (8) ... V2(0,0) => Cant replace V2 with V1
    //

    canReplace = false;
  }

  if (canReplace && defInst->getExecSize() > g4::SIMD1) {
    // Check whether alignment matches for vectors
    // mov (8) V2(0,6)    V1(0,1) ... <-- lvnInst
    // ...
    // mov (8) V3(0,0)    V2(0,6)
    // op (8) V4          V3(0,0)   ...
    //
    // In general, V3(0,0) src in op cannot be replaced by
    // V2(0,6).
    if (lb % builder.numEltPerGRF<Type_UB>() !=
        lvnDst->getLeftBound() % builder.numEltPerGRF<Type_UB>()) {
      canReplace = false;
    }
  }

#ifdef DEBUG_LVN_ON
  if (canReplace) {
    for (auto use_it = uses.begin(); use_it != uses.end(); use_it++) {
      (*use_it).first->emit(std::cerr);
      std::cerr << "\n";
    }

    std::cerr << "\n"
              << "\n"
              << "Replace this occurence?";
    scanf("%1s", &shouldReplace);

    canReplace = (shouldReplace[0] == 'y' ? true : false);
    if (canReplace)
      std::cerr << "Ok, will replace"
                << "\n";
  }
#endif

  return canReplace;
}

// transfer alignment of fromDcl to toDcl if the former is more restrictive
void LVN::transferAlign(G4_Declare *toDcl, G4_Declare *fromDcl) {
  if (!toDcl->isEvenAlign() && fromDcl->isEvenAlign()) {
    toDcl->setEvenAlign();
  }
  G4_SubReg_Align align1 = toDcl->getSubRegAlign();
  G4_SubReg_Align align2 = fromDcl->getSubRegAlign();

  // Compute most constrained sub-reg alignment and assign that
  // to lvnDst dcl since it will replace curDst based operands.
  if (align1 == align2) {
    return;
  }

  G4_SubReg_Align ret = std::max(align1, align2);
  toDcl->setSubRegAlign(ret);
}

void LVN::replaceAllUses(G4_INST *defInst, bool negate, UseList &uses,
                         G4_INST *lvnInst, bool keepRegion) {
  G4_Declare *dstTopDcl = lvnInst->getDst()->getTopDcl();
  const unsigned int regOff = lvnInst->getDst()->getRegOff();
  const unsigned int subRegOff = lvnInst->getDst()->getSubRegOff();

  // Ensure most constrained alignment gets applied to lvnInst's dst
  transferAlign(dstTopDcl, defInst->getDst()->getTopDcl());

  VISA_DEBUG({
    std::cout << "Inst with same value in LVN Table:\n";
    lvnInst->emit(std::cout);
    std::cout << "\nCurrent inst:\n";
    defInst->emit(std::cout);
    std::cout << "\nUses:\n";
  });

  for (const auto &use : uses) {
    G4_INST *useInst = use.first;
    G4_SrcRegRegion *srcToReplace =
        useInst->getOperand(use.second)->asSrcRegRegion();
    G4_SrcModifier srcMod = srcToReplace->getModifier();
    if (negate == true) {
      // Negate will be true only when value is based off an immediate operand
      // so there is no need to check for combination of src modifiers.
      if (srcMod == Mod_src_undef) {
        srcMod = Mod_Minus;
      } else if (srcMod == Mod_Minus) {
        srcMod = Mod_src_undef;
      } else {
        vISA_ASSERT_UNREACHABLE("Unexpected src modifier found in LVN");
      }
    }

    G4_SrcRegRegion *srcRgn = nullptr;
    if (keepRegion) {
      // new offset should include the offset between the use and its original
      // def
      vISA_ASSERT(srcToReplace->getLeftBound() >=
                 defInst->getDst()->getLeftBound(),
             "orig dst does not fully define use");
      int offsetFromOrigDst =
          srcToReplace->getLeftBound() - defInst->getDst()->getLeftBound();
      // we can replace the regVar directly without changing the rest of the
      // region
      auto typeSize = srcToReplace->getTypeSize();
      int offset = regOff * builder.numEltPerGRF<Type_UB>() +
                   subRegOff * lvnInst->getDst()->getTypeSize() +
                   offsetFromOrigDst;
      short newRegOff = offset / builder.numEltPerGRF<Type_UB>();
      short newSubRegOff =
          (offset % builder.numEltPerGRF<Type_UB>()) / typeSize;

      srcRgn = builder.createSrcRegRegion(
          srcMod, Direct, lvnInst->getDst()->getBase()->asRegVar(), newRegOff,
          newSubRegOff, srcToReplace->getRegion(), srcToReplace->getType());
    } else {
      unsigned short vstride = srcToReplace->getRegion()->vertStride;
      unsigned short width = srcToReplace->getRegion()->width;
      // Rule: If Width = 1, HorzStride must be 0 regardless of the values of
      //       ExecSize and VertStride.
      unsigned short hstride =
          (width == 1) ? 0 : getActualHStride(srcToReplace);
      G4_Type type = srcToReplace->getType();

      unsigned int subRegOffScaled =
          subRegOff * lvnInst->getDst()->getTypeSize() / TypeSize(type);

      srcRgn = builder.createSrcRegRegion(
          srcMod, Direct, lvnInst->getDst()->getBase()->asRegVar(),
          (short)regOff, (short)subRegOffScaled,
          builder.createRegionDesc(vstride, width, hstride), type);
    }

    if (srcToReplace->isAccRegValid()) {
      srcRgn->setAccRegSel(srcToReplace->getAccRegSel());
    }

    unsigned int srcIndex = G4_INST::getSrcNum(use.second);
    useInst->setSrc(srcRgn, srcIndex);
  }
}

void LVN::removeVirtualVarRedefs(G4_DstRegRegion *dst) {
  auto dstTopDcl = dst->getTopDcl();
  if (!dstTopDcl)
    return;

  auto it = dclValueTable.find(dstTopDcl);

  if (it != dclValueTable.end()) {
    for (auto second = it->second.begin(), end = it->second.end();
         second != end;) {
      auto potentialRedef = (*second);
#define IS_VAR_REDEFINED(origopnd, opnd)                                       \
  (((origopnd->getLeftBound() <= opnd->getLeftBound() &&                       \
     origopnd->getRightBound() >= opnd->getLeftBound()) ||                     \
    (opnd->getLeftBound() <= origopnd->getLeftBound() &&                       \
     opnd->getRightBound() >= origopnd->getLeftBound())))

      if (potentialRedef->inst && potentialRedef->inst->getDst() &&
          potentialRedef->inst->getDst()->getTopDcl() == dstTopDcl &&
          IS_VAR_REDEFINED(dst, potentialRedef->inst->getDst())) {
        potentialRedef->active = false;

        for (auto use : potentialRedef->uses) {
          use->active = false;
        }
      } else {
        for (unsigned int i = 0, numSrc = potentialRedef->inst->getNumSrc();
             i < numSrc; i++) {
          if (potentialRedef->inst && potentialRedef->inst->getSrc(i) &&
              potentialRedef->inst->getSrc(i)->getTopDcl() == dstTopDcl &&
              IS_VAR_REDEFINED(dst, potentialRedef->inst->getSrc(i))) {
            potentialRedef->active = false;

            for (auto use : potentialRedef->uses) {
              use->active = false;
            }
          }
        }
      }

      second++;
    }
  }

  if (dst->getTopDcl()->getAddressed()) {
    // Iterate over entire LVN table and remove any instruction with
    // src indirect operand that point to current dst. For eg,
    // A0 = &V10
    // V20 = r[A0] <-- inst1
    // ...
    // V10 = 0 <-- Current instruction - Invalidate inst1
    // V30 = r[A0] <-- inst1 != this inst
    for (auto &dcls : lvnTable) {
      for (auto second = dcls.second.begin(); second != dcls.second.end();) {
        auto lvnItems = (*second);
        auto lvnItemsInst = lvnItems->inst;

        if (lvnItemsInst) {
          for (unsigned int i = 0, numSrc = lvnItemsInst->getNumSrc();
               i < numSrc; i++) {
            if (lvnItemsInst->getSrc(i) &&
                lvnItemsInst->getSrc(i)->isSrcRegRegion() &&
                lvnItemsInst->getSrc(i)->asSrcRegRegion()->isIndirect()) {
              if (p2a.isPresentInPointsTo(lvnItemsInst->getSrc(i)
                                              ->asSrcRegRegion()
                                              ->getTopDcl()
                                              ->getRegVar(),
                                          dst->getTopDcl()->getRegVar())) {
                lvnItems->active = false;
                second = dcls.second.erase(second);

                for (auto use : lvnItems->uses) {
                  use->active = false;
                }
              }
            }
          }
        }

        if (second != dcls.second.end()) {
          // Increment iterator only if dcls.second is not empty.
          // Erase operation in earlier loop can result in this.
          second++;
        }
      }
    }
  }
}

bool LVN::sameGRFRef(G4_Declare *dcl1, G4_Declare *dcl2) {
  bool overlap = false;

  unsigned int dst1LowGRF =
      dcl1->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  unsigned int dst1HighGRF = dst1LowGRF + (dcl1->getNumRows());

  unsigned int dst2LowGRF =
      dcl2->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  unsigned int dst2HighGRF = dst2LowGRF + (dcl2->getNumRows());

  if (dst1LowGRF <= dst2HighGRF && dst1HighGRF >= dst2LowGRF) {
    overlap = true;
  }

  return overlap;
}

// Function assumes that dst's G4_RegVar has pre-defined
// physical register. We remove all those entries from
// LVN table that refer to same GRF.
void LVN::removePhysicalVarRedefs(G4_DstRegRegion *dst) {
  G4_Declare *topdcl = dst->getTopDcl();
  for (auto& all : lvnTable) {
    for (auto it = all.second.begin(); it != all.second.end();) {
      auto item = (*it);
      bool erase = false;

      auto dstTopDcl = item->inst->getDst()->getTopDcl();
      if (dstTopDcl->getRegVar()->isGreg()) {
        if (sameGRFRef(topdcl, dstTopDcl)) {
          item->active = false;
          erase = true;
        }
      }

      for (auto i = 0; i < item->inst->getNumSrc(); i++) {
        auto srcTopDcl = item->inst->getSrc(i)->getTopDcl();
        if (srcTopDcl && srcTopDcl->getRegVar()->isGreg()) {
          // Check if both physical registers have an overlap
          if (sameGRFRef(topdcl, srcTopDcl)) {
            item->active = false;
            erase = true;
          }
        }
      }

      if (erase) {
        it = all.second.erase(it);
        continue;
      }

      it++;
    }
  }
}

void LVN::removeFlagVarRedefs(G4_Declare *topdcl) {

  for (auto &all : lvnTable) {
    for (auto it = all.second.begin(); it != all.second.end();) {
      auto item = (*it);
      bool erase = false;

      auto dstTopDcl = item->inst->getDst()->getTopDcl();
      if (dstTopDcl->getRegVar()->isFlag()) {
        if (dstTopDcl == topdcl) {
          item->active = false;
          erase = true;
        }
      }

      if (erase) {
        it = all.second.erase(it);
        continue;
      }

      it++;
    }
  }
}

bool LVN::checkIfInPointsTo(const G4_RegVar *addr, const G4_RegVar *var) const {
  // Check whether var is present in points2 of addr
  auto ptrToAllPointsTo = p2a.getAllInPointsTo(addr);
  if (ptrToAllPointsTo) {
    const G4_RegVar *topRegVar =
        var->getDeclare()->getRootDeclare()->getRegVar();
    auto it =
        std::find_if(ptrToAllPointsTo->begin(), ptrToAllPointsTo->end(),
                     [&topRegVar](const pointInfo &element) {
                       return element.var == topRegVar && element.off == 0;
                     });

    return it != ptrToAllPointsTo->end();
  }

  return false;
}

void LVN::removeAliases(G4_INST *inst) {
  // inst uses indirect dst operand
  vISA_ASSERT(inst->getDst()->isIndirect(),
               "Expecting to see indirect operand in dst");

  auto dstPointsToPtr =
      p2a.getAllInPointsTo(inst->getDst()->getTopDcl()->getRegVar());
  if (!dstPointsToPtr)
    return;

  for (const auto &item : *dstPointsToPtr) {
    auto dcl = item.var->getDeclare()->getRootDeclare();
    auto it = dclValueTable.find(dcl);
    if (it == dclValueTable.end())
      continue;

    for (auto d : it->second) {
      d->active = false;
      VISA_DEBUG({
        std::cout << "Removing inst from LVN table for indirect dst conflict:";
        d->inst->emit(std::cerr);
        std::cout << "Due to indirect dst in:"
                  << "\n";
        inst->emit(std::cerr);
        std::cout << "\n";
      });
    }
  }
}

void LVN::removeRedefs(G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();
  if (dst) {
    if (dst->isIndirect()) {
      removeAliases(inst);
    }

    G4_DstRegRegion *dstRegRegion = dst;
    removeVirtualVarRedefs(dstRegRegion);

    G4_Declare *dstTopDcl = dstRegRegion->getTopDcl();
    if (dstTopDcl && dstTopDcl->getRegVar()->isGreg()) {
      removePhysicalVarRedefs(dstRegRegion);
    }
  }

  G4_CondMod *condMod = inst->getCondMod();
  if (condMod && condMod->getBase() && condMod->getBase()->isRegVar()) {
    G4_Declare *condDcl = condMod->getTopDcl();
    if (condDcl && condDcl->getRegVar()->isFlag()) {
      removeFlagVarRedefs(condDcl);
    }
  }

  G4_Predicate *predicate = inst->getPredicate();
  if (predicate && predicate->getBase() && predicate->getBase()->isRegVar()) {
    G4_Declare *predDcl = predicate->getTopDcl();
    if (predDcl && predDcl->getRegVar()->isFlag()) {
      removeFlagVarRedefs(predDcl);
    }
  }
}
int64_t LVN::getNegativeRepresentation(int64_t imm, G4_Type type) {
  union {
    double ddata;
    float fdata;
    char bdata;
    short wdata;
    int dwdata;
    int64_t lldata;
  } d;
  d.lldata = imm;

  if (type == Type_B || type == Type_UB) {
    d.bdata = -d.bdata;
  } else if (type == Type_W || type == Type_UW) {
    d.wdata = -d.wdata;
  } else if (type == Type_D || type == Type_UD) {
    d.dwdata = -d.dwdata;
  } else if (type == Type_Q || type == Type_UQ) {
    d.lldata = -d.lldata;
  } else if (type == Type_F) {
    d.fdata = -d.fdata;
  } else if (type == Type_DF) {
    d.ddata = -d.ddata;
  } else if (type == Type_HF) {
    d.wdata = d.wdata ^ (1 << 15);
  }

  return d.lldata;
}

const char *LVN::getModifierStr(G4_SrcModifier srcMod) {
  if (srcMod == Mod_src_undef) {
    return "";
  } else if (srcMod == Mod_Minus) {
    return "NEG";
  } else if (srcMod == Mod_Abs) {
    return "ABS";
  } else if (srcMod == Mod_Minus_Abs) {
    return "NEGABS";
  } else if (srcMod == Mod_Not) {
    return "NOT";
  } else {
    return "";
  }
}

void LVN::getValue(int64_t imm, G4_Operand *opnd, Value &value) {
  value.hash = imm;
  value.inst = nullptr;
}

LVNItemInfo *LVN::getOpndValue(G4_Operand *opnd, bool create) {
  auto topdcl = opnd->getTopDcl();
  if (!topdcl)
    return nullptr;

  auto lb = opnd->getLeftBound();
  auto rb = opnd->getRightBound();
  uint16_t hs = 0;
  bool isScalar = false;
  bool isSingleStride = false;
  if (opnd->isSrcRegRegion()) {
    isScalar = opnd->asSrcRegRegion()->getRegion()->isScalar();
    if (!isScalar) {
      isSingleStride = opnd->asSrcRegRegion()->getRegion()->isSingleStride(
          opnd->getInst()->getExecSize(), hs);
      if (!isSingleStride)
        hs = 0;
    }
  } else if (opnd->isDstRegRegion()) {
    isScalar = opnd->getInst()->getExecSize() == g4::SIMD1;
    if (!isScalar) {
      hs = opnd->asDstRegRegion()->getHorzStride();
      isSingleStride = true;
    }
  }

  auto it = dclValueTable.find(topdcl);
  if (it == dclValueTable.end()) {
    if (!create) {
      return nullptr;
    }
  }

  if (it != dclValueTable.end()) {
    for (auto &item : (*it).second) {
      if (!item->active)
        continue;

      if (item->lb == lb && item->rb == rb && item->isScalar == isScalar &&
          item->constHStride == isSingleStride && item->hstride == hs)
        return item;
    }
  }

  if (create) {
    // create instance of LVNItemInfo since it doesnt exist
    LVNItemInfo *item = createLVNItemInfo();
    item->constHStride = isSingleStride;
    item->hstride = hs;
    item->inst = opnd->getInst();
    item->opnd = opnd;
    item->isScalar = isScalar;
    item->lb = lb;
    item->rb = rb;

    return item;
  }

  return nullptr;
}

void LVN::getValue(G4_SrcRegRegion *src, G4_INST *inst, Value &value) {
  G4_Declare *topdcl = src->getTopDcl();

  // check whether operand is present in value table
  auto lvnItemInfo = getOpndValue(src);
  if (lvnItemInfo->value.isValueEmpty()) {
    lvnItemInfo->value.hash = topdcl->getDeclId() + src->getLeftBound() +
                              src->getRightBound() + getActualHStride(src) +
                              dclValueTable.size();
    lvnItemInfo->value.inst = inst;
    perInstValueCache.push_back(std::make_pair(topdcl, lvnItemInfo));
  }
  value = lvnItemInfo->value;
}

void LVN::getValue(G4_DstRegRegion *dst, G4_INST *inst, Value &value) {
  G4_Declare *topdcl = dst->getTopDcl();

  auto lvnItemInfo = getOpndValue(dst);
  if (lvnItemInfo->value.isValueEmpty()) {
    lvnItemInfo->value.hash = topdcl->getDeclId() + dst->getLeftBound() +
                              dst->getRightBound() + dst->getHorzStride() +
                              dclValueTable.size();
    lvnItemInfo->value.inst = inst;
    perInstValueCache.push_back(std::make_pair(topdcl, lvnItemInfo));
  }
  value = lvnItemInfo->value;
}

void LVN::invalidateOldDstValue(G4_INST *inst) {
  // invalidate old values
  if (!inst->getDst())
    return;
  auto topdcl = inst->getDst()->getTopDcl();
  if (!topdcl)
    return;
  auto it = dclValueTable.find(topdcl);
  if (it == dclValueTable.end())
    return;

  auto lb = inst->getDst()->getLeftBound();
  auto rb = inst->getDst()->getRightBound();
  for (auto &item : (*it).second) {
    if (!item->active)
      continue;

    if (item->lb < rb && item->rb >= rb)
      item->active = false;

    if (lb < item->rb && rb >= item->rb)
      item->active = false;
  }
}

void LVN::getValue(G4_INST *inst, Value &value) {
  value.inst = inst;
  value.hash = (unsigned int)inst->opcode();
  for (unsigned int i = 0; i != inst->getNumSrc(); i++) {
    auto opnd = inst->getSrc(i);

    Value v;
    if (opnd->isSrcRegRegion()) {
      getValue(opnd->asSrcRegRegion(), inst, v);
      value.hash += v.hash;
    } else if (opnd->isImm()) {
      getValue(opnd->asImm()->getInt(), opnd, v);
      value.hash += v.hash;
    }
  }
}

template <class T, class K> bool LVN::opndsMatch(T *opnd1, K *opnd2) {
  bool match = true;

  // T,K can either be G4_SrcRegRegion/G4_DstRegRegion
  G4_Declare *topdcl1 = opnd1->getTopDcl();
  G4_Declare *topdcl2 = opnd2->getTopDcl();

  G4_INST *inst1 = opnd1->getInst();
  G4_INST *inst2 = opnd2->getInst();
  // Compare emask for opnd1, opnd2 instructions
  if (!bb->isAllLaneActive()) {
    if (inst1->isWriteEnableInst() != inst2->isWriteEnableInst()) {
      match = false;
    } else {
      if (inst1->getMaskOffset() != inst2->getMaskOffset() &&
          !inst1->isWriteEnableInst()) {
        match = false;
      }
    }
  } else {
    // Not in SIMD CF so don't care
  }

  if (match) {
    const char *mod1 = getModifierStr(Mod_src_undef);
    const char *mod2 = mod1;
    if (opnd1->isSrcRegRegion()) {
      mod1 = getModifierStr(opnd1->asSrcRegRegion()->getModifier());
    }

    if (opnd2->isSrcRegRegion()) {
      mod2 = getModifierStr(opnd2->asSrcRegRegion()->getModifier());
    }

    if (mod1 != mod2) {
      match = false;
    }
  }

  if (match) {
    if (opnd1->isIndirect() != opnd2->isIndirect()) {
      match = false;
    } else {
      // In this branch, both operands are either direct or indirect
      if (opnd1->isIndirect()) {
        if (opnd1->isSrcRegRegion() && opnd2->isSrcRegRegion()) {
          G4_SrcRegRegion *src1 = opnd1->asSrcRegRegion();
          G4_SrcRegRegion *src2 = opnd2->asSrcRegRegion();

          if (topdcl1 != topdcl2 || src1->getRegOff() != src2->getRegOff() ||
              src1->getSubRegOff() != src2->getSubRegOff() ||
              src1->getAddrImm() != src2->getAddrImm() ||
              getActualHStride(src1) != getActualHStride(src2) ||
              src1->getInst()->getExecSize() !=
                  src2->getInst()->getExecSize() ||
              src1->getType() != src2->getType()) {
            match = false;
          }
        } else {
          // opnd1 is a src region and opnd2 is dst region or
          // vice-versa.
          match = false;
        }
      } else {
        // opnd1/opnd2 can be either src/dst regions and may not of the same
        // type.
        if (topdcl1 != topdcl2 || opnd1->getType() != opnd2->getType()) {
          match = false;
        } else {
          int op1lb = 0, op2lb = 0, op1rb = 0, op2rb = 0, op1hs = 0, op2hs = 0;
          if (opnd1->isSrcRegRegion()) {
            G4_SrcRegRegion *src1 = opnd1->asSrcRegRegion();
            op1lb = src1->getLeftBound();
            op1rb = src1->getRightBound();
            op1hs = getActualHStride(src1);
          } else if (opnd1->isDstRegRegion()) {
            G4_DstRegRegion *dst1 = opnd1->asDstRegRegion();
            op1lb = dst1->getLeftBound();
            op1rb = dst1->getRightBound();
            op1hs = dst1->getHorzStride();
          }

          if (opnd2->isSrcRegRegion()) {
            G4_SrcRegRegion *src2 = opnd2->asSrcRegRegion();
            op2lb = src2->getLeftBound();
            op2rb = src2->getRightBound();
            op2hs = getActualHStride(src2);
          } else if (opnd2->isDstRegRegion()) {
            G4_DstRegRegion *dst2 = opnd2->asDstRegRegion();
            op2lb = dst2->getLeftBound();
            op2rb = dst2->getRightBound();
            op2hs = dst2->getHorzStride();
          }

          if (op1lb != op2lb || op1rb != op2rb || op1hs != op2hs) {
            match = false;
          }
        }
      }
    }
  }

  return match;
}

// Given a G4_Operand, this function returns false if,
// operand is a src region that is either scalar <0;1,0> or
// has contiguous region like <8;8,1>. Otherwise, if
// src region is uniform like <8;4,2> then function returns
// false. Only in case of non-uniform region the function
// returns true. Non-uniform regions are regions that
// have overlapping elements or hstride between elements
// in not uniform.
//
// Examples of non-uniform region are
// (4) <1;2,2>, (8) <2;4,1>, (8) <8;4,1>.
// The last one <8;4,1> is actually used in FRC.
bool isNonUniformSrcRegion(G4_SrcRegRegion *srcRgn) {
  auto execSize = srcRgn->getInst()->getExecSize();
  return !srcRgn->getRegion()->isSingleStride(execSize);
}

bool LVN::addValue(G4_INST *inst) {
  auto isAllowedOpcode = [](G4_INST *inst) {
    if (inst->opcode() == G4_mov || inst->opcode() == G4_shl ||
        inst->opcode() == G4_shr)
      return true;
    return false;
  };

  if (!isAllowedOpcode(inst) || inst->getSaturate() ||
      inst->getDst()->isIndirect() || inst->getPredicate() != NULL ||
      inst->getCondMod() != NULL) {
    return false;
  }

  G4_Operand *dst = inst->getDst();
  if (!dst->getBase() || !dst->getBase()->isRegVar() ||
      (dst->getBase()->asRegVar()->getDeclare()->getRegFile() != G4_GRF &&
       !(builder.doFlagLVN() &&
         dst->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_FLAG)) ||
      dst->getTopDcl()->getAddressed()) {
    return false;
  }

  // Only handle mov flag, imm
  if (dst->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_FLAG) {
    if (inst->opcode() != G4_mov) {
      return false;
    }
    if (!inst->getSrc(0)->isImm()) {
      return false;
    }
  }

  if (dst->getBase() && dst->getTopDcl()->isOutput()) {
    return false;
  }

  auto srcValid = [](G4_Operand *src) {
    if (src->isImm()) {
      if (src->isRelocImm()) {
        return false;
      }
      return true;
    }

    if (src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
      if (!srcRgn->getBase() || !srcRgn->getBase()->isRegVar() ||
          (!srcRgn->getBase()->asRegVar()->getDeclare()->useGRF() &&
           !srcRgn->isIndirect())) {
        return false;
      }

      if (srcRgn->isIndirect()) {
        return false;
      }

      if (srcRgn->getBase() && srcRgn->getTopDcl()->isOutput()) {
        return false;
      }
      return true;
    }

    return true;
  };

  bool allSrcsValid = true;
  for (unsigned int i = 0; i != inst->getNumSrc(); i++) {
    auto src = inst->getSrc(i);
    allSrcsValid &= srcValid(src);
  }

  return allSrcsValid;
}

bool LVN::computeValue(G4_INST *inst, bool negate, bool &canNegate,
                       bool &isGlobal, int64_t &tmpPosImm, bool posValValid,
                       Value &value) {
  canNegate = false;
  isGlobal = false;
  value.initializeEmptyValue();

  G4_Operand *src = inst->getSrc(0);
  LVNItemInfo *item = nullptr;

  if (inst->opcode() == G4_mov && src->isImm()) {
    if (src->isImm()) {
      int64_t dstImm = 0;
      G4_Type srcType = src->asImm()->getType();
      G4_Type dstType = inst->getDst()->getType();

      if (negate && posValValid) {
        dstImm =
            getNegativeRepresentation(tmpPosImm, inst->getDst()->getType());
      } else {
        G4_Imm *srcImmOpnd = src->asImm();
        int64_t srcImmData = srcImmOpnd->getImm();
        bool success =
            getDstData(srcImmData, srcType, dstImm, dstType, canNegate);
        tmpPosImm = dstImm;
        if (success == false) {
          // This means mov was unsupported for optimization,
          // eg, a type-conversion mov.
          return false;
        }

        if (negate == true) {
          dstImm = getNegativeRepresentation(dstImm, dstType);
        }
      }

      getValue(dstImm, src, value);
      if (!negate) {
        value.inst = inst;

        // Substituting negative imm values is an optimization case
        item = createLVNItemInfo();
        item->isImm = true;
        item->constHStride = true;
        item->hstride = inst->getDst()->getHorzStride();
        item->inst = inst;
        item->opnd = inst->getDst();
        item->isScalar = inst->getExecSize() == g4::SIMD1;
        item->lb = inst->getDst()->getLeftBound();
        item->rb = inst->getDst()->getRightBound();
        item->value = value;
      }
    }
  } else {
    getValue(inst, value);
    item = createLVNItemInfo();
    item->constHStride = true;
    item->hstride = inst->getDst()->getHorzStride();
    item->inst = inst;
    item->opnd = inst->getDst();
    item->isScalar = inst->getExecSize() == g4::SIMD1;
    item->lb = inst->getDst()->getLeftBound();
    item->rb = inst->getDst()->getRightBound();
    item->value = value;
  }

  if (item && inst->getDst() && inst->getDst()->getTopDcl()) {
    perInstValueCache.push_back(
        std::make_pair(inst->getDst()->getTopDcl(), item));
  }

  if (inst->getDst() && inst->getDst()->getTopDcl()) {
    // Compute value for globals so we can insert it in LVN table.
    // But we don't want to apply optimization on such instructions.
    isGlobal = fg.globalOpndHT.isOpndGlobal(inst->getDst());
    isGlobal |= inst->getDst()->getTopDcl()->isOutput();
    isGlobal |= inst->getDst()->getTopDcl()->isInput();
  }

  return true;
}

// Ordering of val1 and val2 parameters is important.
// val1 is current instruction in top-down order.
// val2 is earlier instruction having operand with same value.
// Ordering is important because we allow optimization when
// val2 instruction uses NoMask and val1 instruction uses
// a subset.
bool LVN::valuesMatch(Value &val1, Value &val2, bool checkNegImm) {
  auto inst1 = val1.inst;
  auto inst2 = val2.inst;

  if (inst1->opcode() != inst2->opcode())
    return false;

  bool match = false;

  uint32_t numSrc = std::max(inst1->getNumSrc(), inst2->getNumSrc());
  for (unsigned int i = 0; i != numSrc; i++) {
    G4_Operand *opnd1 = inst1->getSrc(i);
    G4_Operand *opnd2 = inst2->getSrc(i);

    if ((!opnd1 && opnd2) || (opnd1 && !opnd2))
      return false;

    if (!opnd1 && !opnd2)
      continue;

    if (opnd1->isImm() && opnd2->isImm()) {
      match = true;
      G4_Type type1 = opnd1->getType();
      G4_Type type2 = opnd2->getType();

      if ((type1 == Type_UV && type2 != Type_UV) ||
          (type1 != Type_UV && type2 == Type_UV)) {
        return false;
      } else if ((type1 == Type_V && type2 != Type_V) ||
                 (type1 != Type_V && type2 == Type_V)) {
        return false;
      } else if ((type1 == Type_VF && type2 != Type_VF) ||
                 (type1 != Type_VF && type2 == Type_VF)) {
        return false;
      }

      if (match && !bb->isAllLaneActive()) {
        G4_INST *val1Inst = val1.inst;
        G4_INST *val2Inst = val2.inst;

        if (val1Inst->isWriteEnableInst() != val2Inst->isWriteEnableInst() ||
            // use maskOffset rather than maskOption as some masks are
            // considered default and not explicitly set in mask option
            val1Inst->getMaskOffset() != val2Inst->getMaskOffset()) {
          return false;
        }
      }

      if (!checkNegImm) {
        if (!opnd1->asImm()->isEqualTo(opnd2->asImm())) {
          return false;
        }
      } else {
        auto imm1 = opnd1->asImm()->getImm();
        auto imm2 = opnd2->asImm()->getImm();
        auto bitSize = TypeBitSize(type1);
        // Check all bits except for sign-bit.
        // Assume sign-bit is always the MSB of type.
        for (unsigned int i = 0; i != bitSize - 1; i++) {
          if (((imm1 >> i) & 0x1) != ((imm2 >> i) & 0x1)) {
            return false;
          }
        }
      }
    } else if (opnd1->isSrcRegRegion()) {
      if (opnd2->isSrcRegRegion()) {
        match = opndsMatch(opnd1->asSrcRegRegion(), opnd2->asSrcRegRegion());
      }
    }

    if (!match)
      return false;
  }

  return match;
}

bool LVN::isSameValue(Value &val1, Value &val2, bool negImmVal) {
  if (val1.isEqualValueHash(val2)) {
    // Do a detailed check whether values really match
    if (valuesMatch(val1, val2, negImmVal)) {
      return true;
    }
  }

  return false;
}

LVNItemInfo *LVN::isValueInTable(Value &value, bool negate) {
  int64_t hash = value.hash;

  auto bucket = lvnTable.find(hash);
  if (bucket != lvnTable.end()) {
    for (auto it = bucket->second.rbegin(); it != bucket->second.rend();) {
      auto lvnItem = (*it);

      if (lvnItem->active && isSameValue(value, lvnItem->value, negate)) {
        return lvnItem;
      }

      it++;
    }
  }

  return nullptr;
}

void LVN::addValueToTable(G4_INST *inst, Value &oldValue) {
  auto findLVNItemInfo = [this](G4_INST *inst, G4_Operand *opnd) {
    auto it = dclValueTable.find(opnd->getTopDcl());
    vISA_ASSERT(it != dclValueTable.end(), "Value not added");
    LVNItemInfo *lvnItem = nullptr;
    for (auto item : (*it).second) {
      if (item->inst == inst && opnd->getInst() == item->inst) {
        return item;
      }
    }
    vISA_ASSERT(lvnItem != nullptr, "Not expecting nullptr");

    return lvnItem;
  };

  auto insertInLvnTable = [this](int64_t key, LVNItemInfo *itemToIns) {
    auto it = lvnTable.find(key);
    if (it != lvnTable.end()) {
      it->second.push_back(itemToIns);
    } else {
      std::list<vISA::LVNItemInfo *> second;
      second.push_back(itemToIns);
      lvnTable.insert(make_pair(key, second));
    }
    itemToIns->active = true;
  };

  auto attachUses = [this](G4_INST *inst, LVNItemInfo *lvnItem) {
    // scan all dcls of lvnItem->inst and link
    // all their values in to this lvnItem
    // this is required because G4 IR is not SSA so
    // a redef needs to invalidate earlier computed
    // values that use the older def. for eg,
    // V10 = ... <-- value1
    // V11 = V10 <-- value2
    // V10 = V10 + 1  <-- value3 (invalidate value2)
    // V12 = V10 <-- needs to use value3, not value2
    for (unsigned int i = 0; i != inst->getNumSrc(); i++) {
      auto src = inst->getSrc(i);
      if (src && src->isSrcRegRegion()) {
        auto srcLb = src->getLeftBound();
        auto srcRb = src->getRightBound();
        auto it = dclValueTable.find(src->getTopDcl());
        if (it != dclValueTable.end()) {
          for (auto l : (*it).second) {
            if (l->active) {
              auto lb = l->lb;
              auto rb = l->rb;
              if (lb <= srcRb && rb >= srcLb) {
                l->uses.push_back(lvnItem);
              }
            }
          }
        }
      }
    }
  };

  for (auto &item : perInstValueCache) {
    item.second->active = true;
    dclValueTable[item.first].push_back(item.second);
  }

  auto lvnItem = findLVNItemInfo(inst, inst->getDst());
  insertInLvnTable(oldValue.hash, lvnItem);
  attachUses(inst, lvnItem);

  vISA_ASSERT(lvnItem->inst == lvnItem->value.inst,
               "Missing inst ptr in value");
}

void LVN::addUse(G4_DstRegRegion *dst, G4_INST *use, unsigned int srcIndex) {
  Gen4_Operand_Number srcPos = Opnd_dst;
  if (srcIndex == 0) {
    srcPos = Opnd_src0;
  } else if (srcIndex == 1) {
    srcPos = Opnd_src1;
  } else if (srcIndex == 2) {
    srcPos = Opnd_src2;
  } else if (srcIndex == 10) { // Must be not valid src index
    srcPos = Opnd_condMod;
  } else if (srcIndex == 11) {
    srcPos = Opnd_pred;
  }

  UseInfo useInst = {use, srcPos};
  auto it = defUse.find(dst->getInst());

  if (it == defUse.end()) {
    UseList uses = {useInst};

    defUse.insert(std::make_pair(dst->getInst(), uses));
  } else {
    (*it).second.push_back(useInst);
  }

  if (srcIndex >= 10) {
    return;
  }

  auto srcOpnd = use->getSrc(srcIndex);
  auto itUD = useDef.find(srcOpnd);
  if (itUD == useDef.end()) {
    DefList defs = {dst->getInst()};
    useDef.insert(std::make_pair(srcOpnd, defs));
  } else {
    (*itUD).second.push_back(dst->getInst());
  }
}

void LVN::removeAddrTaken(G4_AddrExp *opnd) {
  G4_Declare *opndTopDcl = opnd->getRegVar()->getDeclare()->getRootDeclare();

  auto range_it = activeDefs.equal_range(opndTopDcl->getDeclId());
  for (auto it = range_it.first; it != range_it.second;) {
    auto prev_it = it;
    (*prev_it).second.second->getInst()->removeAllUses();
    it++;
    activeDefs.erase(prev_it);
  }
}

void LVN::removeFlag(G4_Declare *topDcl) {
  auto range_it = activeDefs.equal_range(topDcl->getDeclId());
  for (auto it = range_it.first; it != range_it.second;) {
    auto prev_it = it;
    (*prev_it).second.second->getInst()->removeAllUses();
    it++;
    activeDefs.erase(prev_it);
  }
}

void LVN::populateDuTable(INST_LIST_ITER inst_it) {
  duTablePopulated = true;
  // Populate duTable from inst_it position
  ActiveDefMMap activeDefs;
  G4_INST *startInst = (*inst_it);
  G4_Operand *startInstDst = startInst->getDst();
  INST_LIST_ITER lastInstIt = bb->end();
  unsigned int numEdgesAdded = 0;

  while (inst_it != lastInstIt) {
    G4_INST *curInst = (*inst_it);
    // First scan src operands and check if their def has been
    // added to activeDefs table already. If so, link them.
    for (unsigned int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
      G4_Operand *opnd = curInst->getSrc(i);
      if (!opnd)
        continue;

      if (opnd->isAddrExp()) {
        // Since this operand is address taken,
        // remove any defs corresponding to it
        // in DU and activeDefs tables.
        removeAddrTaken(opnd->asAddrExp());
      }

      if (opnd->isSrcRegRegion()) {
        G4_Declare *topdcl = opnd->getTopDcl();
        if (topdcl != NULL) {
          auto range_it = activeDefs.equal_range(topdcl->getDeclId());
          if (range_it.first == range_it.second) {
            // No match found so move on to next src opnd
            continue;
          }

          unsigned int lb = opnd->getLeftBound();
          unsigned int rb = opnd->getRightBound();
          unsigned int hs = getActualHStride(opnd->asSrcRegRegion());

          auto start_it = range_it.second;
          start_it--;
          bool done = false;
          for (auto it = start_it;;) {
            G4_DstRegRegion *activeDst = (*it).second.second;

            unsigned int lb_dst = activeDst->getLeftBound();
            unsigned int rb_dst = activeDst->getRightBound();
            unsigned int hs_dst = activeDst->getHorzStride();

            if (lb_dst <= rb && rb_dst >= lb) {
              // Early return case
              // This function is entered when an existing
              // value is found in LVN table. startInst is
              // the instruction that is expected to be
              // optimized away. However, if it so happens
              // that startInst defs a partial subset of
              // data flowing in to a src operand, we can
              // postpone doing DU for the BB. This helps
              // large BBs with false starts for du
              // computation.
              if (activeDst == startInstDst && numEdgesAdded == 0) {
                if (!(lb_dst <= lb && rb_dst >= rb)) {
                  duTablePopulated = false;
                  return;
                }
              }

              addUse(activeDst, curInst, i);
              numEdgesAdded++;

              if (activeDst->getInst()->getPredicate() == NULL &&
                  (lb_dst <= lb && rb_dst >= rb &&
                   (hs_dst == hs ||
                    (hs_dst == 1 && hs == 0 &&
                     activeDst->getInst()->getExecSize() == g4::SIMD1)))) {
                // Active def (in bottom-up order)
                // fully defines use of current inst. So no
                // need to analyze earlier appearing defs.
                break;
              }
            }

            if (done || it == range_it.first) {
              // Last match reached
              break;
            }
            it--;
          }
        }
      }
    }

    G4_DstRegRegion *dst = curInst->getDst();
    if (dst) {
      G4_Declare *curDstTopDcl = dst->getTopDcl();
      if (curDstTopDcl != NULL) {
        // Check if already an overlapping dst region is active
        auto range_it = activeDefs.equal_range(curDstTopDcl->getDeclId());
        for (auto it = range_it.first; it != range_it.second;) {
          G4_DstRegRegion *activeDst = (*it).second.second;

          unsigned int lb = dst->getLeftBound();
          unsigned int rb = dst->getRightBound();
          unsigned int hs = dst->getHorzStride();

          unsigned int lb_dst = activeDst->getLeftBound();
          unsigned int rb_dst = activeDst->getRightBound();
          unsigned int hs_dst = activeDst->getHorzStride();

          if ((lb <= lb_dst && rb >= rb_dst) && hs == hs_dst) {
            if (curInst->getPredicate() == NULL) {
              // Current dst completely overlaps
              // earlier def so retire earlier
              // active def.
              auto prev_it = it;
              it++;
              activeDefs.erase(prev_it);
              continue;
            }
          }

          it++;
        }

        bool addValueToDU = false;
        if (addValue(curInst)) {
          addValueToDU = true;
        }

        if (addValueToDU ||
            (activeDefs.find(curDstTopDcl->getDeclId()) != activeDefs.end())) {
          // mov (8) V10(0,0):d     r0.0:d - 1
          // shr (1) V10(0,1):d     0x1:d  - 2
          // send ... V10 ...              - 3
          //
          // Since inst 1 is a valid LVN candidate it will be inserted
          // in activeDefs table. Inst 2 is not an LVN candidate but
          // it clobbers def of inst 1, hence we have to insert it in
          // the LVN table too. If first instruction wasnt an LVN
          // candidate by itself, we wouldnt insert it or inst 2 in
          // active def table.
          ActiveDef newActiveDef;
          newActiveDef.first = curDstTopDcl;
          newActiveDef.second = dst;
          activeDefs.emplace(curDstTopDcl->getDeclId(), newActiveDef);
        }
      }
    }
    G4_CondMod *condMod = curInst->getCondMod();
    if (condMod && condMod->getBase() && condMod->getBase()->isRegVar()) {
      G4_Declare *condDcl = condMod->getTopDcl();
      if (condDcl && condDcl->getRegVar()->isFlag()) {
        auto range_it = activeDefs.equal_range(condDcl->getDeclId());
        if (range_it.first != range_it.second) {
          auto start_it = range_it.second;
          start_it--;
          for (auto it = start_it;;) {
            G4_DstRegRegion *activeDst = (*it).second.second;
            addUse(activeDst, curInst, 10);
            if (it == range_it.first) {
              // Last match reached
              break;
            }
            it--;
          }
        }
        removeFlag(condDcl);
      }
    }

    G4_Predicate *predicate = curInst->getPredicate();
    if (predicate && predicate->getBase() && predicate->getBase()->isRegVar()) {
      G4_Declare *predDcl = predicate->getTopDcl();
      if (predDcl && predDcl->getRegVar()->isFlag()) {
        auto range_it = activeDefs.equal_range(predDcl->getDeclId());
        if (range_it.first != range_it.second) {
          auto start_it = range_it.second;
          start_it--;
          for (auto it = start_it;;) {
            G4_DstRegRegion *activeDst = (*it).second.second;
            addUse(activeDst, curInst, 11);
            if (it == range_it.first) {
              // Last match reached
              break;
            }
            it--;
          }
        }
      }
    }

    inst_it++;
  }
}

void LVN::invalidate() {
  // invalidate all values cached in value table
  for (auto &bucket : lvnTable) {
    for (auto &lvnItem : bucket.second) {
      lvnItem->active = false;
    }
  }
}

void LVN::doLVN() {
  bb->resetLocalIds();
  for (INST_LIST_ITER inst_it = bb->begin(), inst_end_it = bb->end();
       inst_it != inst_end_it; inst_it++) {
    G4_INST *inst = (*inst_it);
    bool negMatch = false;

    if (inst->isOptBarrier()) {
      // disallow optimization across opt barrier
      invalidate();
      continue;
    }

    if (inst->getDst() == NULL) {
      continue;
    }

    Value value, oldValue;
    bool canNegate = false, isGlobal = false;
    G4_INST *lvnInst = NULL;
    LVNItemInfo *lvnItem = NULL;
    int64_t posVal = 0;
    bool addGlobalValueToTable = false;

    bool canAdd = addValue(inst);
    bool success = false;
    if (canAdd) {
      perInstValueCache.clear();

      // Compute value of current instruction
      // success is false when there is a float type-conversion mov
      // that we don't implement, eg :hf->:f interpretation.
      success =
          computeValue(inst, false, canNegate, isGlobal, posVal, false, value);
      value.inst = inst;
      oldValue = value;

      if (isGlobal || value.isValueEmpty()) {
        if (isGlobal && !value.isValueEmpty() &&
            inst->getDst()->getTopDcl()->getRegFile() != G4_FLAG) {
          // If variable is global, we want to add it to LVN table.
          addGlobalValueToTable = true;
        }

        // If a value is global or is empty we cannot eliminate
        // the definition just because another variable holds
        // same data. So the logic of replacing operands is
        // skipped with canAdd == false.
        canAdd = false;
      }

      if (canAdd && success) {
        // Check if value exists in table
        lvnItem = isValueInTable(value, false);

        // If value doesnt exist, search for value's
        // negative representation (only for imm).

        if (canNegate && lvnItem == NULL) {
          computeValue(inst, true, canNegate, isGlobal, posVal, true, value);
          value.inst = inst;
          negMatch = true;

          lvnItem = isValueInTable(value, true);
        }

        if (lvnItem != NULL) {
          lvnInst = lvnItem->inst;
          if (inst->getLocalId() - lvnInst->getLocalId() >
              LVN::MaxLVNDistance) {
            // do not do LVN to avoid register pressure increase
            // removeRedef should get rid of this lvnInst later
          } else {
            auto dstRegionMatch = [](G4_DstRegRegion *dst1,
                                     G4_DstRegRegion *dst2) {
              return dst1->getType() == dst2->getType() &&
                     dst1->getRegOff() == dst2->getRegOff() &&
                     dst1->getSubRegOff() == dst2->getSubRegOff() &&
                     dst1->getHorzStride() == dst2->getHorzStride();
            };
            // if this and lvnInst has identical dst regions, we can replace
            // partial uses as well
            bool hasSameDstRegion =
                dstRegionMatch(inst->getDst(), lvnInst->getDst());
            // How can duTablePopulated be false here? I don't understand the
            // other comment???
            if (duTablePopulated == false) {
              populateDuTable(inst_it);
            }

            bool removeInst = false;
            if (duTablePopulated) {
              UseList uses;
              bool defFound = getAllUses(inst, uses);
              if (defFound) {
                bool canReplaceAllUses = canReplaceUses(
                    inst_it, uses, lvnInst, negMatch, !hasSameDstRegion);

                if (canReplaceAllUses) {
                  replaceAllUses(inst, negMatch, uses, lvnInst,
                                 hasSameDstRegion);
                  removeInst = true;
                }
              }
            }

            if (removeInst) {
              INST_LIST_ITER prev_it = inst_it;
              inst_it--;
              bb->erase(prev_it);

              numInstsRemoved++;
              continue;
            }
          }
        }
      }
    }

    // Scan LVN table and remove all those var entries
    // that have same variable as current inst's dst.
    removeRedefs(inst);

    if (success && (canAdd || addGlobalValueToTable)) {
      addValueToTable(inst, oldValue);
    }
  }
}

// This is a static method. It can be called to cleanup after any optimization.
unsigned int LVN::removeRedundantSamplerMovs(G4_Kernel &kernel, G4_BB *bb) {
  // Remove all redundant writes to rx.2 where x is samplerHeader dcl
  auto samplerDcl = kernel.fg.builder->getBuiltinSamplerHeader();
  if (!samplerDcl)
    return 0;

  G4_INST *lastSamplerDclDef = nullptr;
  unsigned int numInstsRemoved = 0;

  for (auto instIt = bb->begin(); instIt != bb->end();) {
    auto inst = (*instIt);
    auto dst = inst->getDst();
    if (dst && dst->getTopDcl() == samplerDcl) {
      if (!lastSamplerDclDef) {
        // Seeing def for first time in BB
        lastSamplerDclDef = inst;
        ++instIt;
        continue;
      } else if (lastSamplerDclDef->opcode() == G4_mov &&
                 inst->opcode() == G4_mov &&
                 lastSamplerDclDef->getDst()->getType() == dst->getType() &&
                 lastSamplerDclDef->getDst()->getLeftBound() ==
                     dst->getLeftBound() &&
                 lastSamplerDclDef->getDst()->getRightBound() ==
                     dst->getRightBound() &&
                 lastSamplerDclDef->getSrc(0)->isImm() &&
                 inst->getSrc(0)->isImm() &&
                 lastSamplerDclDef->getSrc(0)->asImm()->isEqualTo(
                     inst->getSrc(0)->asImm())) {
        // Redundant mov found, erase it
        instIt = bb->erase(instIt);
        numInstsRemoved++;
        continue;
      } else {
        lastSamplerDclDef = inst;
      }
    }
    ++instIt;
  }

  return numInstsRemoved;
}

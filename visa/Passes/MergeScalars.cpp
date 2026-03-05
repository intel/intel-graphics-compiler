/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeScalars.hpp"

#include <algorithm>
#include <vector>

using namespace vISA;

// This will create a new dcl or reuse the existing one if available.
// The returned dcl will have
//    its Reg's byte address == input's
//    its type == srcType
//    itss numElem == bundleSize.
static G4_Declare *getInputDeclare(IR_Builder &builder,
                                   std::vector<G4_Declare *> &declares,
                                   G4_Declare *input, G4_Type eltType,
                                   int bundleSize, int firstEltOffset) {
  vISA_ASSERT(input->isInput() && input->getAliasDeclare() == nullptr,
              "Expect root input variable");
  for (auto dcl : declares) {
    vISA_ASSERT(dcl->isInput() && dcl->getAliasDeclare() == nullptr,
                "declare must be root input variable");
    if (dcl->getRegVar()->getByteAddr(builder) ==
            input->getRegVar()->getByteAddr(builder) &&
        dcl->getElemType() == eltType && dcl->getTotalElems() >= bundleSize) {
      return dcl;
    }
  }

  // Now, create a new dcl.
  // As the new dcl's offset is in unit of element type, we will convert offset
  // in bytes to offset in elementType.  As the previous check guarantees that
  // the byte offset must be multiple of elementType, here just add additional
  // checks to make sure this is the case.
  uint32_t offset = input->getRegVar()->getPhyRegOff() * input->getElemSize() +
                    firstEltOffset;
  uint32_t eltBytes = TypeSize(eltType);
  vISA_ASSERT((offset % eltBytes) == 0,
              "Offset should be multiple of element size");
  offset = offset / eltBytes;
  const char *name = builder.getNameString(
      16, "InputR%d.%d", input->getRegVar()->getPhyReg()->asGreg()->getRegNum(),
      offset);
  G4_Declare *newInputDcl =
      builder.createDeclare(name, G4_INPUT, (uint16_t)bundleSize, 1, eltType);
  newInputDcl->getRegVar()->setPhyReg(input->getRegVar()->getPhyReg(), offset);
  declares.push_back(newInputDcl);
  return newInputDcl;
}

//
//  merge all instructions in the bundle into a single one, by modifying the
//  first instruction in the bundle and delete the rest returns true if merge is
//  successful merge can fail if a source/dst appears in multiple bundles and
//  has a conflicing alignment (this only happens when the operand is in
//  DISJOINT pattern)
//
bool BUNDLE_INFO::doMerge(IR_Builder &builder,
                          std::unordered_set<G4_Declare *> &modifiedDcl,
                          std::vector<G4_Declare *> &newInputs) {
  if (size == 1) {
    return false;
  }

  // Round down esize to power of 2 to make sure it wouldn't be out of bound.
  int roundDownPow2Size = (int)Round_Down_Pow2(size);
  while (size > roundDownPow2Size) {
    deleteLastInst();
  }

  if (!builder.hasAlign1Ternary() && size == 2 && inst[0]->getNumSrc() == 3) {
    return false;
  }

  // for distinct patterns (both src and dst), we need to check the variable is
  // only used once in the entire bundle, so that the new variable created for
  // the operand does not conflict with another. this is to prevent coalescing
  // two variables with conflicting life range, e.g.,
  //  add (M1_NM, 1) V66(0,0)<1> V66(0,0)<0;1,0> 0x1:d /// $36
  //  add (M1_NM, 1) V67(0,0)<1> V65(0,0)<0;1,0> 0x1:d  /// $37 or
  //  make a variable with two root aliases:
  //  mul (M1_NM, 1) V186(0,0)<1> V184(0,0)<0;1,0> V185(0,0)<0; 1, 0> /// $206
  //  mul (M1_NM, 1) V187(0,0)<1> V182(0,0)<0;1,0> V182(0,0)<0; 1, 0> /// $207
  std::set<G4_Declare *> uniqueDeclares;

  // check if merging is legal
  if (dstPattern == OPND_PATTERN::DISJOINT) {
    G4_Declare *rootDcl = inst[0]->getDst()->getTopDcl()->getAliasDeclare();
    for (int instId = 0; instId < size; ++instId) {
      G4_DstRegRegion *dst = inst[instId]->getDst();
      G4_Declare *dcl = dst->getTopDcl();
      if (dcl->getAliasDeclare() != rootDcl) {
        // all dsts in the bundle should have the same base variable (or
        // nullptr) this is to avoid the situation where {V1, V2} is in one
        // bundle and {V1, V3} is in another bundle, and we can't optimize both
        // since V2 and V3 may be both live
        return false;
      }
      if (dcl->getAliasDeclare() != nullptr) {
        if (dcl->getAliasOffset() != dst->getTypeSize() * instId) {
          return false;
        }
      }
      uniqueDeclares.insert(dcl);
    }
  }
  for (int i = 0; i < inst[0]->getNumSrc(); ++i) {
    if (srcPattern[i] == OPND_PATTERN::DISJOINT) {
      G4_Declare *rootDcl = inst[0]->getSrc(i)->getTopDcl()->getAliasDeclare();
      for (int instId = 0; instId < size; ++instId) {
        G4_Operand *src = inst[instId]->getSrc(i);
        G4_Declare *dcl = src->getTopDcl();
        if (dcl->getAliasDeclare() != rootDcl) {
          // all srcs in the bundle, if aliased, should have the same alias
          // variable see comments from above for dst
          return false;
        }
        if (dcl->getAliasDeclare() != nullptr) {
          if (dcl->getAliasOffset() != src->getTypeSize() * instId) {
            return false;
          }
        }
        if (uniqueDeclares.find(dcl) != uniqueDeclares.end()) {
          return false;
        }
        uniqueDeclares.insert(dcl);
      }
    }
  }

  G4_ExecSize execSize = (G4_ExecSize)size;
  G4_INST *newInst = inst[0]; // reuse inst[0] as the new merged inst

  // at this point merge will definitely succeed
  // handle merged dst
  if (dstPattern == OPND_PATTERN::DISJOINT) {
    G4_Type dstType = newInst->getDst()->getType();
    // create a new declare with bundle size and have the original dsts alias to
    // it
    G4_Declare *newDcl = nullptr;
    if (newInst->getDst()->getTopDcl()->getAliasDeclare() != nullptr) {
      newDcl = newInst->getDst()->getTopDcl()->getAliasDeclare();
    } else {
      newDcl = builder.createTempVar(execSize, dstType, Eight_Word, "Merged");
    }
    for (int i = 0; i < size; ++i) {
      G4_Declare *dstDcl = inst[i]->getDst()->getTopDcl();
      if (dstDcl->getAliasDeclare() == nullptr) {
        dstDcl->setAliasDeclare(newDcl, i * TypeSize(dstType));
        modifiedDcl.insert(dstDcl);
        VISA_DEBUG_VERBOSE(std::cout << "Dcl " << dstDcl->getName() << "--> ("
                                     << newDcl->getName() << ", "
                                     << i * TypeSize(dstType) << ")\n");
      }
    }
    G4_DstRegRegion *newDst =
        builder.createDst(newDcl->getRegVar(), 0, 0, 1, dstType);
    newInst->setDest(newDst);
  } else {
    vISA_ASSERT(dstPattern == OPND_PATTERN::CONTIGUOUS,
                "unexpected dst pattern");

    // Note that SIMD1 movs may have inefficient or incorrect dst stride prior
    // to merging. But that is ok because the instruction is SIMD1 and the dst
    // stride is not exercised
    // For example:
    // mov (M1_NM, 1) V0050(0,1)<4> V0046(0,7)<0;1,0>
    // mov (M1_NM, 1) V0050(0,2)<4> V0046(0,8)<0;1,0>
    // These movs are correct even though the dst stride is 4; the dst stride
    // will be ignored because these are SIMD1 movs
    // However, when we merge these two mov instructions into a SIMD2, we do
    // not want to apply this stride of 4 to the merged instruction's dest i.e
    // mov (2) V0050(0,1)<4> V0046(0,7)<1;1,0>:b
    // the above instruction is incorrect

    // set the destination to have a horizontal stride of 1
    newInst->getDst()->setHorzStride(1);
  }

  for (int i = 0; i < newInst->getNumSrc(); ++i) {
    if (srcPattern[i] == OPND_PATTERN::DISJOINT) {
      G4_Operand *oldSrc = newInst->getSrc(i);
      G4_Type srcType = oldSrc->getType();
      // create a new declare with bundle size and have the original dsts alias
      // to it
      G4_Declare *newDcl = nullptr;
      if (oldSrc->getTopDcl()->getAliasDeclare() != nullptr) {
        newDcl = oldSrc->getTopDcl()->getAliasDeclare();
      } else {
        newDcl = builder.createTempVar(execSize, srcType, Eight_Word, "Merged");
      }
      for (int j = 0; j < size; ++j) {
        vISA_ASSERT(inst[j]->getSrc(i)->isSrcRegRegion(),
                    "Src must be a region");
        G4_SrcRegRegion *src = inst[j]->getSrc(i)->asSrcRegRegion();
        G4_Declare *srcDcl = src->getTopDcl();
        if (srcDcl->getAliasDeclare() == nullptr) {
          srcDcl->setAliasDeclare(newDcl, j * TypeSize(srcType));
          modifiedDcl.insert(srcDcl);
          VISA_DEBUG_VERBOSE(std::cout << "Dcl " << srcDcl->getName() << "--> ("
                                       << newDcl->getName() << ", "
                                       << i * TypeSize(srcType) << ")\n");
        }
      }
      G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(
          oldSrc->asSrcRegRegion()->getModifier(), Direct, newDcl->getRegVar(),
          0, 0, builder.getRegionStride1(), srcType);
      newInst->setSrc(newSrc, i);
    } else if (srcPattern[i] == OPND_PATTERN::CONTIGUOUS) {
      // update region
      G4_SrcRegRegion *src = newInst->getSrc(i)->asSrcRegRegion();
      G4_Declare *rootDcl = src->getTopDcl()->getRootDeclare();
      if (rootDcl->isInput()) {
        // check if the existing input is good enough
        bool sameInput = true;
        for (int instId = 1; instId < size; ++instId) {
          auto dcl = inst[instId]->getSrc(i)->getTopDcl()->getRootDeclare();
          if (rootDcl != dcl) {
            sameInput = false;
            break;
          }
        }

        if (sameInput) {
          src->setRegion(builder, builder.getRegionStride1());
        } else {
          // we need a new input variable that covers all inputs used in the
          // bundle Since rootDcl may have a different type than the operands',
          // need to use operand's type instead of rootDcl, so we pass srcType
          // into getInputDeclare().
          G4_Type srcType = newInst->getSrc(i)->getType();
          int firstEltOffset =
              src->getRegOff() * builder.numEltPerGRF<Type_UB>() +
              src->getSubRegOff() * TypeSize(srcType);
          G4_Declare *newInputDcl = getInputDeclare(
              builder, newInputs, rootDcl, srcType, size, firstEltOffset);
          src = builder.createSrcRegRegion(
              src->getModifier(), Direct, newInputDcl->getRegVar(), 0, 0,
              builder.getRegionStride1(), src->getType());
          newInst->setSrc(src, i);
        }
      } else {
        src->setRegion(builder, builder.getRegionStride1());
      }
      newInst->setExecSize(execSize);
    } else if (srcPattern[i] == OPND_PATTERN::PACKED) {

      // lambda to compute the data type based on how many values we
      // packed into a single value
      auto dataTypeSize = [&](G4_Operand *dst) {
        // size is a power of 2; size is round down to the lowest power of 2
        // before performing the merge; for example, a size of 3 is round down
        // to size of 2.
        // size is capped to sizeLimit, which is 4
        uint32_t totalBytes = size * dst->getTypeSize();
        if (totalBytes < 4)
          return Type_UW;
        if (totalBytes == 4)
          return Type_UD;
        if (totalBytes == 8 && !builder.noInt64())
          return Type_UQ;
        // todo: revisit this to explore opportunities for coalescing into
        // qword; note that not all platforms support such movs
        return Type_UNDEF;
      };

      // In canMergeSource, we check only on whether the instructions
      // operate on different immediate values. Here, we do a complete check for
      // coalescing scalar moves based on the following conditions:
      // 1. check if instruction in bundle is mov
      // 2. check if mov instructions in bundle are operating on same
      //    destination
      // 3. check if mov execution mask is no mask SIMD1
      // 4. check if srcs in bundle are immediate values (handled in
      //    canMergeSrc)
      // 5. check if destination type is not float (soft constraint, todo: must
      // address)
      // 6. check is the total size of packed data type is less than equal to
      //    largest datatype size (qword).
      for (int j = 0; j < size; j++) {
        if (!inst[j]->isMov())
          return false;
        if (j > 0 && inst[j - 1]->getDst()->getTopDcl() !=
                         inst[j]->getDst()->getTopDcl()) {
          return false;
        }
      }

      auto packedType = dataTypeSize(newInst->getDst());
      if (packedType == Type_UNDEF)
        return false;

      // create the packed value
      // since we also create packed values with non-motonic subregs, the
      // shift amount is subregID * type size (bytes) * 8
      uint64_t packedVal = 0;
      for (int j = 0; j < size; j++) {
        G4_Imm *immValue = inst[j]->getSrc(0)->asImm();
        uint32_t shiftVal = j * newInst->getDst()->getTypeSize() * 8;
        uint64_t val = 0;
        switch (inst[j]->getDst()->getType()) {
        case G4_Type::Type_UD:
        case G4_Type::Type_D:
        case G4_Type::Type_F:
          val = (uint32_t)immValue->getImm();
          break;
        case G4_Type::Type_UW:
        case G4_Type::Type_W:
        case G4_Type::Type_HF:
          val = (uint16_t)immValue->getImm();
          break;
        case G4_Type::Type_UB:
        case G4_Type::Type_B:
          val = (uint8_t)immValue->getImm();
          break;
        default:
          vISA_ASSERT_UNREACHABLE("unsupported data type");
        }
        packedVal += ((uint64_t)val << shiftVal);
      }

      unsigned packedSize = G4_Type_Table[packedType].byteSize;
      // check alignment
      // if destination alignment is less than the datatype of packed value, we
      // cannot do the coalescing
      if (!builder.tryToAlignOperand(newInst->getDst(), packedSize)) {
        return false;
      }
      // set the source of packed instruction with immediate value equal
      // to the new packed value
      newInst->setSrc(builder.createImm(packedVal, packedType), i);

      // create a packed type dcl and set number of elements to 1
      G4_Declare *newDcl = builder.createTempVar(1, packedType, Any, "Packed");
      // newDcl->copyAlign(newInst->getDst()->getTopDcl());
      // set the newDcl dcl alias to the instruction destination dcl
      unsigned int byteOffset =
          newInst->getDst()->getRegOff() * builder.numEltPerGRF<Type_UB>() +
          newInst->getDst()->getSubRegOff() *
              TypeSize(newInst->getDst()->getType());

      newDcl->setAliasDeclare(newInst->getDst()->getTopDcl(), byteOffset);

      // set the destination of new packed instruction
      G4_DstRegRegion *newDst =
          builder.createDst(newDcl->getRegVar(), 0, 0, 1, packedType);
      newInst->setDest(newDst);

      execSize = g4::SIMD1;
    } else {
      vISA_ASSERT(srcPattern[i] == OPND_PATTERN::IDENTICAL,
                  "unexpected source pattern");
    }
  }

  newInst->setExecSize(execSize);

  auto iter = startIter;
  ++iter;
  for (int i = 1; i < size; ++i) {
    G4_INST *instToDelete = *iter;
    instToDelete->transferUse(newInst, true);
    for (int srcNum = 0, numSrc = instToDelete->getNumSrc(); srcNum < numSrc;
         ++srcNum) {
      Gen4_Operand_Number opndPos = instToDelete->getSrcOperandNum(srcNum);
      instToDelete->transferDef(newInst, opndPos, opndPos);
    }

    iter = bb->erase(iter);
  }

  VISA_DEBUG_VERBOSE({
    std::cout << "new inst after merging:\n";
    newInst->emit(std::cout);
    std::cout << "\n";
    newInst->emitDefUse(std::cout);
    std::cout << "\n";
  });

  return true;
}

//
// check to see if offset1 + type size == offset2
// if so and pattern is either unknown or contiguous, return true
// if pattern is unknown also change it to contiguous
//
// also check to see if distance between offset2 and origOffset
// (offset of the first instruction found in current bundle)
// exceeds GRF size. If so returns false.
//
//
static bool checkContiguous(unsigned offset1, unsigned offset2, G4_Type type,
                            OPND_PATTERN &pattern, unsigned origOffset,
                            const IR_Builder &builder) {
  if (origOffset + builder.numEltPerGRF<Type_UB>() <= offset2)
    return false;
  if (offset1 + TypeSize(type) == offset2) {
    switch (pattern) {
    case OPND_PATTERN::UNKNOWN:
      pattern = OPND_PATTERN::CONTIGUOUS;
      return true;
    case OPND_PATTERN::CONTIGUOUS:
      return true;
    default:
      return false;
    }
  }
  return false;
}

// check if operand span is within 2 GRFs
// if operand span is less than equal to 2 GRFs, return true
// else return false
static bool checkOperandGRFSpan(const IR_Builder &builder,
                                int bundleIndex,
                                unsigned short typeSize) {
  // bundleIndex is the index of the instruction appended to
  // the bundle of instructions to be merged if all checks have passed
  // in this function, we check the following: the number of GRFs occupied by
  // the operands from instruction 0 to bundleIndex are within 2 GRFs
  // size of the bundle = bundleIndex+1
  int accumSizeBytes = (bundleIndex+1) * typeSize;
  if (accumSizeBytes > (int)(2 * builder.numEltPerGRF<Type_UB>())) return false;
  return true;
}

static bool checkMergedOperandSize(const IR_Builder &builder,
                                   int bundleIndex,
                                   unsigned short typeSize) {
  // for packing immediate patterns, check if the packed data type so far is
  // within qword (for platforms that support) or dword legal limit
  // the benefit of doing this check here is so that we can return a non-zero
  // size of doing partial coalescing rather than failing entirely and not doing
  // any coalescing/merging in the doMerge function
  // For example:
  //   mov (M1_NM, 1) V1ub(0,0)<1> 0x64:ub
  //   mov (M1_NM, 1) V1ub(0,1)<1> 0x35:ub
  //   mov (M1_NM, 1) V1ub(0,2)<1> 0x6:ub
  //   mov (M1_NM, 1) V1ub(0,3)<1> 0x7:ub
  //   mov (M1_NM, 1) V1ub(0,4)<1> 0x2A:ub
  //   mov (M1_NM, 1) V1ub(0,5)<1> 0x1C:ub
  //   mov (M1_NM, 1) V1ub(0,6)<1> 0xE:ub
  //   mov (M1_NM, 1) V1ub(0,7)<1> 0x3D:ub
  //   In this example, the logic passes this bundle of 8 instructions to
  //   doMerge as they pass all checks in canMerge. These 8 instructions can be
  //   merged into a single qword. However, in doMerge, we check whether the
  //   platform supports qword movs and on a failure do not do any merging.
  //   Rather, it would be good to do 2 merges of 4 instructions each rather
  //   than no merging. By doing this check here, we can tune the bundle size to
  //   merge

  int accumSizeBytes = (bundleIndex+1)*typeSize;
  if (accumSizeBytes < 8)
    return true;
  if (accumSizeBytes == 8) {
    if (!builder.noInt64())
      return true;
  }
  // return false if accum size is greater than 8 or no qword support
  return false;
}

// check if this instruction is eligbile to be merged into a vector instruction
// the conditions are:
// -- arithmetic, logic, mov, math, or pseudo_mad instructions
// -- simd1, NoMask, no predicates or conditional modifier
// -- dst must be direct GRF whose declare has no alias
// -- all sources must be either direct GRF or immediates
bool BUNDLE_INFO::isMergeCandidate(
    G4_INST *inst, const IR_Builder &builder,
    std::unordered_set<G4_Declare *> &modifiedDcl, bool isInSimdFlow) {
  if (inst->isOptBarrier())
    return false;

  // Don't merge mixed mode instructions as SIMD2/4 mixed mode instructions with
  // packed HF is not allowed by HW.
  if (inst->isMixedMode())
    return false;

  if (!inst->isArithmetic() && !inst->isLogic() && !inst->isMath() &&
      !inst->isMov() && inst->opcode() != G4_pseudo_mad) {
    return false;
  }

  if (inst->getExecSize() != g4::SIMD1 ||
      (!inst->isWriteEnableInst() && isInSimdFlow)) {
    return false;
  }

  if (inst->getPredicate() || inst->getCondMod()) {
    return false;
  }

  vISA_ASSERT(inst->getDst() != nullptr, "dst must not be nullptr");
  if (inst->getDst()->isIndirect()) {
    return false;
  }

  if (builder.no64bitRegioning() && inst->getDst()->getTypeSize() == 8) {
    // applying merge scalar makes code quality worse since we need to insert
    // moves to compensate for the lack of 64-bit regions later
    return false;
  }

  // When there's a platform-specific alignment requirement for the 3-src inst,
  // be conservative and skip merging the current inst if
  //   1. the current inst is a 3-src inst, or
  //   2. any source operand is defined by a 3-src inst
  if (builder.has3SrcDstAlignRequirement() && (inst->getNumSrc() == 3 ||
          std::any_of(inst->def_begin(), inst->def_end(),
                      [](const USE_DEF_NODE &node) {
                        return node.first->getNumSrc() == 3; })))
    return false;

  G4_VarBase *dstBase = inst->getDst()->getBase();
  G4_Declare *dstDcl =
      dstBase->isRegVar() ? dstBase->asRegVar()->getDeclare() : nullptr;

  /*
   // fixme: Why is the below condition enforced?
   // It prevents the following merging of scalar moves:
   // (W) mov (1)              v_b(0,0)<1>:b  0x4:w
   // (W) mov (1)              v_b(0,1)<1>:b  0x5:w
   // (W) mov (1)              v_b(0,2)<1>:b  0x6:w
   // (W) mov (1)              v_b(0,3)<1>:b  0x7:w
   //     mov (16)             dst(0,0)<1>:d v(0,0)<0;1,0>:d
   // into
   // (W) mov (1)              v(0,0)<1>:d 0x7060504:d
   //     mov (16)              dst(0,0)<1>:d v(0,0)<0;1,0>:d
  */
  if (dstDcl != nullptr &&
      (inst->getDst()->getTypeSize() != dstDcl->getElemSize() ||
       !dstDcl->useGRF())) {
    return false;
  }

  if (dstDcl != nullptr && dstDcl->getAliasDeclare() != nullptr) {
    // If the merge(alias) declare is created by scalar merge pass, it can be
    // reused for merging same aliased variables. This was implemented for
    // instructions in same BB already. So should be valid for instructios in
    // different BBs as well. The modifiedDcl stores the merge delcares cross
    // BB, so searching it should be enough.
    if (!builder.shareMergeVarGlobal() ||
        modifiedDcl.find(dstDcl) == modifiedDcl.end()) {
      return false;
    }
  }

  if (dstDcl != nullptr && (dstDcl->isOutput() || dstDcl->isInput())) {
    return false;
  }

  for (int i = 0; i < inst->getNumSrc(); ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src->isSrcRegRegion()) {
      if (src->asSrcRegRegion()->isIndirect()) {
        return false;
      }
      G4_Declare *srcDcl = src->getTopDcl();
      if (srcDcl) {
        if (!srcDcl->useGRF() /* || srcDcl->getTotalElems() != 1 */) {
          return false;
        }
        // can't do opt if source decl type is inconsistent with its use
        if (TypeSize(srcDcl->getElemType()) != src->getTypeSize()) {
          return false;
        }
      } else if (!src->isNullReg()) // Skip null reg.
      {
        return false;
      }
    } else if (src->isImm() && !src->isRelocImm()) {
      // ok
    } else {
      return false;
    }
  }

  return true;
}

// returns true if dcl is a scalar root variable that is naturally aligned
// Note that this also rules out input
static bool isScalarNaturalAlignedVar(
    G4_Declare *dcl, std::unordered_set<G4_Declare *> &modifiedDcl,
    const IR_Builder &builder) {
  bool isNaturalAlignedVar =
      dcl->getTotalElems() == 1 &&
      dcl->getByteAlignment() == TypeSize(dcl->getElemType()) &&
      !dcl->isInput();
  if (isNaturalAlignedVar && dcl->getAliasDeclare() != nullptr) {
    if (builder.shareMergeVarGlobal()) {
      // If the merge(alias) declare is created by scalar merge pass, it can be
      // reused for merging same aliased variables. This was implemented for
      // instructions in same BB already. So should be valid for instructios in
      // different BBs as well. The modifiedDcl stores the merge delcares cross
      // BB, so searching it should be enough.
      isNaturalAlignedVar = modifiedDcl.find(dcl) != modifiedDcl.end();
    } else {
      isNaturalAlignedVar = false;
    }
  }

  return isNaturalAlignedVar;
}

//
// Check if the dst can be combined into the bundle
// There are 2 scenarios:
// CONTIGUOUS: the dsts together form a contiguous region
// DISJOINT: all dsts are distinct scalar, naturally aligned root variables
//
bool BUNDLE_INFO::canMergeDst(G4_DstRegRegion *dst,
                              std::unordered_set<G4_Declare *> &modifiedDcl,
                              const IR_Builder &builder) {
  // src must be either Imm or SrcRegRegion

  G4_DstRegRegion *firstDst = inst[0]->getDst();
  G4_DstRegRegion *prevDst = inst[size - 1]->getDst();
  if (prevDst->getType() != dst->getType()) {
    return false;
  }

  G4_Declare *prevDstDcl = prevDst->getTopDcl();
  G4_Declare *dstDcl = dst->getTopDcl();

  if ((dstDcl == nullptr) || (prevDstDcl == nullptr)) {
    return false;
  }

  if (prevDstDcl == dstDcl) {
    if (dstPattern == OPND_PATTERN::DISJOINT) {
      return false;
    }

    if (!checkContiguous(prevDst->getLeftBound(), dst->getLeftBound(),
                         dst->getType(), dstPattern, firstDst->getLeftBound(),
                         builder)) {
      return false;
    }
  } else if (prevDstDcl->isInput() && dstDcl->isInput()) {
    unsigned firstDstGRFOffset =
        firstDst->getLeftBound() +
        firstDst->getTopDcl()->getRegVar()->getByteAddr(builder);
    unsigned prevDstGRFOffset =
        prevDst->getLeftBound() + prevDstDcl->getRegVar()->getByteAddr(builder);
    unsigned dstGRFOffset =
        dst->getLeftBound() + dstDcl->getRegVar()->getByteAddr(builder);
    if (!checkContiguous(prevDstGRFOffset, dstGRFOffset, dst->getType(),
                         dstPattern, firstDstGRFOffset, builder)) {
      return false;
    }
  } else {
    switch (dstPattern) {
    case OPND_PATTERN::UNKNOWN:
      // allow if both sources are size 1 root variables with no alignment
      if (isScalarNaturalAlignedVar(prevDstDcl, modifiedDcl, builder) &&
          isScalarNaturalAlignedVar(dstDcl, modifiedDcl, builder)) {
        dstPattern = OPND_PATTERN::DISJOINT;
      } else {
        return false;
      }
      break;
    case OPND_PATTERN::DISJOINT:
      if (!isScalarNaturalAlignedVar(dstDcl, modifiedDcl, builder)) {
        return false;
      }
      // also check to see if dst is the same as any other previous dst
      for (int i = 0; i < size - 1; i++) {
        G4_INST *bundleInst = inst[i];
        if (dstDcl == bundleInst->getDst()->getTopDcl()) {
          return false;
        }
      }
      break;
    default:
      return false;
    }
  }

  // additionally check if dst has a WAR dependence with one of the source in
  // earlier instructions it may seem that WAR dependencies should not interfere
  // with vectorization since src is always read before the dst; however, the
  // problem is that if src and dst in different instructions refer to the same
  // variable we can't get them to align properly.
  for (int i = 0; i < size; i++) {
    for (int srcPos = 0; srcPos < inst[i]->getNumSrc(); ++srcPos) {
      G4_Operand *src = inst[i]->getSrc(srcPos);
      // since both are scalar, check is as simple as comparing the dcl
      if (src->getTopDcl() != nullptr && src->getTopDcl() == dst->getTopDcl()) {
        return false;
      }
    }
  }

  // check whether dst operand is within 2 GRFs
  // note that we can send the type size of current dst as we check earlier
  // whether the operand types in the bundle are the same
  return checkOperandGRFSpan(builder, size, dst->getTypeSize());
}

//
// Check if this src at srcPos can be combined into the bundle
// There are 4 scenarios:
// IDENTICAL: all sources in the bundle are the same scalar variable
// CONTIGUOUS: the sources together form a contiguous region
// DISJOINT: all sources are distinct scalar, naturally aligned root variables
// PACKED: all sources in the bundle are different scalar imm values that can be
// packed into a wider scalar imm value
bool BUNDLE_INFO::canMergeSource(G4_Operand *src, int srcPos,
                                 std::unordered_set<G4_Declare *> &modifiedDcl,
                                 const IR_Builder &builder) {
  // src must be either Imm or SrcRegRegion

  vISA_ASSERT(srcPos < maxNumSrc, "invalid srcPos");

  if (src->isRelocImm()) {
    return false;
  }

  if ((inst[0]->isMath() && inst[0]->asMathInst()->isOneSrcMath()) &&
      srcPos == 1) {
    // null source is always legal
    srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
    return true;
  }

  G4_Operand *firstSrc = inst[0]->getSrc(srcPos);
  G4_Operand *prevSrc = inst[size - 1]->getSrc(srcPos);
  if (prevSrc->getType() != src->getType()) {
    return false;
  }
  if (prevSrc->isImm()) {
    if (!src->isImm()) {
      // no coalescing of immediate values possible
      return false;
    } else {
      // The PACKED pattern bundle attempts to combine mov immediate
      // instructions into a single mov instruction with the immediate value
      // obtained from packing multiple immediate values into a single value. As
      // a result, the packed immediate value has a wider data type. Since,
      // select platforms support wide datatype moves, the packing optimization
      // is only enabled for immediate value data types below qword.
      // For data types above dword, if the values are identical, then the
      // identical pattern logic will be exercised resulting in wider SIMD
      // moves; otherwise no optimization is performed
      if (builder.getOption(vISA_CoalesceScalarMoves) && !IS_QTYPE(src->getType())) {
        // first check whether we have already detected an IDENTICAL or PACKED
        // pattern with prior instructions
        if (srcPattern[srcPos] == OPND_PATTERN::PACKED) {
          // already detected a packed pattern
          if (dstPattern != OPND_PATTERN::CONTIGUOUS) {
            // if dst pattern is not contiguous, cannot coalesce this mov
            // instruction with instructions in the bundle
            return false;
          }
        } else if (srcPattern[srcPos] == OPND_PATTERN::IDENTICAL) {
          // already detected an identical pattern with prior instructions
          // continue using identical pattern
          if (prevSrc->asImm()->getImm() != src->asImm()->getImm()) {
            // if values are not same, cannot merge with instructions in the
            // bundle
            return false;
          }
        } else if (srcPattern[srcPos] == OPND_PATTERN::UNKNOWN) {
          // no pattern detected yet
          // choose based on the immediate value checks
          if (prevSrc->asImm()->getImm() == src->asImm()->getImm()) {
            srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
          } else if (dstPattern == OPND_PATTERN::CONTIGUOUS) {
            srcPattern[srcPos] = OPND_PATTERN::PACKED;
          } else {
            return false;
          }
        }
      } else if (prevSrc->asImm()->getImm() == src->asImm()->getImm()) {
        srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
      } else {
        return false;
      }
    }
  } else {
    if (!src->isSrcRegRegion()) {
      return false;
    }
    G4_SrcRegRegion *prevSrcRegion = prevSrc->asSrcRegRegion();
    G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
    if (prevSrcRegion->getModifier() != srcRegion->getModifier()) {
      return false;
    }

    G4_Declare *prevSrcDcl = prevSrc->getTopDcl();
    G4_Declare *srcDcl = src->getTopDcl();
    if (prevSrcDcl == srcDcl) {
      if (srcPattern[srcPos] == OPND_PATTERN::DISJOINT) {
        return false;
      }

      if (prevSrc->getLeftBound() == src->getLeftBound()) {
        // the case where we have identical source for each instruction in the
        // bundle
        switch (srcPattern[srcPos]) {
        case OPND_PATTERN::UNKNOWN:
          srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
          break;
        case OPND_PATTERN::IDENTICAL:
          // do nothing
          break;
        default:
          return false;
        }
      } else if (!checkContiguous(prevSrc->getLeftBound(), src->getLeftBound(),
                                  src->getType(), srcPattern[srcPos],
                                  firstSrc->getLeftBound(), builder)) {
        return false;
      }
    } else if (prevSrcDcl->isInput() && srcDcl->isInput()) {
      unsigned firstSrcGRFOffset =
          firstSrc->getLeftBound() +
          firstSrc->getTopDcl()->getRegVar()->getByteAddr(builder);
      unsigned prevSrcGRFOffset = prevSrc->getLeftBound() +
                                  prevSrcDcl->getRegVar()->getByteAddr(builder);
      unsigned srcGRFOffset =
          src->getLeftBound() + srcDcl->getRegVar()->getByteAddr(builder);
      if (!checkContiguous(prevSrcGRFOffset, srcGRFOffset, src->getType(),
                           srcPattern[srcPos], firstSrcGRFOffset, builder)) {
        return false;
      } else if (prevSrcGRFOffset / builder.numEltPerGRF<Type_UB>() !=
                 srcGRFOffset / 32) {
        // resulting input would cross GRF boundary, and our RA does not like it
        // one bit
        return false;
      }

      if (inst[0]->getNumSrc() == 3) {
        // for 3src inst, we can't merge if inst0's src is not oword-aligned
        if ((prevSrcGRFOffset & 0xF) != 0) {
          return false;
        }
      }
    } else {
      switch (srcPattern[srcPos]) {
      case OPND_PATTERN::UNKNOWN:
        // allow if both sources are size 1 root variables with no alignment
        if (isScalarNaturalAlignedVar(prevSrcDcl, modifiedDcl, builder) &&
            isScalarNaturalAlignedVar(srcDcl, modifiedDcl, builder)) {
          srcPattern[srcPos] = OPND_PATTERN::DISJOINT;
        } else {
          return false;
        }
        break;
      case OPND_PATTERN::DISJOINT:
        if (!isScalarNaturalAlignedVar(srcDcl, modifiedDcl, builder)) {
          return false;
        }
        // also check to see if src is the same as any other previous sources
        for (int i = 0; i < size - 1; i++) {
          G4_INST *bundleInst = inst[i];
          if (srcDcl == bundleInst->getSrc(srcPos)->getTopDcl()) {
            return false;
          }
        }
        break;
      default:
        return false;
      }
    }
  }

  if (src->isSrcRegRegion()) {
    // we additionally have to check if src has a RAW dependency with one of the
    // previous dst in the bundle
    for (int i = 0; i < size; i++) {
      G4_DstRegRegion *dst = inst[i]->getDst();
      // since both are scalar, check is as simple as comparing the dcl
      if (src->getTopDcl() == dst->getTopDcl()) {
        return false;
      }
    }
  }

  if (srcPattern[srcPos] == OPND_PATTERN::PACKED) {
    if (!checkMergedOperandSize(builder, size, src->getTypeSize())) {
      return false;
    }
  }

  // check if src operand is within 2 GRFs
  // note that we can send the type size of current src as we check earlier
  // whether the source operand types of previous and current instruction are
  // the same
  return checkOperandGRFSpan(builder, size, src->getTypeSize());
}

//
// check if inst can be successfully appended to the bundle
// For this to be successful:
// -- inst must itself be a merge candidate (checked by caller)
// -- inst must have same opcode/dst modifier/src modifier as all other
// instructions in the bundle
// -- dst and src operand for the inst must form one of the legal patterns with
// the instructions in the bundle
//
bool BUNDLE_INFO::canMerge(G4_INST *inst,
                           std::unordered_set<G4_Declare *> &modifiedDcl,
                           const IR_Builder &builder) {
  G4_INST *firstInst = this->inst[0];
  if (firstInst->opcode() != inst->opcode()) {
    return false;
  }
  if (firstInst->isBfn() && firstInst->asBfnInst()->getBooleanFuncCtrl() !=
                                inst->asBfnInst()->getBooleanFuncCtrl()) {
    return false;
  }

  if (inst->isMath()) {
    G4_MathOp firstMathOp = MATH_RESERVED;
    if (firstInst->isMath()) {
      firstMathOp = firstInst->asMathInst()->getMathCtrl();
    }

    if (firstMathOp != inst->asMathInst()->getMathCtrl()) {
      return false;
    }
  }

  if (firstInst->getSaturate() != inst->getSaturate()) {
    return false;
  }

  if (!canMergeDst(inst->getDst(), modifiedDcl, builder)) {
    return false;
  }

  for (int i = 0; i < inst->getNumSrc(); ++i) {
    if (!canMergeSource(inst->getSrc(i), i, modifiedDcl, builder)) {
      return false;
    }
  }

  // append instruction to bundle
  appendInst(inst);
  return true;
}

//
// iter is advanced to the next instruction not belonging to the handle
//
void BUNDLE_INFO::findInstructionToMerge(
    INST_LIST_ITER &iter, std::unordered_set<G4_Declare *> &modifiedDcl,
    const IR_Builder &builder) {

  for (; iter != bb->end() && this->size < this->sizeLimit; ++iter) {
    G4_INST *nextInst = *iter;
    if (!BUNDLE_INFO::isMergeCandidate(nextInst, builder, modifiedDcl,
                                       !bb->isAllLaneActive())) {
      break;
    }

    if (!canMerge(nextInst, modifiedDcl, builder)) {
      break;
    }
  }
}

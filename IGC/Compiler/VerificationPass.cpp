/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/VerificationPass.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Intrinsics.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

// Register pass to igc-opt
#define PASS_FLAG "igc-verify"
#define PASS_DESCRIPTION " IGC IR Module Verifier"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(VerificationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(VerificationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char VerificationPass::ID = 0;

VerificationPass::VerificationPass() : ModulePass(ID), m_Module(nullptr), m_str(), m_messagesToDump(m_str) {
  initializeVerificationPassPass(*PassRegistry::getPassRegistry());
  initVerificationPass();
}

void VerificationPass::initVerificationPass() {
  ///
  /// Fill the m_IGC_IR_spec structure with the values
  /// provided in the "IR" section of IGC_IR_spec.hpp.
  ///
#define SPECIFIC_INSTRUCTION_VERIFIER(insType, verifierFunc)

#define IGC_IR_FP_TYPE(name, size) m_IGC_IR_spec.FPTypeSizes.insert(size);

#define IGC_IR_VECTOR_TYPE(name, size) m_IGC_IR_spec.vectorTypeSizes.insert(size);

#define IGC_IR_LLVM_INTRINSIC(name) m_IGC_IR_spec.IGCLLVMIntrinsics.insert(Intrinsic::name);

#define IGC_IR_LLVM_INSTRUCTION(name)                                                                                  \
  if (!m_IGC_IR_spec.Instructions.count(Instruction::name)) {                                                          \
    m_IGC_IR_spec.Instructions.insert(Instruction::name);                                                              \
  }

#define DECLARE_OPCODE(llvm_name, inst_class, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic,     \
                       regioning)                                                                                      \
  addIRSpecObject(IRSpecObjectClass::inst_class, (unsigned int)inst_class::llvm_name);

#include "Compiler/IGC_IR_spec.hpp"

#undef IGC_IR_INTEGER_TYPE
#undef IGC_IR_FP_TYPE
#undef IGC_IR_VECTOR_TYPE
#undef IGC_IR_LLVM_INTRINSIC
#undef IGC_IR_LLVM_INSTRUCTION
#undef SPECIFIC_INSTRUCTION_VERIFIER
#undef DECLARE_OPCODE
}

void VerificationPass::addIRSpecObject(IRSpecObjectClass objClass, unsigned int ID) {
  if (objClass == IRSpecObjectClass::Instruction) {
    m_IGC_IR_spec.Instructions.insert(ID);
  }
}

bool VerificationPass::runOnModule(Module &M) {
  m_Module = &M;
  bool success = true;

  if (IGC_IS_FLAG_DISABLED(DisableIRVerification)) {
    // Verify the content of IR functions.
    for (auto funcIt = M.begin(), E = M.end(); funcIt != E; ++funcIt) {
      if (!funcIt->isDeclaration()) {
        if (!verifyFunction(*funcIt)) {
          success = false;
        }
      }
    }

    if (false == success) {
      printf("\nIGC IR verification failed:\n\n");
      errs() << m_messagesToDump.str();
      IGC_ASSERT_MESSAGE(0, "IGC IR Verification failed");
    }
  }

  return false;
}

bool VerificationPass::verifyFunction(Function &F) {
  bool success = true;

  for (auto inst = inst_begin(F), last = inst_end(F); inst != last; ++inst) {
    if (!verifyInstruction(*inst)) {
      success = false;
      break;
    }
  }

  return success;
}

bool VerificationPass::verifyInstruction(Instruction &inst) {
  bool success = true;
  bool verified = false;
  unsigned int opcode = inst.getOpcode();
  if (!m_IGC_IR_spec.Instructions.count(opcode)) {
    m_messagesToDump << "Unexpected instruction:\n";
    printValue(&inst);
    return false;
  }

  // verifyInstCommon() is executed for all instruction.
  // If a specific verification function is provided in the
  // IGC_IR_spec.hpp, it will be called before the common verification.

#define IGC_IR_FP_TYPE(name, size)
#define IGC_IR_VECTOR_TYPE(name, size)
#define IGC_IR_LLVM_INTRINSIC(name)
#define IGC_IR_LLVM_INSTRUCTION(name)
#define DECLARE_OPCODE(llvm_name, inst_class, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic,     \
                       regioning)

#define SPECIFIC_INSTRUCTION_VERIFIER(instClass, verifier)                                                             \
  if (opcode == Instruction::instClass) {                                                                              \
    verified = true;                                                                                                   \
    success &= verifier(inst);                                                                                         \
  }

#include "IGC_IR_spec.hpp"

#undef IGC_IR_FP_TYPE
#undef IGC_IR_VECTOR_TYPE
#undef IGC_IR_LLVM_INTRINSIC
#undef IGC_IR_LLVM_INSTRUCTION
#undef SPECIFIC_INSTRUCTION_VERIFIER
#undef DECLARE_OPCODE

  if (!verified && !verifyInstCommon(inst)) {
    success = false;
  }

  return success;
}

bool VerificationPass::verifyInstCommon(Instruction &inst) {
  // Walk over all the operands of each instruction
  bool success = true;
  int numOperands = inst.getNumOperands();

  for (int i = 0; i < numOperands; ++i) {
    if (!verifyValue(inst.getOperand(i))) {
      success = false;
    }
  }

  return success;
}

///-----------------------------------------------------------------------------------
/// Specific verification functions for instructions should be implemented here.
///-----------------------------------------------------------------------------------

///
/// Specific verification function for Call instructions.
///
bool VerificationPass::verifyInstCall(Instruction &inst) {
  CallInst *instCall = dyn_cast<CallInst>(&inst);
  IGC_ASSERT_MESSAGE(instCall, "Unexpected instruction");

  if (GenIntrinsicInst::classof(instCall)) {
    return true;
  }

  if (dyn_cast<IntrinsicInst>(instCall)) {
    IGCLLVM::Intrinsic intrinID = (IGCLLVM::Intrinsic)instCall->getCalledFunction()->getIntrinsicID();
    if (m_IGC_IR_spec.IGCLLVMIntrinsics.count(intrinID)) {
      return true;
    } else {
      m_messagesToDump << "Unsupported LLVM intrinsic:\n";
      printValue(&inst);
      return false;
    }
  }
  if (!verifyInstCommon(inst)) {
    return false;
  }

  return true;
}

bool VerificationPass::verifyVectorInst(llvm::Instruction &inst) {
  bool success = true;
  int numOperands = inst.getNumOperands();

  for (int i = 0; i < numOperands; ++i) {
    Value *val = inst.getOperand(i);
    Type *T = val->getType();
    if (auto VT = dyn_cast<VectorType>(T)) {
      // Insert and extract element support relaxed vector type
      T = VT->getElementType();
    }
    if (!verifyType(T, val)) {
      success = false;
    }
  }
  return success;
}

///-----------------------------------------------------------------------------------

bool VerificationPass::verifyValue(Value *val) {
  // Check that the type is allowed in IGC IR.
  return verifyType(val->getType(), val);
}

bool VerificationPass::verifyType(Type *type, Value *val) {
  bool success = true;

  if (type->isFloatingPointTy()) {
    unsigned int typeSize = (unsigned int)type->getPrimitiveSizeInBits();
    if (!m_IGC_IR_spec.FPTypeSizes.count(typeSize)) {
      m_messagesToDump << "Unexpected FP type found in value:\n";
      printValue(val);
      return false;
    }
    return true;
  }

  switch (type->getTypeID()) {
  case Type::VoidTyID:
  case Type::LabelTyID:
  case Type::FunctionTyID:
  case Type::MetadataTyID:
    break;

  case Type::IntegerTyID:
    // All integer types are valide
    break;

  case IGCLLVM::VectorTyID: {
    auto VType = cast<IGCLLVM::FixedVectorType>(type);
    unsigned typeSize = (unsigned)VType->getNumElements();
    if (!m_IGC_IR_spec.vectorTypeSizes.count(typeSize)) {
      m_messagesToDump << "Unexpected vector size found in value:\n";
      printValue(val);
      success = false;
    }
    success &= verifyType(VType->getElementType(), val);
    break;
  }

  case Type::PointerTyID: {
    // int addrSpace = type->getPointerAddressSpace();

    // if (addrSpace != ADDRESS_SPACE_GLOBAL &&
    //     addrSpace != ADDRESS_SPACE_LOCAL &&
    //     addrSpace != ADDRESS_SPACE_PRIVATE &&
    //     addrSpace != ADDRESS_SPACE_CONSTANT &&
    //     addrSpace != ADDRESS_SPACE_GENERIC &&
    //     addrSpace != ADDRESS_SPACE_GLOBAL_OR_PRIVATE)
    //{
    //     m_messagesToDump << "Unexpected pointer type found in value:\n";
    //     printValue(val);
    //     success = false;
    // }

    // All pointers are valid - a pointer that points to an unsupported
    // type is still valid unless it gets dereferenced, which will get
    // checked later.

    break;
  }

  case Type::ArrayTyID:
    success = verifyType(type->getArrayElementType(), val);
    break;

  case Type::StructTyID:
    for (int i = 0, N = type->getStructNumElements(); i < N; i++) {
      success &= verifyType(type->getStructElementType(i), val);
    }
    break;

  default:
    m_messagesToDump << "Unexpected type found in value:\n";
    printValue(val);
    success = false;
  }

  return success;
}

void VerificationPass::printValue(Value *value) {
  if (!value)
    return;
  m_messagesToDump << *value << '\n';
}

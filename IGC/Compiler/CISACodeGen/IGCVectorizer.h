/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Compiler/IGCPassSupport.h"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"
#include "Probe/Assertion.h"
#include "ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"
#include <fstream>

using namespace IGC;

namespace IGC {

class IGCVectorizerCommon {

public:
  typedef llvm::SmallPtrSet<Instruction *, 8> ValueSet;
  typedef llvm::SmallVector<Instruction *, 8> VecArr;
  typedef llvm::SmallVector<Constant *, 8> VecConst;
  typedef llvm::SmallVector<Value *, 8> VecVal;
  typedef llvm::SmallVector<VecArr, 8> VectorSliceChain;

  struct Slice {
    unsigned int OpNum;
    VecArr Vector;
    unsigned ParentIndex;
  };

  typedef llvm::SmallVector<Slice, 32> VecOfSlices;
  typedef llvm::SmallVector<VecOfSlices, 3> Tree;
  typedef std::unordered_map<Instruction *, VecArr *> InstructionToSliceMap;

  struct InsertStruct {
    Instruction *Final = nullptr;
    // contains insert elements
    VecArr Vec;
    // contains slices of vector tree
    VecOfSlices SlChain;
  };

  bool checkDependencyAndTryToEliminate(VecArr &Slice, unsigned WindowSize);
  unsigned checkSIMD(llvm::Function &F, IGCMD::MetaDataUtils *MDUtils);
  void initializeLogFile(Function &F, string Name);
  void writeLog();

  Instruction *getMaxPoint(VecArr &Slice);
  Instruction *getMinPoint(VecArr &Slice);
  unsigned getPositionInsideBB(llvm::Instruction *Inst);
  void collectPositionInsideBB(llvm::Instruction *Inst);

  bool basicCheck(VecArr &Slice);

  // contains information about instruction position inside BB
  // with relation to other instructions
  std::unordered_map<Value *, unsigned> PositionMap;

  // logging
  std::unique_ptr<std::ofstream> OutputLogFile;
  std::string LogStr;
  llvm::raw_string_ostream OutputLogStream = raw_string_ostream(LogStr);

  CodeGenContext *CGCtx = nullptr;
  IGCMD::MetaDataUtils *MDUtils = nullptr;
  WIAnalysis *WI = nullptr;
  Module *M = nullptr;
  unsigned SIMDSize = 0;
};

class IGCVectorizer : public llvm::FunctionPass, IGCVectorizerCommon {

private:
  // when we vectorize, we build a new vector chain,
  // this map contains associations between scalar and vector
  // basically every element inside scalarSlice should point to the same
  // vectorized element which contains all of them
  std::unordered_map<Value *, Value *> ScalarToVector;

  // all vector instructions that were produced for chain will be stored
  // in this array, used for clean up if we bail
  VecArr CreatedVectorInstructions;

  bool AllowedPlatform = true;

  bool isSafeToVectorize(llvm::Instruction *I);
  bool isSafeToVectorizeSIMD16(llvm::Instruction *I);
  bool isSafeToVectorizeSIMD32(llvm::Instruction *I);

  void findInsertElementsInDataFlow(llvm::Instruction *I, VecArr &Chain);
  bool checkSlice(VecArr &Slice, InsertStruct &InSt);
  bool processChain(InsertStruct &InSt);
  void clusterInsertElement(InsertStruct &InSt);
  void collectInstructionToProcess(VecArr &ToProcess, Function &F);
  void buildTree(VecArr &V, VecOfSlices &Chain);
  void printSlice(Slice *S);

  Instruction *getInsertPointForVector(VecArr &Arr);
  Instruction *getInsertPointForCreatedInstruction(VecVal &Arr, VecArr &Slice);

  bool checkPHI(Instruction *Compare, VecArr &Slice);
  bool handleStub(VecArr &Slice);
  bool handlePHI(VecArr &Slice);
  bool checkInsertElement(Instruction *First, VecArr &Slice);
  bool handleInsertElement(VecArr &Slice, Instruction *Final);
  bool checkExtractElement(Instruction *Compare, VecArr &Slice);
  bool handleExtractElement(VecArr &Slice);
  bool handleCastInstruction(VecArr &Slice);
  bool handleSelectInstruction(VecArr &Slice);
  bool handleBinaryInstruction(VecArr &Slice);
  bool handleCMPInstruction(VecArr &Slice);
  bool handleIntrinsic(VecArr &Slice);
  bool handleWaveAll(VecArr &Slice);
  bool checkBinaryOperator(VecArr &Slice);
  bool checkPrevVectorization(VecArr &Slice, Value *&PrevVectorization);
  bool collectOperandsForVectorization(unsigned OperNumToStart, unsigned OperNumToStop, Instruction *First,
                                       VecArr &Slice, VecVal &Operands);
  bool handleIntrinsicInstruction(VecArr &Slice);

  Value *checkOperandsToBeVectorized(Instruction *First, unsigned int OperNum, VecArr &Slice);
  Value *vectorizeSlice(VecArr &Slice, unsigned int OperNum);

  bool compareOperands(Value *A, Value *B);
  InsertElementInst *createVector(VecArr &Slice, Instruction *InsertPoint);
  void replaceSliceInstructionsWithExtract(VecArr &Slice, Instruction *CreatedInst);

public:
  llvm::StringRef getPassName() const override { return "IGCVectorizer"; }
  virtual ~IGCVectorizer() { ScalarToVector.clear(); }
  IGCVectorizer(const IGCVectorizer &) = delete;
  IGCVectorizer &operator=(const IGCVectorizer &) = delete;
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<WIAnalysis>();
  }
  IGCVectorizer();
  IGCVectorizer(const std::string &FileName);
  static char ID;
};

class IGCVectorCoalescer : public llvm::FunctionPass, IGCVectorizerCommon {

  void processMap(std::unordered_map<unsigned int, std::vector<Instruction *>> &MapOfInstructions);
  void mergeHorizontalSlice(VecArr &Slice);
  void mergeHorizontalSliceIntrinsic(VecArr &Slice);
  void mergeHorizontalSliceBinary(VecArr &Slice);
  void ShuffleIn(VecArr &Slice, unsigned StartIndex, unsigned EndIndex, VecVal &Operands);
  void ShuffleOut(VecArr &Slice, Instruction *WideInstruction);

public:
  llvm::StringRef getPassName() const override { return "IGCVectorCoalescer"; }
  IGCVectorCoalescer(const IGCVectorCoalescer &) = delete;
  IGCVectorCoalescer &operator=(const IGCVectorCoalescer &) = delete;
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<WIAnalysis>();
  }
  IGCVectorCoalescer();
  IGCVectorCoalescer(const std::string &FileName);
  static char ID;
};

}; // namespace IGC

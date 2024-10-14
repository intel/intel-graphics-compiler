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

class IGCVectorizer : public llvm::FunctionPass {

    typedef llvm::SmallPtrSet<Instruction*, 8> ValueSet;
    typedef llvm::SmallVector<Instruction*, 8> VecArr;
    typedef llvm::SmallVector<VecArr, 8> VectorSliceChain;
    typedef std::unordered_map<Instruction*, VecArr*> InstructionToSliceMap;

    struct InsertStruct {
        Instruction* Final = nullptr;
        // contains insert elements
        VecArr Vec;
        // contains slices of vector tree
        VectorSliceChain Chain;
    };

    CodeGenContext *CGCtx = nullptr;

    // when we vectorize, we build a new vector chain,
    // this map contains associations between scalar and vector
    // basically every element inside scalarSlice should point to the same
    // vectorized element which contains all of them
    std::unordered_map<Value*, Value*> ScalarToVector;
    InstructionToSliceMap InstructionToSlice;
    // all vector instructions that were produced for chain will be stored
    // in this array, used for clean up if we bail
    VecArr CreatedVectorInstructions;

    // logging
    std::unique_ptr<std::ofstream> OutputLogFile;
    std::string LogStr;
    llvm::raw_string_ostream OutputLogStream = raw_string_ostream(LogStr);
    void initializeLogFile(Function& F);
    void writeLog();

    void findInsertElementsInDataFlow(llvm::Instruction* I, VecArr& Chain);
    void collectScalarPath(VecArr& V, VectorSliceChain& Chain);
    bool checkSlice(VecArr& Slice, InsertStruct& InSt);
    bool processChain(InsertStruct& InSt);
    void clusterInsertElement(InsertElementInst* VecOfInsert, InsertStruct& InSt);
    void collectInstructionToProcess(VecArr& ToProcess, Function& F);

    bool checkPHI(Instruction* Compare, VecArr& Slice);
    bool handlePHI(VecArr& Slice, Type* VectorType);
    bool checkInsertElement(Instruction* First, VecArr& Slice);
    bool handleInsertElement(VecArr& Slice, Instruction* Final);
    bool checkExtractElement(Instruction* Compare, VecArr& Slice);
    bool handleExtractElement(VecArr& Slice);
    bool handleCastInstruction(VecArr& Slice);

    bool compareOperands(Value* A, Value* B);

    public:
    llvm::StringRef getPassName() const override { return "IGCVectorizer"; }
    virtual ~IGCVectorizer() { ScalarToVector.clear(); }
    IGCVectorizer(const IGCVectorizer&) = delete;
    IGCVectorizer& operator=(const IGCVectorizer&) = delete;
    virtual bool runOnFunction(llvm::Function &F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.addRequired<CodeGenContextWrapper>();
    }
    IGCVectorizer();
    IGCVectorizer(const std::string& FileName);
    static char ID;
};
}; // namespace IGC

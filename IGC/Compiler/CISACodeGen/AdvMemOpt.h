#ifndef _CISA_ADVMEMOPT_H_
#define _CISA_ADVMEMOPT_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    void initializeAdvMemOptPass(llvm::PassRegistry&);
    llvm::FunctionPass* createAdvMemOptPass();
} // End namespace IGC

#endif // _CISA_ADVMEMOPT_H_

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
See https://llvm.org/LICENSE.txt for license information.
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// \file
/// Utilities to print analysis info for various kinds of passes.
///
//===----------------------------------------------------------------------===//

// This file was copied from LLVM, because this code is not a part of any
// library in LLVM. No changes in the implementation are expected.

#ifndef VC_SUPPORT_PASSPRINTERS_H
#define VC_SUPPORT_PASSPRINTERS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class CallGraphSCCPass;
class FunctionPass;
class ModulePass;
class LoopPass;
class PassInfo;
class raw_ostream;
class RegionPass;
class Module;

} // end namespace llvm

// The namespace is changed from llvm to vc to avoid potential linking issues.
namespace vc {

llvm::FunctionPass *createFunctionPassPrinter(const llvm::PassInfo *PI,
                                              llvm::raw_ostream &out);

llvm::CallGraphSCCPass *createCallGraphPassPrinter(const llvm::PassInfo *PI,
                                                   llvm::raw_ostream &out);

llvm::ModulePass *createModulePassPrinter(const llvm::PassInfo *PI,
                                          llvm::raw_ostream &out);

llvm::LoopPass *createLoopPassPrinter(const llvm::PassInfo *PI,
                                      llvm::raw_ostream &out);

llvm::RegionPass *createRegionPassPrinter(const llvm::PassInfo *PI,
                                          llvm::raw_ostream &out);

} // end namespace vc

#endif // VC_SUPPORT_PASSPRINTERS_H

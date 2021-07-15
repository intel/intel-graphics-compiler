/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef DRIVER_VCPASSMANAGER_H
#define DRIVER_VCPASSMANAGER_H

#include <llvm/IR/LegacyPassManager.h>

//
// A simple wrapper over a legacy PassManager.
// Extends the default implementation by providing extra hooks that allow
// modification of the default pipeline - like an injection of additional
// verification passes. The intention behind these hooks is to simplify the
// debugging process.
//
//===----------------------------------------------------------------------===//

class VCPassManager : public llvm::legacy::PassManager {
public:
  void add(llvm::Pass *P) override;
};

#endif // DRIVER_VCPASSMANAGER_H

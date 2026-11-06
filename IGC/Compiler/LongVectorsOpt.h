/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class DataLayout;
class Pass;
class PassRegistry;
class Type;
} // namespace llvm

namespace IGC {
llvm::Pass *createLongVectorsOptPass();
void initializeLongVectorsOptPass(llvm::PassRegistry &);

/// Whether LongVectorsOpt may move values of type \p Ty to memory, including
/// private memory it allocates itself.
bool isLongVectorsOptCandidate(llvm::Type *Ty, const llvm::DataLayout &DL);
} // namespace IGC

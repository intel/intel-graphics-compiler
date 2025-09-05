/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_MC_MCSTREAMER_H
#define IGCLLVM_MC_MCSTREAMER_H

#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCStreamer.h>

namespace IGCLLVM {
inline void initSections(llvm::MCStreamer *streamer, bool NoExecStack, const llvm::MCContext *Context) {
  streamer->initSections(NoExecStack, *(Context->getSubtargetInfo()));
}

inline void switchSection(llvm::MCStreamer *streamer, llvm::MCSection *Section,
                          const llvm::MCExpr *Subsection = nullptr) {
#if (LLVM_VERSION_MAJOR < 15) || defined(IGC_LLVM_TRUNK_REVISION)
  streamer->SwitchSection(Section, Subsection);
#else
  streamer->switchSection(Section, Subsection);
#endif
}

inline void finish(llvm::MCStreamer *streamer) {
#if (LLVM_VERSION_MAJOR < 15) || defined(IGC_LLVM_TRUNK_REVISION)
  streamer->Finish();
#else
  streamer->finish();
#endif
}
} // namespace IGCLLVM

#endif

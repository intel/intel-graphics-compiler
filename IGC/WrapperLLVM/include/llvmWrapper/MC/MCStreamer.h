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
#if LLVM_VERSION_MAJOR >= 14
    streamer->initSections(NoExecStack, *(Context->getSubtargetInfo()));
#else
    streamer->InitSections(NoExecStack);
#endif
}
}

#endif

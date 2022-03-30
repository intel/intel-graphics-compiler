/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/GenXDiagnostic.h"

namespace vc {

const int DiagnosticInfo::KindID = llvm::getNextAvailablePluginDiagnosticKind();

} // namespace vc

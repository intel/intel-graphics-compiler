/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// Generic diagnostic info pass to not reinvent it anytime
//
//===----------------------------------------------------------------------===//

#ifndef VC_SUPPORT_COMPILATION_STAGE_H
#define VC_SUPPORT_COMPILATION_STAGE_H

namespace vc {

// NOTE: "Legalized" IR is the IR we have after GenXLowering pass
enum class LegalizationStage { NotLegalized, Legalized };

} // namespace vc

#endif // VC_SUPPORT_COMPILATION_STAGE_H

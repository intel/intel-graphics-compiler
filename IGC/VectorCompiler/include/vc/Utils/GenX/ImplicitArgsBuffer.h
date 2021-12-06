/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// implicit_args structure/buffer description and utils.
///
//===----------------------------------------------------------------------===//

#ifndef VC_UTILS_GENX_IMPLICITARGSBUFFER_H
#define VC_UTILS_GENX_IMPLICITARGSBUFFER_H

namespace vc {

namespace ImplicitArgs {

// An attribute that indicates that for the marked kernel implicit args buffer
// should be generated, e.g. kernel calls external function that uses some
// implicit argument.
inline const char KernelAttr[] = "RequiresImplArgsBuffer";

} // namespace ImplicitArgs

} // end namespace vc

#endif // VC_UTILS_GENX_IMPLICITARGSBUFFER_H

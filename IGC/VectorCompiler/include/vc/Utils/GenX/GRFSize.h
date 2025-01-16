/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_GRFSIZE_H
#define VC_UTILS_GENX_GRFSIZE_H

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/KernelInfo.h"

namespace vc {

inline int getGRFSize(const llvm::GenXBackendConfig *BC,
                      const llvm::GenXSubtarget *ST, const KernelMetadata &KM) {
  int NumGRF = -1;
  // Set by compile option.
  if (BC->isAutoLargeGRFMode())
    NumGRF = 0;
  if (BC->getGRFSize())
    NumGRF = BC->getGRFSize();
  // Set by kernel metadata.
  if (KM.getGRFSize()) {
    unsigned NumGRFPerKernel = *KM.getGRFSize();
    if (NumGRFPerKernel == 0 || ST->isValidGRFSize(NumGRFPerKernel))
      NumGRF = NumGRFPerKernel;
  }
  return NumGRF;
}

} // namespace vc

#endif // VC_UTILS_GENX_GRFSIZE_H

/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ! Keep them ordered !
GENX_VERIFY_STAGE(
    PostSPIRVReader, "post-spirv-reader",
    "Checks valid for early stage of IR acquired right after SPIRV->LLVM "
    "translation.")
GENX_VERIFY_STAGE(PostIrAdaptors, "post-ir-adaptors",
                  "Checks valid after IR adaptors run.")
GENX_VERIFY_STAGE(PostGenXLowering, "post-genx-lowering",
                  "Checks valid after GenXLowering pass.")
GENX_VERIFY_STAGE(PostGenXLegalization, "post-genx-legalization",
                  "Checks valid after GenXLegalization pass.")

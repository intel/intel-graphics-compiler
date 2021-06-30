/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common internal options {{
let Flags = InternalHelper.CommonInternalFlags in {

defm intel_use_bindless_buffers : CommonFlag<"intel-use-bindless-buffers">,
  HelpText<"Use bindless mode for buffers">;

}
// }} Common internal options

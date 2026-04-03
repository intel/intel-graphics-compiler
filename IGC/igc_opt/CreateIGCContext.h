/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace IGC {
class CodeGenContext;
}

enum class ClientApi {
  Unspecified,
  OCL,
  NEO
};

// Creates a CodeGenContext configured from cl::opt command-line options.
// Caller owns the returned pointer.
// Used by igc_opt main()
IGC::CodeGenContext *CreateCodeGenContext();

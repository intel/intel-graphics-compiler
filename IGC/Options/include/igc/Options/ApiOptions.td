/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by scalar and vector compilers.
//
// Api options are exposed to user via, e.g., openCL clBuildProgram.

include "llvm/Option/OptParser.td"

// Option kinds {{
// VC options accessible using public API.
def VCApiOption : OptionFlag;

// Api options compatible with igcmc.
// These are used only when -cmc is present in api options.
def IgcmcApiOption : OptionFlag;

// IGC options accessible using public API.
def IGCApiOption : OptionFlag;
// }} Option kinds

// Helpers {{
def ApiHelper {
  list<OptionFlag> VCBackwardCompatFlags = [VCApiOption, IgcmcApiOption];
  list<OptionFlag> CommonApiFlags = [VCApiOption, IGCApiOption];
}

include "OptionHelpers.td"
// }} Helpers

include "CommonApiOptions.td"
include "VCApiOptions.td"
include "IGCApiOptions.td"

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by scalar and vector compilers.
// Internal options are for passing of additional info of various purposes.
// Among these can be: debug, things that are not exposed to user directly.

include "llvm/Option/OptParser.td"

// Option kinds {{
// VC internal options.
def VCInternalOption : OptionFlag;

// IGC internal options.
def IGCInternalOption : OptionFlag;
// }} Option kinds

// Helpers {{
def InternalHelper {
  list<OptionFlag> CommonInternalFlags = [VCInternalOption, IGCInternalOption];
}

include "OptionHelpers.td"
// }} Helpers

include "CommonInternalOptions.td"
include "VCInternalOptions.td"
include "IGCInternalOptions.td"

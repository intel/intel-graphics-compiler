/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by scalar and vector compilers.
// Internal options are for passing of additional info of various purposes.
// Among these can be: debug, things that are not exposed to user directly.

include "llvm/Option/OptParser.td"

// Option kinds {{
// Vector Compiler internal options.
def VectorCompilerInternalOption : OptionFlag;

// Scalar Compiler internal options.
def ScalarCompilerInternalOption : OptionFlag;
// }} Option kinds

// Helpers {{
def InternalHelper {
  list<OptionFlag> CommonInternalFlags = [VectorCompilerInternalOption, ScalarCompilerInternalOption];
}

include "OptionHelpers.td"
// }} Helpers

include "CommonInternalOptions.td"
include "VectorCompilerInternalOptions.td"
include "ScalarCompilerInternalOptions.td"

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by scalar and vector compilers.
//
// Api options are exposed to user via, e.g., openCL clBuildProgram.

include "llvm/Option/OptParser.td"

// Option kinds {{
// Vector Compiler options accessible using public API.
def VectorCompilerApiOption : OptionFlag;

// Api options compatible with igcmc.
// These are used only when -cmc is present in api options.
def IgcmcApiOption : OptionFlag;

// Scalar Compiler options accessible using public API.
def ScalarCompilerApiOption : OptionFlag;
// }} Option kinds

// Helpers {{
def ApiHelper {
  list<OptionFlag> VCBackwardCompatFlags = [VectorCompilerApiOption, IgcmcApiOption];
  list<OptionFlag> CommonApiFlags = [VectorCompilerApiOption, ScalarCompilerApiOption];
}

include "OptionHelpers.td"
// }} Helpers

include "CommonApiOptions.td"
include "VectorCompilerApiOptions.td"
include "ScalarCompilerApiOptions.td"

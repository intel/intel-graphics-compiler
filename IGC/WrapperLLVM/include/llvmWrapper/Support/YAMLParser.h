/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_YAMLPARSER_H
#define IGCLLVM_SUPPORT_YAMLPARSER_H

#include "llvm/Support/YAMLParser.h"

#if LLVM_VERSION_MAJOR < 10
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::yaml::Hex64)
#endif

#endif // IGCLLVM_SUPPORT_YAMLPARSER_H
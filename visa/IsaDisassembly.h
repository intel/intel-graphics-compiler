/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
///
/// ISA IR Disassembler
///

#include "Common_ISA.h"

#include <list>
#include <string>

/// Looks up a GEN variable name
const char *getGenVarName(int id, const print_format_provider_t &header);

/// Exposing these declare print functions for use by verifier diagnostics code
/// or for disassembly output.
std::string printPredicateDecl(const print_format_provider_t *header,
                               unsigned declID);
std::string printVariableDecl(const print_format_provider_t *header,
                              unsigned declID, const Options *options);
std::string printAddressDecl(const print_format_provider_t *header,
                             unsigned declID);
std::string printSamplerDecl(const print_format_provider_t *header,
                             unsigned declID);
std::string printSurfaceDecl(const print_format_provider_t *header,
                             unsigned declID, unsigned numPredefinedSurfaces);
std::string printFuncInput(const print_format_provider_t *header,
                           unsigned declID, const Options *options);
std::string printOneAttribute(const print_format_provider_t *kernel,
                              const attribute_info_t *attr);
// Used for printing non-kernel attributes
// format:  attrs={attr0,attr1,......attrn}, where  each attr is
// AttrName|AttrName=<V>
std::string printAttributes(const print_format_provider_t *header,
                            const int attr_count,
                            const attribute_info_t *attrs);

/// Exposing these for inline asm code generation
std::string printVectorOperand(const print_format_provider_t *header,
                               const VISA_opnd *opnd, const Options *opt,
                               bool showRegion);
std::string printFunctionDecl(const print_format_provider_t *header,
                              bool isKernel);
std::string printBuildVersion(uint16_t major, uint16_t minor);

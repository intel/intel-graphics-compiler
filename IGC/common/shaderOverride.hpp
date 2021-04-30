/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <string>
#include "inc/common/igfxfmid.h"

void appendToShaderOverrideLogFile(std::string const & binFileName, const char * message);
void overrideShaderBinary(void *& genxbin, int & binSize, std::string const & binFileName, bool &binOverride);
void overrideShaderIGA(PLATFORM const & platform, void *& genxbin, int & binSize, std::string const & binFileName, bool &binOverride);

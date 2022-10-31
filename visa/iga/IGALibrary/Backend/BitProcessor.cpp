/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BitProcessor.hpp"
#include "../strings.hpp"

#include <stdarg.h>
#include <stdio.h>

using namespace iga;

void BitProcessor::warningAtS(const Loc &loc, std::string msg) {
  m_errorHandler.reportWarning(loc, msg);
}
void BitProcessor::errorAtS(const Loc &loc, std::string msg) {
  m_errorHandler.reportError(loc, msg);
}
void BitProcessor::fatalAtS(const Loc &loc, std::string msg) {
  m_errorHandler.throwFatal(loc, msg);
}

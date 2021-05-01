/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGIL_LOADBUFFER_H
#define IGIL_LOADBUFFER_H

#include <string>
#include <list>
#include <memory>

  #define LLVM_EXIT(a) { (throw std::exception()); }

namespace llvm {
    char* LoadCharBufferFromResource(unsigned int ResNumber, const char *pResType, long unsigned int& m_buffSize);
} // namespace llvm

#endif // IGIL_LOADBUFFER_H


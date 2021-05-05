/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_UTILS_H
#define IGCLLVM_UTILS_H

namespace IGCLLVM
{
template<typename T>
inline T& getref(T& ref) { return ref; }

template<typename T>
inline T& getref(T* ptr) { return *ptr; }

template<typename T>
inline T* getptr(T& ref) { return &ref; }

template<typename T>
inline T* getptr(T* ptr) { return ptr; }
}

#endif



/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cstdlib>

namespace CIF {

inline void AbortImpl(){
    std::abort();
}

template<typename T = void>
inline T Abort();

template<typename T>
inline T Abort(){
    AbortImpl();
    T t{};
    return t;
}

template<>
inline void Abort() {
    AbortImpl();
}

namespace Sanity{

template<typename T>
inline T *NotNullOrAbort(T *ptr){
    if(ptr == nullptr){
        Abort();
    }
    return ptr;
}

template<typename T>
inline T &ToReferenceOrAbort(T *ptr){
    if(ptr == nullptr){
        Abort();
    }
    return *ptr;
}

}

}

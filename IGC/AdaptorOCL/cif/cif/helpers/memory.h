/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/helpers/error.h"
#if !defined(_WIN32)
#include "inc/common/secure_mem.h"
#endif

namespace CIF{

inline void SafeCopy(void *dst, size_t dstAvailableSizeInBytes, const void *src, size_t bytesToCopy){
    if(dstAvailableSizeInBytes < bytesToCopy){
        CIF::Abort();
    }

    if(bytesToCopy == 0){
        return;
    }

    if((dst == nullptr) || (src == nullptr)){
        CIF::Abort();
    }

    char * dstAsChar = reinterpret_cast<char*>(dst);
    const char * srcAsChar = reinterpret_cast<const char*>(src);

    // TODO : Verify that compiler actually picks this pattern-up as memcpy and optimizes it
    memcpy_s(dstAsChar, dstAvailableSizeInBytes, srcAsChar, bytesToCopy);
}

template<typename T>
inline void SafeCopy(void *dst, size_t dstAvailableSizeInBytes, const T &v){
    SafeCopy(dst, dstAvailableSizeInBytes, &v, sizeof(T));
}

inline void SafeZeroOut(void *dst, size_t dstAvailableSizeInBytes){
    char * dstAsChar = reinterpret_cast<char*>(dst);

    // TODO : Verify that compiler actually picks this pattern-up as memcpy and optimizes it
    for(size_t i = 0; i < dstAvailableSizeInBytes; ++i){
        dstAsChar[i] = 0;
    }
}

template<typename T>
inline void SafeZeroOut(T &v){
    SafeZeroOut(&v, sizeof(T));
}

template<typename PtrElT, typename OffsetT>
inline PtrElT *OffsetedPtr(PtrElT *ptr, OffsetT offset){
    return reinterpret_cast<PtrElT*>(reinterpret_cast<char *>(ptr) + offset);
}

template<typename PtrElT, typename OffsetT>
inline const PtrElT *OffsetedPtr(const PtrElT *ptr, OffsetT offset){
    return reinterpret_cast<const PtrElT*>(reinterpret_cast<char *>(ptr) + offset);
}

}

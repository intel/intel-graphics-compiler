/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#pragma once

#include "cif/helpers/error.h"

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
    for(size_t i = 0; i < bytesToCopy; ++i){
        dstAsChar[i] = srcAsChar[i];
    }
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

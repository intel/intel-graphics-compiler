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

#include <atomic>
#include <cinttypes>
#include <memory>

#include "cif/common/id.h"
#include "cif/common/cif.h"
#include "cif/helpers/error.h"

namespace CIF{

struct PimplBase
{
    PimplBase(){
        referenceCounter.store(0U);
    }
    virtual ~PimplBase() = default;

    virtual void Retain(CIF::ICIF *retainer){
        ++referenceCounter;
    }

    virtual std::unique_ptr<PimplBase> Release(CIF::ICIF *releaser){
        auto prevRefCount = referenceCounter.load();
        --referenceCounter;

        if(prevRefCount == 0){
            CIF::Abort();
        }

        if(prevRefCount > 1){
            return std::unique_ptr<PimplBase>(nullptr);
        }

        assert(prevRefCount == 1);
        // this was the last reference to this object
        // make sure the caller will delete this
        return std::unique_ptr<PimplBase>(this);
    }

protected:
    std::atomic<std::uint32_t> referenceCounter;
};

}

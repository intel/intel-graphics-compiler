/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

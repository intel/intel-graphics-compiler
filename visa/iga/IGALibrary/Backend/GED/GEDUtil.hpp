/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_GEDUTIL_H_
#define _IGA_BACKEND_GED_GEDUTIL_H_

#include "../../Models/OpSpec.hpp"
#include "ged.h"

// These interfaces can be used by IGA to encapsulate usage of GED enums
// and the GED api.
//
// Some other stuff common to both the encoder and decoder may go
// in here as well.
namespace iga {
//    iga::SFID getSFID(Platform p, const OpSpec &os, uint32_t exDesc, uint32_t
//    desc);
iga::SFMessageType getMessageType(Platform p, SFID sfid, uint32_t desc);
uint32_t getMessageLengths(Platform p, const OpSpec &os, uint32_t exDesc,
                           uint32_t desc, uint32_t *mLen, uint32_t *emLen,
                           uint32_t *rLen);

} // namespace iga

#endif //_IGA_GEDUTIL_H_

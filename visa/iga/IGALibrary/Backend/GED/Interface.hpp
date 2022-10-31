/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_INTERFACE_HPP_
#define _IGA_BACKEND_GED_INTERFACE_HPP_

////////////////////////////////////////////
// GED ENCODER USERS USE THIS INTERFACE
#include "../../ErrorHandler.hpp"
#include "../../IR/Kernel.hpp"
#include "../DecoderOpts.hpp"
#include "../EncoderOpts.hpp"

namespace iga {
namespace ged {
bool IsEncodeSupported(const Model &m, const EncoderOpts &opts);
void Encode(const Model &m, const EncoderOpts &opts, ErrorHandler &eh,
            Kernel &k, void *&bits, size_t &bitsLen);

bool IsDecodeSupported(const Model &m, const DecoderOpts &opts);
Kernel *Decode(const Model &m, const DecoderOpts &dopts, ErrorHandler &eh,
               const void *bits, size_t bitsLen);
} // namespace ged
} // namespace iga

#endif // _IGA_BACKEND_GED_INTERFACE_HPP_

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_FRONTEND_SEND_DESCRIPTOR_DECODING_HPP
#define IGA_FRONTEND_SEND_DESCRIPTOR_DECODING_HPP

#include "../IR/Types.hpp"
#include "../Models/Models.hpp"

#include <sstream>

namespace iga
{
    void EmitSendDescriptorInfo(
        Platform p,
        SFID sfid,
        ExecSize execSize,
        bool dstNonNull,
        int dstLen,
        int src0Len,
        int src1Len,
        const SendDesc &exDesc,
        const SendDesc &desc,
        std::stringstream &ss);

} // iga::

#endif

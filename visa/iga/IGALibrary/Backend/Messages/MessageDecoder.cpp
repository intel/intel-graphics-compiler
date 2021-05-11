/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <tuple>
#include <utility>


using namespace iga;

void MessageDecoder::decodePayloadSizes() {
    bool hasMLenRLenInDesc = true;
    bool hasXLenInExDesc = true;
    auto plural = [](int x) {return x == 1 ? "" : "s";};
    if (hasMLenRLenInDesc) {
        decodeDescField("Mlen", 25, 4,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " address register" << plural(val) << " written";
            });
        decodeDescField("Rlen", 20, 5,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " register" << plural(val) << " read back";
            });
    }
    if (hasXLenInExDesc) {
        decodeDescField("Xlen", 32 + 6, 5,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " data register" << plural(val) << " written";
            });
    }
    if (platform() <= Platform::GEN11) {
        decodeDescField("SFID", 32 + 0, 4,
            [] (std::stringstream &ss, uint32_t val) {
                ss << val << " shared function ID";
            });
    }
}


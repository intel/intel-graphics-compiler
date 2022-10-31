/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_INSTDIFF_HPP
#define IGA_INSTDIFF_HPP

#include "IR/Types.hpp"
#include "api/iga.h"

#include <cstdint>
#include <ostream>

namespace iga {
// returns true if a warning or error was emitted
iga_status_t DecodeFields(Platform p, int verbosity, bool useNativeDecoder,
                          uint32_t fmtOpts, std::ostream &os,
                          const uint8_t *bits, size_t bitsLen);

// returns true if a warning or error was emitted
iga_status_t DiffFields(Platform p, int verbosity, bool useNativeDecoder,
                        uint32_t fmtOpts, std::ostream &os, const char *source1,
                        const uint8_t *bits1, size_t bitsLen1,
                        const char *source2, const uint8_t *bits2,
                        size_t bitsLen2);
iga_status_t DiffFieldsFromPCs(Platform p, int verbosity, bool useNativeDecoder,
                               uint32_t fmtOpts, std::ostream &os,
                               const char *source1, size_t pc1,
                               const uint8_t *bits1, size_t bitsLen1,
                               const char *source2, size_t pc2,
                               const uint8_t *bits2, size_t bitsLen2);

// returns true if a warning or error was emitted
iga_status_t DebugCompaction(Platform p, int verbosity, bool useNativeDecoder,
                             uint32_t fmtOpts, std::ostream &os,
                             const uint8_t *bits, size_t bitsLen);
} // namespace iga

#endif // IGA_INSTDIFF_HPP

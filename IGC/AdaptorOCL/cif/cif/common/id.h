/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cinttypes>
#include <string>

#include "cif/common/coder.h"

namespace CIF {

using InterfaceId_t = uint64_t;
using Version_t = uint64_t;

using InterfaceIdCoder = Coder<InterfaceId_t>;
using VersionCoder = Coder<Version_t>;

constexpr Version_t InvalidVersion = std::numeric_limits<Version_t>::max();
constexpr Version_t UnknownVersion = InvalidVersion - 1;
constexpr Version_t TraitsSpecialVersion = UnknownVersion - 1;
constexpr Version_t AnyVersion = UnknownVersion;

constexpr Version_t BaseVersion = 0;
constexpr Version_t MinVersion = BaseVersion + 1;
constexpr Version_t MaxVersion = TraitsSpecialVersion - 1;

constexpr InterfaceId_t InvalidInterface =
    std::numeric_limits<InterfaceId_t>::max();
constexpr InterfaceId_t UnknownInterface = InvalidInterface - 1;
}

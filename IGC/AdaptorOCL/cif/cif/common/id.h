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

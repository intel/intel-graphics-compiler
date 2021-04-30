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

#include <cstring>
#include "ged_base.h"
#include "ged_validation_utils.h"

using std::memcmp;


//#if GED_VALIDATION_API

bool ged_ins_field_mapping_fragment_t::operator<(const ged_ins_field_mapping_fragment_t& rhs) const
{
    GEDASSERT(_from._lowBit != rhs._from._lowBit);
    return (_from._lowBit < rhs._from._lowBit);
}

bool ged_ins_field_mapping_fragment_t::operator==(const ged_ins_field_mapping_fragment_t& rhs) const
{
    return (0 == memcmp(this, &rhs, sizeof(ged_ins_field_mapping_fragment_t)));
}

//#endif

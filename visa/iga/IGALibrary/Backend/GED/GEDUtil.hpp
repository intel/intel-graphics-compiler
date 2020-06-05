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

#ifndef _IGA_BACKEND_GED_GEDUTIL_H_
#define _IGA_BACKEND_GED_GEDUTIL_H_


#include "../../Models/OpSpec.hpp"
#include "ged.h"


// These interfaces can be used by IGA to encapsulate usage of GED enums
// and the GED api.
//
// Some other stuff common to both the encoder and decoder may go
// in here as well.
namespace iga
{
//    iga::SFID getSFID(Platform p, const OpSpec &os, uint32_t exDesc, uint32_t desc);
    iga::SFMessageType getMessageType(Platform p, SFID sfid, uint32_t desc);
//    uint32_t getMessageLengths(Platform p, const OpSpec &os, uint32_t exDesc, uint32_t desc, uint32_t* mLen, uint32_t* emLen, uint32_t* rLen);


}

#endif //_IGA_GEDUTIL_H_

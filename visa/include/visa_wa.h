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
/**
** File Name     : visa_wa.h
**
** Abstract      : Declaration of all SW workarounds implemented in vISA finalizer.
**
**  ---------------------------------------------------------------------- */

#ifndef _VISA_WA_H_
#define _VISA_WA_H_

#define VISA_WA_DECLARE( wa, wa_comment, wa_bugType ) unsigned int wa : 1;
#define VISA_WA_INIT( wa ) wa = 0;
#define VISA_WA_ENABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = true;                  \
}
#define VISA_WA_DISABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = false;                  \
}
#define VISA_WA_CHECK(pWaTable, w)  ((pWaTable)->w)

enum VISA_WA_BUG_TYPE
{
    VISA_WA_BUG_TYPE_UNKNOWN    = 0,
    VISA_WA_BUG_TYPE_CORRUPTION = 1,
    VISA_WA_BUG_TYPE_HANG       = 2,
    VISA_WA_BUG_TYPE_PERF       = 4,
    VISA_WA_BUG_TYPE_FUNCTIONAL = 8,
    VISA_WA_BUG_TYPE_SPEC       = 16,
    VISA_WA_BUG_TYPE_FAIL       = 32
};

#endif

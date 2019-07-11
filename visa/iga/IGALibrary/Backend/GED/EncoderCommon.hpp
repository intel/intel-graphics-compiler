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
#ifndef _IGA_ENCODER_COMMON_H_
#define _IGA_ENCODER_COMMON_H_

#include "ged.h"

namespace iga
{
// IGA encoder common macros

// #define GED_TRACE_SETTERS
//
#ifdef GED_TRACE_SETTERS
#define GED_TRACE_SETTER(X) X
#else
#define GED_TRACE_SETTER(X)
#endif

#define GED_ENCODE(FUNC, ARG) \
    GED_ENCODE_TO(FUNC, ARG, &m_gedInst)
#if defined(GED_TIMER) || defined(_DEBUG)
#define START_GED_TIMER() startIGATimer(TIMER_GED);
#define STOP_GED_TIMER()  stopIGATimer(TIMER_GED);
#else
#define START_GED_TIMER()
#define STOP_GED_TIMER()
#endif

#if defined(TOTAL_ENCODE_TIMER) || defined (_DEBUG)
#define START_ENCODER_TIMER() startIGATimer(TIMER_TOTAL);
#define STOP_ENCODER_TIMER()  stopIGATimer(TIMER_TOTAL);
#else
#define START_ENCODER_TIMER()
#define STOP_ENCODER_TIMER()
#endif
#ifdef GED_DEBUG_PRINT
static const bool gedDebugPrint = true;
#else
static const bool gedDebugPrint = false;
#endif

#if GED_VALIDATION_API
static const bool print_ged_debug = true;
#else
static const bool print_ged_debug = false;
static GED_RETURN_VALUE GED_PrintFieldBitLocation(
    const ged_ins_t* ins, const GED_INS_FIELD field)
{
    return GED_RETURN_VALUE_SUCCESS;
}
#endif


#define GED_ENCODE_TO(FUNC, ARG, GED) \
    do { \
        GED_RETURN_VALUE _status; \
        if(print_ged_debug) { \
          std::cout << "FIELD: " << #FUNC << std::endl; \
          GED_PrintFieldBitLocation(GED, GED_INS_FIELD_ ## FUNC); \
        } \
        START_GED_TIMER() \
        _status = GED_Set ## FUNC (GED, ARG); \
        STOP_GED_TIMER() \
        GED_TRACE_SETTER(printf("%s <= GED_%s(...,%d)\n", \
            gedReturnValueToString(_status), #FUNC, (int)ARG)); \
        if (_status != GED_RETURN_VALUE_SUCCESS) { \
            handleGedError(__LINE__, #FUNC, _status); \
        } \
    } while(0)

} // iga::
#endif //_IGA_ENCODER_COMMON_H_

/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef GED_BASE_H
#define GED_BASE_H

#include <cassert>
#include <iostream>
#include <string>
#include <cstdlib>
#include "common/ged_types_internal.h"

using std::cerr;
using std::endl;
using std::flush;
using std::string;


enum GED_ERROR_TYPE
{
    GED_ERROR_TYPE_SUCCESS = 0,
    GED_ERROR_TYPE_BAD_OPTIONS,
    GED_ERROR_TYPE_NYI,
    GED_ERROR_TYPE_GENERAL_ERROR
};


extern const char* gedVersion;

# define GEDVERSION(ost) ost << "GED VERSION: " << gedVersion << endl;


# if defined(GED_DEBUG)
#   ifndef GED_VALIDATE
#     define GED_VALIDATE
#   endif
#   define GEDASSERT(cond) assert(cond)
# else
#   if defined(GED_VALIDATE)
#     define GEDASSERT(cond) { if (0 == (cond)) { cerr << "GED ASSERTION: " #cond << endl << \
                               "file " << __FILE__ << ", line " << __LINE__ << endl << flush; exit(1); } }
#   else
#     define GEDASSERT(cond)
#   endif
# endif

# if defined(GED_VALIDATE)
#  define COMMA ,
#  define GEDFORASSERT(expr) expr
#  define GEDASSERTM(cond, msg) { if (0 == (cond)) { cerr << msg << endl << flush; GEDASSERT(cond); } }
#  define GEDERRORC(code, msg) { cerr << "GED ERROR: " << __FILE__ << "::" << __FUNCTION__ << endl << msg << endl << flush; \
                                 GEDASSERT(0); }
# else // not defined GED_VALIDATE
#  define COMMA
#  define GEDFORASSERT(expr)
#  define GEDASSERTM(cond, msg)
#  define GEDERRORC(code, msg) { cerr << "GED ERROR: " << msg << endl; GEDVERSION(cerr); cerr.flush(); exit(code); }
# endif // end of not defined GED_VALIDATE


# if defined(GED_IGNORE_WARNINGS)
#  define GEDWARNING(msg)
# else
#  define GEDWARNING(msg) { cerr << "GED WARNING: " << __FILE__ << "::" << __FUNCTION__ << endl << msg << endl << flush; }
# endif // GED_IGNORE_WARNINGS


# define GEDBADOPTION(msg)   { cerr << "USAGE ERROR: " << msg << endl << flush; exit(GED_ERROR_TYPE_BAD_OPTIONS); }
# define GEDERROR(msg)       GEDERRORC(GED_ERROR_TYPE_GENERAL_ERROR, msg)
# define NYI                 GEDERRORC(GED_ERROR_TYPE_NYI, string(__FILE__) + "::" + __FUNCTION__ + " is not yet implemented")

#endif // GED_BASE_H

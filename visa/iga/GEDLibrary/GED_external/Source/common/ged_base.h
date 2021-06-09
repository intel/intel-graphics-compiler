/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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

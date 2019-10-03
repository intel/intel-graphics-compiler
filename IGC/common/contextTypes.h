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
#ifndef ContextTypes_h_INCLUDED
#define ContextTypes_h_INCLUDED

#if defined _USC_
#   include "IGC/common/igc_debug.h"
#   include "3d/common/iStdLib/types.h"
#   include "API/ShaderTypes.h"
#else
#   include "API/ShaderTypes.h"
#endif

#include "ErrorCode.h"

namespace GHAL3D
{

/*****************************************************************************\
ENUM: GHAL3D_CONTEXT_TYPE
\*****************************************************************************/
enum GHAL3D_CONTEXT_TYPE
{
    GHAL3D_CONTEXT_IMMEDIATE,
    GHAL3D_CONTEXT_DEFERRED,
    NUM_GHAL3D_CONTEXT_TYPES
};

/*****************************************************************************\
ENUM: SHADER_TYPE
\*****************************************************************************/
#if !defined _USC_ && !defined OPENGL_IMOLA
enum SHADER_TYPE
{
    VERTEX_SHADER,
    GEOMETRY_SHADER,
    PIXEL_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    COMPUTE_SHADER,
    NUM_SHADER_TYPES
};
#endif //!defined(_USC_)

#ifndef OPENGL_IMOLA
/*****************************************************************************\
STRUCT: RETVAL
\*****************************************************************************/
struct RETVAL
{
    // External values
    DWORD       Success             : 1;    // Call was successful
    DWORD       Error               : 1;    // Invalid call
    DWORD       OutOfSystemMemory   : 1;    // System memory allocation failed
    DWORD       Busy                : 1;    // Compilation not done yet
    DWORD       _Unused             : 28;   // Reserved // For GCC 4.7 bug (do not allow to static initialize anonymous members)

    RETVAL& operator = (const ErrorCode&);
    operator ErrorCode();                   // convertion operator to ErrorCode API type
};

C_ASSERT( sizeof( RETVAL ) == sizeof( ErrorCode ) );


inline RETVAL& RETVAL::operator = (const ErrorCode& errorCode)
{
    Success            = errorCode.Success;
    Error              = errorCode.Error;
    OutOfSystemMemory  = errorCode.OutOfSystemMemory;
    Busy               = errorCode.Busy;

    return *this;
}

inline RETVAL::operator ErrorCode()
{
    return * reinterpret_cast<ErrorCode*>(this);
}

/*****************************************************************************\
CONST: g_cInitRetVal
\*****************************************************************************/
const RETVAL g_cInitRetVal =
{
    true,   // Success
    false,  // Error
    false,  // OutOfSystemMemory
    false,  // Busy
};

C_ASSERT(sizeof(g_cInitRetVal) == sizeof(g_cInitErrorCode));

#endif

/*****************************************************************************\
TYPEDEF: HANDLE_TYPE
\*****************************************************************************/
struct HANDLE_TYPE
{
    void*   handle;

    bool operator == ( const HANDLE_TYPE& rvalue ) const;
    bool operator != ( const HANDLE_TYPE& rvalue ) const;
};

inline bool HANDLE_TYPE::operator == ( const HANDLE_TYPE& rvalue ) const
{
    return handle == rvalue.handle;
};

inline bool HANDLE_TYPE::operator != ( const HANDLE_TYPE& rvalue ) const
{
    return handle != rvalue.handle;
}

/*****************************************************************************\
TYPEDEF: STATE_HANDLE
\*****************************************************************************/
typedef HANDLE_TYPE   STATE_HANDLE;

/*****************************************************************************\
CONST: DISABLE_HANDLE
\*****************************************************************************/
const HANDLE_TYPE DISABLE_HANDLE = { 0 };

} // namespace GHAL3D

#endif //ContextTypes_h_INCLUDED

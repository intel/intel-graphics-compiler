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
#ifndef __USC_OSS_H__
#define __USC_OSS_H__

#include <stddef.h>
#include "usc_config.h"

namespace USC 
{

/*****************************************************************************\
DEFINE: USC_REGISTRY_KEY
\*****************************************************************************/
#define USC_REGISTRY_KEY "SOFTWARE\\INTEL\\IGFX\\USC"

#if defined( _DEBUG ) || defined( _INTERNAL )

/*****************************************************************************\
ENUM: COMPILATION_STAGE
Description:
    List of compilation stages to be used to determine shader dump type
    of the client dump call.
\*****************************************************************************/
enum COMPILATION_STAGE
{
    COMPILATION_STAGE_IL = 1,
    COMPILATION_STAGE_LIR = 2,
    COMPILATION_STAGE_ISA = 4,
    NUM_COMPILATION_STAGES
};

/*****************************************************************************\

Function pointer:
    PFNREADREGISTRY

Description:
    Called to read data from the OS registry.

Input:    
    const char* pName   - A string that contains the name of the registry value.
    void* pValue        - A pointer to the location to write the value.
    unsigned int size   - The maximum size of the data to be written.

Output:
    void*  pValue       - The value of the registry key, if successful.
    bool                - True if the call was successful.

\*****************************************************************************/
typedef bool ( __stdcall* PFNREADREGISTRY )(
    const char*, void* pValue, unsigned int size );

/*****************************************************************************\

Function pointer:
    PFNDUMPOPEN

Description:
    Called to open a dump file for shader with given hash code.

Input:    
    unsigned long long hash
    COMPILATION_STAGE dumpLevel

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNDUMPOPEN )( 
    unsigned long long, USC::COMPILATION_STAGE );

/*****************************************************************************\

Function pointer:
    PFNDUMPWRITE

Description:
    Called to write a string to dump file for shader with given hash code.

Input:    
    unsigned long long hash
    COMPILATION_STAGE dumpLevel
    const char* str 

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNDUMPWRITE )(
    unsigned long long, USC::COMPILATION_STAGE, const char* );

/*****************************************************************************\

Function pointer:
    PFNDUMPCLOSE

Description:
    Called to close a dump file for shader with given hash code.

Input:    
    unsigned long long hash
    COMPILATION_STAGE dumpLevel

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNDUMPCLOSE )( 
    unsigned long long, USC::COMPILATION_STAGE );

/*****************************************************************************\

Function pointer:
    PFNDEVELOPEREVENT

Description:
    Called when a specific developer event happens.

Input:    
    unsigned int - Developer event identifier.
    unsigned int - Value dependent on the implementation.

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNDEVELOPEREVENT )(
    unsigned int, unsigned int );

/*****************************************************************************\

Function pointer:
    PFNPRINTDEBUGMESSAGE

Description:
    Called to print a debug message.

Input: 
    const char* str     - text of the message.    

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNPRINTDEBUGMESSAGE )( 
    const char* );

/*****************************************************************************\

Function pointer:
    PFNDEBUGBREAK

Description:
    Called to break into debugger

Input:    
    void

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNDEBUGBREAK )( 
    void );

/*****************************************************************************\

Function pointer:
    PFNASSERTHIT

Description:
    Called after printing assert message and before breaking into debugger.

 Input:
    const bool allowPrinting  - Debug condition check based on debug level
    const char* prefix        - Prefix printed before each line of assert information
    const char* file          - String containing file in wich assert was hit
    const char* func          - String containing name of function in wich assert 
                                was hit.
    const char* expr          - String containing expression which triggered assert
                                to be hit.
    const int lineNumber      - Number of line in which assert was hit.

Output:
    none

\*****************************************************************************/
typedef void ( __stdcall* PFNASSERTHIT )(
    const bool, const char*, const char*, const char*, const char*, const int );

/*****************************************************************************\

Function pointer:
    PFNMALLOC

Description:
    Called from CAllocator::Allocate() to allocate the memory.

Input:    
    size_t - size of the memory allocation

Output:
    void * - pointer to the allocated memory.

\*****************************************************************************/
typedef void * ( __cdecl* PFNMALLOC )( size_t );

/*****************************************************************************\

Function pointer:
    PFNFREE

Description:
    Called from CAllocator::Deallocate() to free the memory allocation.

Input:    
    void * - pointer to the allocated memory.

Output:
    none

\*****************************************************************************/
typedef void ( __cdecl* PFNFREE )( void * );

#endif // defined( _DEBUG ) || defined( _INTERNAL )

/*****************************************************************************\
Struct: SClientCallbacks
Description: 
    A vector of function pointers to client functions providing specific 
    services or functions acting as event receivers. 
    A client may set those pointers with SetClientCallbacks function.
\*****************************************************************************/
struct SClientCallbacks
{    
#if defined( _DEBUG ) || defined( _INTERNAL )
    PFNREADREGISTRY       pfnReadRegistry;    
    PFNDUMPOPEN           pfnDumpOpen;
    PFNDUMPWRITE          pfnDumpWrite;
    PFNDUMPCLOSE          pfnDumpClose;
    PFNDEVELOPEREVENT     pfnDeveloperEvent;
#if defined( _DEBUG )
    PFNPRINTDEBUGMESSAGE  pfnPrintDebugMessage;
    PFNDEBUGBREAK         pfnDebugBreak;
    PFNASSERTHIT          pfnAssertHit;    
#endif // _DEBUG
#endif // defined( _DEBUG ) || defined( _INTERNAL )
};

void USC_API_CALL SetClientCallbacks( struct SClientCallbacks const * const pClientCallbacks );

} // namespace USC
#endif // __USC_OSS_H__

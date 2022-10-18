/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*****************************************************************************\
Abstract:  Provides all the registry reading/writing code for Intel
           registry keys.
\*****************************************************************************/

#pragma once

#if defined(_DEBUG) || defined(_INTERNAL)

#if defined _WIN32
# define OCLRT_REGKEY   "Software\\Intel\\IGFX\\OCL"

// Prevent windows.h from defining max macro; its definition conflicts with MathExtras.h's usage.
#ifndef NOMINMAX
# define NOMINMAX
#endif
# include <windows.h>

# include <windef.h>
# include <stdio.h>
#else
# include <string.h>
#endif

#include <stdlib.h>

namespace llvm
{
//TO REMOVE
//typedef unsigned char byte;

/*****************************************************************************\
                                    Windows

Inline Function: ReadRegistry

Description:  Reads and returns a DWORD value from the key path specified.
              If the key path cannot be found or cannot be opened the default
              value supplied will be returned.

\*****************************************************************************/
#if defined _WIN32
inline DWORD ReadRegistry( const char* pKeyName, const DWORD defaultValue )
{
    HKEY Key;
    DWORD value = defaultValue;
    DWORD success = ERROR_SUCCESS;

    success = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                             OCLRT_REGKEY,
                             0,
                             KEY_READ,
                             &Key );

    if( ERROR_SUCCESS == success )
    {
        DWORD regType;
        DWORD size = sizeof( ULONG );

        success = RegQueryValueExA( Key,
                                    pKeyName,
                                    NULL,
                                    &regType,
                                    (unsigned char*) &value,
                                    &size );

        // close key
        RegCloseKey( Key );
    }

    return value;
}

#else

/*****************************************************************************\
                               Unix Based Systems

Inline Function: ReadRegistry

Description:  Reads and returns a uint32_t value from the environment variable
              specified.  If the variable is not defined then the value
              supplied will be returned.

\*****************************************************************************/
inline uint32_t ReadRegistry( const char* pKeyName, const uint32_t defaultValue )
{
#ifdef ANDROID
    const char* envVal = getenv(pKeyName);
#else
    std::string pKey = "OCL_";
    pKey += pKeyName;
    const char* envVal = getenv(pKey.c_str());
#endif
    uint32_t value = defaultValue;

    if (envVal)
        value = (uint32_t)atoi(envVal);

    return value;
}
#endif

#ifdef _WIN32
/*****************************************************************************\
                                    Windows

Inline Function: ReadRegistryBinary

Description:  Reads and returns a value from the key path specified.
              If the key path cannot be found or cannot be opened the default
              value supplied will be returned.

              Use this for reading non-DWORD key values.

              false is returned only if the supplied buffer is too small
              or NULL pointers were supplied.

\*****************************************************************************/
inline bool ReadRegistryBinary( const char* pKeyName, DWORD regType, void* &pData, DWORD* pDataSize )
{
    HKEY Key;
    DWORD success = ERROR_SUCCESS;

    if ( !pKeyName || !pData || !pDataSize )
        return false;

    success = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                             OCLRT_REGKEY,
                             0,
                             KEY_READ,
                             &Key );

    if( ERROR_SUCCESS == success )
    {
        success = RegQueryValueExA( Key,
                                    pKeyName,
                                    NULL,
                                    &regType,
                                    (BYTE*)pData,
                                    pDataSize );

        // close key
        RegCloseKey( Key );

        // If supplied buffer was too small, it is now undefined and shouldn't be used
        if ( ERROR_MORE_DATA == success )
            return false;
    }

    return true;  /* Also returns TRUE when key is not found */
}

#else

/*****************************************************************************\
                               Unix Based Systems

Inline Function: ReadRegistryBinary

Description:  Reads and returns a value from the environment variable specified.
              If the key is not present the default value supplied will be
              returned.

              Use this for reading NULL-terminated key values.

              Returns false if supplied buffer is too small to receive data or
              NULL pointers were provided.  Returns true when caller is safe to
              use data stored in pData.

\*****************************************************************************/
inline bool ReadRegistryBinary( const char* pKeyName, uint32_t regType, void* &pData, uint32_t* pDataSize )
{
    bool rvalue = true;

    if (!pData || !pKeyName || !pDataSize)
        return false;

#ifdef ANDROID
    const char* envVal = getenv(pKeyName);
#else
    std::string pKey = "OCL_";
    pKey += pKeyName;
    const char* envVal = getenv(pKey.c_str());
#endif

    if (envVal)
    {
        if (*pDataSize <= strlen((const char*)pData))
        {
            *pDataSize = 0;
            rvalue = false;
        }
        else
        {
            strncpy((char*)pData, envVal, *pDataSize);
        }
    }

    return rvalue;  /* Also returns TRUE when variable is not found */
}
#endif

#if defined _WIN32
/*****************************************************************************\

Inline Function: WriteRegistry

Description:  Writes a DWORD to the specified key path.  Returns "true" if
              successful and "false" if the key cannot be written.

\*****************************************************************************/
inline bool WriteRegistry( const char* pKeyName, const unsigned long value )
{
    HKEY Key;

    if ( ERROR_SUCCESS == RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                                        OCLRT_REGKEY,
                                        0,
                                        KEY_WRITE,
                                        &Key ) )
    {
        //ULONG data = 0;
        DWORD size = sizeof( ULONG );

        RegSetValueExA( Key,
                        pKeyName,
                        0,
                        REG_DWORD,
                        (unsigned char*) &value,
                        size );

        RegCloseKey( Key );

        return true;
    }

    return false;
}
#endif

} // namespace OCLRT

#endif

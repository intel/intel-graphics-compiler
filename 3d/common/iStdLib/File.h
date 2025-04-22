/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <errno.h>
#include "MemCopy.h"

#include "Debug.h"

#if defined _WIN32
#   include <direct.h>
#   include <wtypes.h>
#   include <winbase.h>
#   include <process.h>
// disable warning about deprecated functions:
// 'vsprintf' and 'fopen'
#   pragma warning(push)
#   pragma warning(disable: 4996)
#elif defined(__GNUC__)
#   include <libgen.h>
#   include <sys/stat.h>
#endif


namespace iSTD
{

/*****************************************************************************\
Function Prototypes
\*****************************************************************************/
inline int DirectoryCreate( const char * dirPath );
inline DWORD GetModuleFileName( char* pFileName, DWORD bufSize );

inline void* FileOpen( const char * fileName,
                      const char * mode );

inline void FileWrite( const void* pFile, const char * str, ... );
inline void FileClose( void* pFile );
inline DWORD FileRead( const void* pFile, void* pBuffer, DWORD bufSize );
inline DWORD FileSize( const void* pFile );
inline void LogToFile( const char* filename, const char * str, ... );
inline signed long ReadParamFromFile( char* filename, DWORD line );


/*****************************************************************************\
Inline Function:
    DirectoryCreate

Description:
    Creates a directory
\*****************************************************************************/
inline int DirectoryCreate( const char * dirPath )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
#   if defined _WIN32
    return _mkdir( dirPath );
#   endif
#   if defined (__GNUC__)
    return mkdir( dirPath, ( S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_IFDIR ) );
#   endif
#endif
}


/*****************************************************************************\
Inline Function:
    GetModuleFileName

Description:
    Gets file name of a runnimg application module
\*****************************************************************************/
inline DWORD GetModuleFileName( char* pFileName, DWORD bufSize )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently not implemented for kernel-mode
    return 0;
#elif defined(ISTDLIB_UMD)
    #if defined(_WIN32)
        return ::GetModuleFileNameA( NULL, pFileName, bufSize );
    #elif defined(__linux__)
        //TODO: add Linux implementation.
        return 0;
    #else
        // TO DO: replace with non-Windows version
        #error "TODO implement non-Windows equivalent of GetModuleFileName"
    #endif
#else
    // this compilation path is not intended
    // neither UMD nor KMD flag is set

   // this assert is commented out due to the number of solutions that do not define
   // ISTDLIB as KMD or UMD, it may be enabled in separate commit
   // C_ASSERT( 0 );

    return 0;
#endif
}


/*****************************************************************************\
Inline Function:
    OpenFile

Description:
    Opens a file based on the provided mode
\*****************************************************************************/
inline void* FileOpen( const char * fileName,
                      const char * mode )
{
    FILE * pFile;
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
    pFile = NULL;
#elif defined(ISTDLIB_UMD)
    pFile = fopen( fileName , mode );
#if defined(_WIN32)
    // Clear extra error information = ERROR_ALREADY_EXISTS set by windows when function success
    // It is necessary, cause it could be wrongly interpreted as fail of other function.
    if ( pFile &&  GetLastError() == ERROR_ALREADY_EXISTS )
    {
        SetLastError( 0 );
    }
#endif
#endif
    return pFile;
}


/*****************************************************************************\
Inline Function:
    CloseFile

Description:
    Close a file
\*****************************************************************************/
inline void FileClose( void* pFile )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( pFile )
    {
        fclose ( (FILE*)pFile );
    }
#endif
}


/*****************************************************************************\
Inline Function:
    WriteFile

Description:
    Writes the str to the provide file if it exists
\*****************************************************************************/
inline void FileWrite( const void* pFile, const char * str, ... )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( str != NULL )
    {
        va_list args;

        va_start( args, str );
        const size_t length = _vscprintf( str, args );
        va_end( args );

        char* temp = new char[ length + 1 ];

        if( temp )
        {
            va_start( args, str );
            VSNPRINTF( temp, length + 1, length+ 1, str, args );
            va_end( args );

            if( pFile )
            {
                fwrite( temp , 1 , length, (FILE*)pFile );
            }
            delete[] temp;
        }
    }
#endif
}


/*****************************************************************************\
Inline Function:
    WriteFile

Description:
    Overloaded function writes stream to the provide file if it exists
\*****************************************************************************/
inline void FileWrite( const void * str, size_t size , size_t count, const void* pFile )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( str != NULL )
    {
        if( pFile )
        {
            fwrite( str , size , count , (FILE*)pFile );
        }
    }
#endif
}


/*****************************************************************************\
Inline Function:
    FileRead

Description:
    Reads bufSize bytes from given file, and saves it to pBuffer. Returns
    number of read bytes - it may be diffrent than bufSize, if file is
    shorter than bufSize.
\*****************************************************************************/
inline DWORD FileRead( const void* pFile, void* pBuffer, DWORD bufSize )
{
    DWORD bytesRead = 0;
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( pFile )
    {
        bytesRead = (DWORD) fread( pBuffer, sizeof(BYTE), bufSize, (FILE*)pFile );
    }
#endif
    return bytesRead;
}


/*****************************************************************************\
Inline Function:
    FileSize

Description:
    Returns size in bytes of given file.
\*****************************************************************************/
inline DWORD FileSize( const void* pFile )
{
    DWORD fileSize = 0;
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( pFile )
    {
        long int currPosition = ftell( (FILE*)pFile );
        fseek( (FILE*)pFile, 0, SEEK_END );

        // Clamp result to 0, ftell can return -1 in case of an error.
        long int endPosition = ftell( (FILE*)pFile );
        fileSize = (DWORD)( endPosition >= 0 ? endPosition : 0 );

        fseek( (FILE*)pFile, currPosition, SEEK_SET );
    }
#endif
    return fileSize;
}


/*****************************************************************************\
Inline Function:
    LogToFile

Description:
    Writes the str to the file with provided filename, creates the file if it
    does not exists, for debug purposes only
\*****************************************************************************/
inline void LogToFile( const char* filename, const char * str, ... )
{
#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)
    if( str != NULL )
    {
        va_list args;

        va_start( args, str );
        const size_t length = _vscprintf( str, args );
        va_end( args );

        char* temp = new char[ length + 1 ];

        if( temp )
        {
            va_start( args, str );
            VSNPRINTF( temp, length + 1, length + 1, str, args );
            va_end( args );

            FILE* file = fopen( filename, "a" );
            if( file )
            {
                fwrite( temp , 1 , length, (FILE*)file );
                fclose( file );
            }
            delete[] temp;
        }
    }
#endif
}


/*****************************************************************************\
Inline Function:
    ReadParamFromFile

Description:
    reads n-th line from file specified as param, converts it (if possible)
    to signed long and return its value, if conversion is impossible or if
    file does not exists returns 0, line length in file cannot exceed
    bufsize (currently 20) characters, for debug purposes only
\*****************************************************************************/
inline signed long ReadParamFromFile( char* filename, DWORD line = 1 )
{
    signed long ret = 0;

#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD)

    FILE* file = fopen( filename, "r" );
    if( file )
    {
        const char bufsize = 20;
        char buf[ bufsize + 1 ] = "";

        // skip first n-1 lines
        while( --line && !feof( file ) )
        {
            fgets( buf, bufsize, file );
        }

        if( !feof( file ) )
        {
            iSTD::SafeMemSet( buf, 0, bufsize );
            fgets( buf, bufsize, file );
            ret = atoi( buf );
        }
        fclose( file );
    }
#endif
    return ret;
}


/*****************************************************************************\
Inline Function:
    CreateAppOutputDir

Description:
    Creates folder path based on outputDirectory string and input parameters.
    Then writes it in pathBuf.

Input:
    unsigned int    bufSize
    const char*     outputDirectory
    bool            addSystemDrive
    bool            addProcName
    bool            pidEnabled

Output:
    char*           pathBuf
    int             ret             - return value: 0 - success, -1 - fail

\*****************************************************************************/
inline int CreateAppOutputDir(
    char*           pathBuf,
    unsigned int    bufSize,
    const char*     outputDirectory,
    bool            addSystemDrive,
    bool            addProcName,
    bool            pidEnabled )
{
    int ret = 0;

#if defined(ISTDLIB_KMD)
    // TO DO: Currently only UMD is implemented
#elif defined(ISTDLIB_UMD) && defined(_WIN32)
    const size_t size = 1024;

    char outputDirPath[size] = {0};

    if( addSystemDrive )
    {
        // get system drive from environment variables
        char* envSystemDrive = getenv( "SystemDrive" );
        if( envSystemDrive )
        {
            STRCPY( outputDirPath, size, envSystemDrive );
        }
        STRCAT( outputDirPath, size, outputDirectory );
    }
    else
    {
        STRCPY( outputDirPath, size, outputDirectory );
    }

    //check whether latest mark is backslash and insert it if not
    size_t pathLen = strnlen_s( outputDirPath, size );
    if( ( pathLen < size ) && ( outputDirPath[pathLen-1] != '\\' ) )
    {
        STRCAT( outputDirPath, size, "\\" );
    }

    if( addProcName )
    {
        char appPath[512] = {0};

        // check a process id and make an adequate directory for it:
        if( GetModuleFileName( appPath, sizeof(appPath) -1 ) )
        {
            char* pprocessName = strrchr( appPath, '\\' );
            STRCAT( outputDirPath, size, ++pprocessName );
            STRCAT( outputDirPath, size, "\\" );
        }
        else
        {
            STRCAT( outputDirPath, size, "unknownProcess\\" );
        }

        if ( pidEnabled )
        {
            //append process id
            size_t pid = _getpid();
            size_t pathLen = strnlen_s( outputDirPath, size );
            if( ( pathLen < size ) && ( pathLen > 0 ) )
            {
                //check latest mark and add pid number before backslash
                if( outputDirPath[pathLen-1] == '\\' )
                {
                    --pathLen;
                }
                SPRINTF( outputDirPath + pathLen, size - pathLen, "_%Id\\", pid );
            }
        }
    }

    char currDirectory[size] = {0};
    int currDirIter = 0;
    const char* cPtr = outputDirPath;

    bool afterFirstSlash = false;
    errno_t err;

    // Create directories in path one by one
    while( *cPtr )
    {
        currDirectory[currDirIter++] = *cPtr;
        currDirectory[currDirIter] = 0;

        // If we found backslash, create directory
        // Omit first backslash - this is the root
        if( *cPtr == '\\' )
        {
            if( afterFirstSlash )
            {
                if( DirectoryCreate( currDirectory ) != 0 )
                {
                    _get_errno( &err );
                    if ( err != EEXIST )
                    {
                        ret = -1;
                        break;
                    }
                }
            }
            else
            {
                afterFirstSlash = true;
            }
        }

        cPtr++;
    }

    DWORD length = ( DWORD ) strlen( outputDirPath );
    length = ( bufSize-1 < length ) ? bufSize-1 : length;
    MemCopy( pathBuf, outputDirPath, length );
    pathBuf[length] = 0;

#elif !defined(_WIN32)
//if not Win, need to be Linux or Android.
//not _WIN32 is needed because not all projects define ISTDLIB_UMD
    char *path = (char*)malloc(sizeof(char)*bufSize);
    char *outputDirectoryCopy = (char*)malloc(sizeof(char)*bufSize);

    if (!path || !outputDirectoryCopy)
    {
        free(path);
        free(outputDirectoryCopy);
        return -1;
    }

    STRCPY(outputDirectoryCopy, bufSize, outputDirectory);
    if (addProcName)
    {

        const int defaultBuffSize = 1024; //max size for file from /proc/%d/cmdline
        char cmdpath[defaultBuffSize];
        char cmdline[defaultBuffSize] = "\0";
        char cmdlineoutput[defaultBuffSize] = "/";
        unsigned int pid = getpid();

        snprintf(cmdpath, defaultBuffSize - 1, "/proc/%u/cmdline", pid);
        FILE * fp = fopen(cmdpath, "r");
        if (fp)
        {
            if (fgets(cmdline, defaultBuffSize, fp) == NULL)
            {
                STRCPY(cmdline, defaultBuffSize, "unknownProcess");
            }
            fclose(fp);
        }

        if (strcmp(cmdline, "") == 0)
        {
            snprintf(cmdline, defaultBuffSize - 1, "unknownProcess");
        }
        else
        {
            for (int i = 0; i < defaultBuffSize - 1 && cmdline[i] != '\0'; i++)
            {
                // If $APPNAME launches another process this another process is named "$APPNAME:another_process".
                if (cmdline[i] == ':')
                {
                    cmdline[i] = '\0';
                    break;
                }
            }

            // checking sizes.
            size_t srcLen = strnlen( cmdline, defaultBuffSize );
            size_t dstLen = strnlen( cmdlineoutput, defaultBuffSize );
            size_t maxCopyLen = defaultBuffSize - 1 - dstLen;
            size_t copyLen = srcLen < maxCopyLen ? srcLen : maxCopyLen;

            // using safe macro.
            STRCPY(cmdlineoutput, copyLen, cmdline);
            char* pch = strrchr(cmdlineoutput, '/');
            unsigned int i = 0;
            if (pch != NULL)
            {
                pch++; //remove last occurrence of '/'
                for (; i < defaultBuffSize - (pch - cmdlineoutput) && pch[i] != '\0'; i++)
                {
                    cmdlineoutput[i] = pch[i];
                }
            }
            cmdlineoutput[i] = '\0';
        }

        size_t pathLen = strnlen(outputDirectoryCopy, bufSize);
        if (pathLen < bufSize)
        {
            //check latest mark and add pid number before backslash
            if (pathLen > 1 && outputDirectoryCopy[pathLen - 1] != '/')
            {
                outputDirectoryCopy[pathLen] = '/';
                pathLen++;
            }

            if (pidEnabled)
            {
                snprintf(outputDirectoryCopy + pathLen, bufSize - pathLen, "%s_%u/", cmdlineoutput, pid);
            }
            else
            {
                snprintf(outputDirectoryCopy + pathLen, bufSize - pathLen, "%s/", cmdlineoutput);
            }
        }
    }

    struct stat statbuf;
    unsigned int i = 0;

    while (i < bufSize - 1)
    {
        if ((outputDirectoryCopy[i] == '/' && i != 0 ) || outputDirectoryCopy[i] == '\0')
        {
            path[i] = '\0';
            if (stat(path, &statbuf) != 0) //direcory doesn't exist
            {
                //create dir
                if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) != 0)
                {
                    pathBuf[0] = '\0';
                    free(path);
                    free(outputDirectoryCopy);
                    return ret;
                }
            }
            else
            {
                if (!S_ISDIR(statbuf.st_mode)) //not dir
                {
                    pathBuf[0]= '\0';
                    free(path);
                    free(outputDirectoryCopy);
                    return ret;
                }
            }
            if (outputDirectoryCopy[i] == '\0')
            {
                break;
            }
        }
        path[i] = outputDirectoryCopy[i];
        i++;
    }

    if (i < bufSize-1) //fails if outputDirectory is longer than the bufSize
    {
        unsigned int length = (unsigned int)strlen(path);
        length = (bufSize-1 < length) ? bufSize-1 : length;
        memcpy(pathBuf, path, length);
        pathBuf[length] = 0;
    } else {
        pathBuf[0] = '\0';
    }

    free(path);
    free(outputDirectoryCopy);
#endif //#elif !defined(_WIN32)
    return ret;
}


/*****************************************************************************\
Inline Function:
    CreateAppOutputDir

Description:
    Creates folder path based on outputDirectory string.
    Then writes it in pathBuf.

Input:
    unsigned int    bufSize
    const char*     outputDirectory

Output:
    char*           pathBuf
    int             ret         - return value: 0 - success, -1 - fail

\*****************************************************************************/
inline int CreateAppOutputDir(
    char*           pathBuf,
    unsigned int    bufSize,
    const char*     outputDirectory )
{
    int ret = CreateAppOutputDir( pathBuf, bufSize, outputDirectory, true, true, false );

    return ret;
}


/*****************************************************************************\
Inline Function:
    ParentDirectoryCreate

Description:
    Creates a directory to hold the given file
\*****************************************************************************/
#if !defined(_WIN32) && !defined(_WIN64)
#include <limits.h>
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif
#endif
inline int ParentDirectoryCreate(const char * filePath)
{
    char pathBuf[MAX_PATH];
    char parentBuf[MAX_PATH];

#if defined _WIN32 || _WIN64
    char drive[MAX_PATH];
    char directory[MAX_PATH];

    if (_splitpath_s(
        filePath,
        drive,
        sizeof(drive),
        directory,
        sizeof(directory),
        NULL,
        0,
        NULL,
        0) != 0)
    {
        return -1;
    }

    _makepath(parentBuf, drive, directory, NULL, NULL);
#else
    // dirname may alter its parameter, so copy it to a buffer first.
    STRCPY(pathBuf, MAX_PATH, filePath);
    STRCPY(parentBuf, MAX_PATH, dirname(pathBuf));
#endif

return CreateAppOutputDir(pathBuf, sizeof(pathBuf), parentBuf, false, false, false);

}


} // iSTD

#if defined _WIN32
#   pragma warning(pop)
#endif

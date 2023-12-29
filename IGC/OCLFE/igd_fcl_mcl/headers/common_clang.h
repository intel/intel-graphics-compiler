/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if defined (_WIN32)
#if defined (COMMON_CLANG_EXPORTS)
#define CC_DLL_EXPORT _declspec(dllexport)
#else
#define CC_DLL_EXPORT _declspec(dllimport)
#endif
#else
#define CC_DLL_EXPORT
#endif

namespace Intel { namespace OpenCL { namespace ClangFE {
    //
    // Type of the binary returned after compilation and/or link
    //
    enum IR_TYPE
    {
        IR_TYPE_UNKNOWN,
        IR_TYPE_EXECUTABLE,
        IR_TYPE_LIBRARY,
        IR_TYPE_COMPILED_OBJECT
    };

    //
    // Compilation results interface
    // Returned by Compile method
    //
    struct IOCLFEBinaryResult
    {
        IOCLFEBinaryResult(const IOCLFEBinaryResult& cm) = delete;
        IOCLFEBinaryResult& operator=(const IOCLFEBinaryResult& cm) = delete;
        // Returns the size in bytes of the IR buffer
        virtual size_t      GetIRSize() const = 0;
        // Returns the pointer to the IR buffer or NULL if no IR buffer is present
        virtual const void* GetIR() const = 0;
        // Returns the name of the program
        virtual const char* GetIRName() const = 0;
        // Returns the type of the resulted binary
        virtual IR_TYPE GetIRType() const = 0;
        // Returns the pointer to the compilation log string or NULL if not log was created
        virtual const char* GetErrorLog() const = 0;
        // Releases the result object
        virtual void        Release() = 0;
        protected:
        virtual             ~IOCLFEBinaryResult(){}
    };
}}}

//
//Verifies the given OpenCL application supplied compilation options
//Params:
//    pszOptions - compilation options string
//    pszUnknownOptions - optional outbound pointer to the space separated unrecognized options
//    uiUnknownOptionsSize - size of the pszUnknownOptions buffer
//Returns:
//    true if the options verification was succesfull, false otherwise
//
extern "C" CC_DLL_EXPORT bool CheckCompileOptions(
    // A string for compile options
    const char*     pszOptions,
    // buffer to get the list of unknown options
    char* pszUnknownOptions,
    // size of the buffer for unknown options
    size_t uiUnknownOptionsSize
    );

//
//Verifies the given OpenCL application supplied link options
//Params:
//    pszOptions - compilation options string
//    pszUnknownOptions - optional outbound pointer to the space separated unrecognized options
//    uiUnknownOptionsSize - size of the pszUnknownOptions buffer
//Returns:
//    true if the options verification was succesfull, false otherwise
//
extern "C" CC_DLL_EXPORT bool CheckLinkOptions(
    // A string for compile options
    const char*     pszOptions,
    // buffer to get the list of unknown options
    char* pszUnknownOptions,
    // size of the buffer for unknown options
    size_t uiUnknownOptionsSize
    );

//
//Compiles the given OpenCL program to the LLVM IR
//Params:
//    pProgramSource - OpenCL source program to compile
//    pInputHeaders - array of the header buffers
//    uiNumInputHeader - size of the pInputHeaders array
//    pszInputHeadersNames - array of the headers names
//    pPCHBuffer - optional pointer to the pch buffer
//    uiPCHBufferSize - size of the pch buffer
//    pszOptions - OpenCL application supplied options
//    pszOptionsEx - optional extra options string usually supplied by runtime
//    pszDeviceSupportedExtentions - space separated list of the supported OpenCL extensions string
//    pszOpenCLVer - OpenCL version string - "120" for OpenCL 1.2, "200" for OpenCL 2.0, ...
//    pBinaryResult - optional outbound pointer to the compilation results
//Returns:
//    Compilation Result as int:  0 - success, error otherwise.
//
extern "C" CC_DLL_EXPORT int Compile(
    // A pointer to main program's source (null terminated string)
    const char*     pszProgramSource,
    // array of additional input headers to be passed in memory (each null terminated)
    const char**    pInputHeaders,
    // the number of input headers in pInputHeaders
    unsigned int    uiNumInputHeaders,
    // array of input headers names corresponding to pInputHeaders
    const char**    pInputHeadersNames,
    // optional pointer to the pch buffer
    const char*     pPCHBuffer,
    // size of the pch buffer
    size_t          uiPCHBufferSize,
    // OpenCL application supplied options
    const char*     pszOptions,
    // optional extra options string usually supplied by runtime
    const char*     pszOptionsEx,
    // OpenCL version string - "120" for OpenCL 1.2, "200" for OpenCL 2.0, ...
    const char*     pszOpenCLVer,
    // optional outbound pointer to the compilation results
    Intel::OpenCL::ClangFE::IOCLFEBinaryResult** pBinaryResult
    );

//
//Links the given OpenCL binaries
//Params:
//    pInputHeaders - array of the header buffers
//    uiNumInputHeader - size of the pInputHeaders array
//    pszInputHeadersNames - array of the headers names
//    pPCHBuffer - optional pointer to the pch buffer
//    uiPCHBufferSize - size of the pch buffer
//    pszOptions - OpenCL application supplied options
//    pszOptionsEx - optional extra options string usually supplied by runtime
//    pszDeviceSupportedExtentions - space separated list of the supported OpenCL extensions string
//    pszOpenCLVer - OpenCL version string - "120" for OpenCL 1.2, "200" for OpenCL 2.0, ...
//    pBinaryResult - optional outbound pointer to the compilation results
//Returns:
//    Link Result as int:  0 - success, error otherwise.
//
extern "C" CC_DLL_EXPORT int Link(
    // array of additional input headers to be passed in memory
    const void**    pInputBinaries,
    // the number of input binaries
    unsigned int    uiNumBinaries,
    // the size in bytes of each binary
    const size_t*   puiBinariesSizes,
    // OpenCL application supplied options
    const char*     pszOptions,
    // optional outbound pointer to the compilation results
    Intel::OpenCL::ClangFE::IOCLFEBinaryResult** pBinaryResult
    );

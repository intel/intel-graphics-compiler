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

struct VISA_GenVar;
struct VISA_AddrVar;
struct VISA_PredVar;
struct VISA_SamplerVar;
struct VISA_SurfaceVar;
struct VISA_VMEVar;
struct VISA_LabelVar;
struct VISA_FileVar;


struct VISA_PredOpnd;
struct VISA_RawOpnd;
struct VISA_VectorOpnd;
struct VISA_LabelOpnd;
struct VISA_StateOpndHandle;

#define CM_BUILDER_API

#include "JitterDataStruct.h"
#include "visa_igc_common_header.h"
#include "VISABuilderAPIDefinition.h"
#include "visa_wa.h"
#include "RelocationInfo.h"

extern "C" CM_BUILDER_API int CreateVISABuilder(VISABuilder* &builder, vISABuilderMode mode, CM_VISA_BUILDER_OPTION builderOption, TARGET_PLATFORM platform, int numArgs, const char* flags[], PVISA_WA_TABLE pWaTable);
extern "C" CM_BUILDER_API int DestroyVISABuilder(VISABuilder *&builder);

/**
 *
 *  Interface for CMRT to free the kernel binary allocated
 *  by the Jitter
 */
extern "C" CM_BUILDER_API void freeBlock(void* ptr);

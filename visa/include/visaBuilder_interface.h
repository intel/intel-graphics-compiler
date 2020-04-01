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

typedef struct _VISA_GenVar      VISA_GenVar;
typedef struct _VISA_AddrVar     VISA_AddrVar;
typedef struct _VISA_PredVar     VISA_PredVar;
typedef struct _VISA_SamplerVar  VISA_SamplerVar;
typedef struct _VISA_SurfaceVar  VISA_SurfaceVar;
typedef struct _VISA_VMEVar      VISA_VMEVar;
typedef struct _VISA_LabelVar    VISA_LabelVar;
typedef struct _VISA_FileVar     VISA_FileVar;


typedef struct _VISA_LabelOpnd        VISA_LabelOpnd;
typedef struct _VISA_VectorOpnd       VISA_VectorOpnd;
typedef struct _VISA_RawOpnd          VISA_RawOpnd;
typedef struct _VISA_PredOpnd         VISA_PredOpnd;
typedef struct _VISA_StateOpndHandle  VISA_StateOpndHandle;

#include "JitterDataStruct.h"
#include "visa_igc_common_header.h"
#include "VISABuilderAPIDefinition.h"
#include "visa_wa.h"
#include "RelocationInfo.h"
#include "inc/common/sku_wa.h"

extern "C" int CreateVISABuilder(VISABuilder* &builder, vISABuilderMode mode,
    VISA_BUILDER_OPTION builderOption, TARGET_PLATFORM platform, int numArgs, const char* flags[],
    PWA_TABLE pWaTable);
extern "C" int DestroyVISABuilder(VISABuilder *&builder);

/**
 *
 *  Interface to free the kernel binary allocated by the vISA finalizer
 */
extern "C" void freeBlock(void* ptr);

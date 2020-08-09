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

#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"
#include "VISADefines.h"
#include "inc/common/sku_wa.h"


extern "C"
VISA_BUILDER_API int CreateVISABuilder(VISABuilder* &builder, vISABuilderMode mode,
    VISA_BUILDER_OPTION builderOption, TARGET_PLATFORM platform, int numArgs,
    const char* flags[], const PWA_TABLE pWaTable)
{
    if (builder)
    {
        return VISA_FAILURE;
    }
    CISA_IR_Builder* cisa_builder = NULL;
    int status = CISA_IR_Builder::CreateBuilder(cisa_builder, mode, builderOption, platform,
        numArgs, flags, pWaTable);
    builder = static_cast<VISABuilder*>(cisa_builder);

    return status;
}

extern "C"
VISA_BUILDER_API int DestroyVISABuilder(VISABuilder *&builder)
{
    CISA_IR_Builder *cisa_builder = (CISA_IR_Builder *) builder;
    if (cisa_builder == NULL)
    {
        return VISA_FAILURE;
    }

    int status = CISA_IR_Builder::DestroyBuilder(cisa_builder);
    return status;
}

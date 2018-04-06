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
#include "AdaptorCommon/GTPinInterfaceUtils.hpp"

void* AllocGTPinDataBuffer(const IGC::SKernelProgram* program, const IGC::SProgramOutput* kernel, unsigned &bufferSize)
{
    if (!program || !kernel || !kernel->m_programBin)
    {
        bufferSize = 0;
        return nullptr;
    }

    const gtpin::igc::igc_init_t* gtpinRequest = &(program->m_GTPinRequest);
    gtpin::igc::igc_info_t gtpinReturnStatus;
    memset(&gtpinReturnStatus, 0, sizeof(gtpinReturnStatus));

    // Version check
    if (gtpinRequest->version != gtpin::igc::GTPIN_IGC_INTERFACE_VERSION)
    {
        bufferSize = 0;
        return nullptr;
    }
    // Check for zero input
    if (memcmp(gtpinRequest, &(gtpinReturnStatus.init_status), sizeof(gtpin::igc::igc_init_t)) == 0)
    {
        bufferSize = 0;
        return nullptr;
    }

    unsigned int allocSize = 0;
    unsigned int offset = 0;
    char* buffer = nullptr;
    bool hasFreeGRFInfo = gtpinRequest->grf_info > 0 && kernel->m_freeGRFInfoSize > 0;

    // Calculate allocation size
    {
        allocSize += sizeof(gtpin::igc::igc_info_t);            // init status
        if (hasFreeGRFInfo)
        {
            allocSize += sizeof(gtpin::igc::igc_token_header_t); // token header
            allocSize += kernel->m_freeGRFInfoSize;              // free grf data
        }

        // Allocate the buffer
        buffer = (char*)malloc(allocSize);
    }

    // Update the gtpin return status
    {
        gtpinReturnStatus.init_status.version = gtpin::igc::GTPIN_IGC_INTERFACE_VERSION;
        gtpinReturnStatus.init_status.re_ra = gtpinRequest->re_ra;

        if (hasFreeGRFInfo)
        {
            gtpinReturnStatus.num_tokens++;
            gtpinReturnStatus.init_status.grf_info = 1;
        }

        // copy the init status to buffer:
        memcpy_s(buffer, sizeof(gtpinReturnStatus), &gtpinReturnStatus, sizeof(gtpinReturnStatus));
        offset += sizeof(gtpinReturnStatus);
    }

    if (hasFreeGRFInfo)
    {
        // Set the token headers
        gtpin::igc::igc_token_header_t tokenHeader;
        tokenHeader.token = gtpin::igc::GTPIN_IGC_TOKEN_GRF_INFO;
        tokenHeader.token_size = sizeof(gtpin::igc::igc_token_header_t) + kernel->m_freeGRFInfoSize;

        // copy the free grf token header:
        memcpy_s(buffer+offset, sizeof(gtpin::igc::igc_token_header_t), &tokenHeader, sizeof(gtpin::igc::igc_token_header_t));
        offset += sizeof(gtpin::igc::igc_token_header_t);
        // copy the free grf data
        memcpy_s(buffer+offset, kernel->m_freeGRFInfoSize, kernel->m_freeGRFInfo, kernel->m_freeGRFInfoSize);
        offset += kernel->m_freeGRFInfoSize;
    }

    bufferSize = allocSize;
    return (void*) buffer;
}

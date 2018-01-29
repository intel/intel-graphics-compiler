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
#pragma once

#include <vector>
#include <stdint.h>
#include "usc.h"

namespace SIP
{
    enum EU_ARCHITECTURE_REGISTER_TYPE
    {
        EU_ARF_NULL                 = 0x00,
        EU_ARF_ADDRESS              = 0x10,
        EU_ARF_ACCUMULATOR          = 0x20,   
        EU_ARF_FLAG                 = 0x30,
        // New Gen7.5 CE register reusing code from (removed) mask register
        EU_ARF_CHANNEL_ENABLE       = 0x40,
        // New Gen8 MSG register reusing code from (removed) mask stack register
        EU_ARF_MESSAGE_CONTROL      = 0x50,    
        // New Gen6 SP register reusing code from (removed) mask stack depth
        EU_ARF_SP                   = 0x60,
        EU_ARF_STATE                = 0x70,
        EU_ARF_CONTROL              = 0x80,
        EU_ARF_NOTIFICATION_COUNT   = 0x90,
        EU_ARF_IP                   = 0xA0,
        EU_ARF_TDR                  = 0xB0,
        EU_ARF_TIMESTAMP            = 0xC0,
        EU_ARF_FLOW_CONTROL         = 0xD0,
        EU_ARF_DBG                  = 0xF0,
    };

    enum EU_REGISTER_TYPE
    {
        EU_REGISTER_TYPE_UD = 0,
        EU_REGISTER_TYPE_D  = 1,
        EU_REGISTER_TYPE_UW = 2,
        EU_REGISTER_TYPE_W  = 3,
        EU_REGISTER_TYPE_UB = 4,
        EU_REGISTER_TYPE_UV = 4,
        EU_REGISTER_TYPE_B  = 5,
        EU_REGISTER_TYPE_VF = 5,
        EU_REGISTER_TYPE_V  = 6,
        EU_REGISTER_TYPE_DF = 6,
        EU_REGISTER_TYPE_F  = 7,
        EU_REGISTER_TYPE_UQ = 8,
        EU_REGISTER_TYPE_Q  = 9,
        EU_REGISTER_TYPE_HF = 10,
        EU_REGISTER_TYPE_IMMEDIATE_DF = 10,
        EU_REGISTER_TYPE_IMMEDIATE_HF = 11,
        NUM_EU_REGISTER_TYPES
    };

    struct SArfRegData 
    {
        SIP::EU_ARCHITECTURE_REGISTER_TYPE arfRegFile;
        unsigned int arfRegNum;
        unsigned int arfSubRegNum;
        SIP::EU_REGISTER_TYPE arfRegType;
        unsigned int destRegOffset;
        unsigned int destSubRegOffset;
        unsigned int accessExecSize;
        unsigned int arfScratchSpaceOffset;
        bool arfDataReady;
    };

    /*****************************************************************************\
    CONST: cGen7DataCacheSizeInBytes
    Size of the data cache.
    The R/W portion of L3 data cache can be up to 128KB in IVB - programmable.
    \*****************************************************************************/
    const unsigned int cGen7DataCacheSizeInBytes = 0x20000; // 128KB

    /*****************************************************************************\
    CONST: cGen7SIPScratchSizeInBytes
    Per thread data size that will be dumped by the system thread.
    \*****************************************************************************/
    const unsigned int cGen7SIPDataSizeInBytes = 0x1140;

    /*****************************************************************************\
    CONST: cGen8SIPScratchSizeInHWords
    This constant is used For asserts and copying.
    For memory allocation cGen8SIPScratchSizeInBytes should be used.
    \*****************************************************************************/
    static const unsigned int cGen8SIPScratchSizeInHWords = 0x100;

    /*****************************************************************************\
    CONST: cGen8SIPScratchSizeInHWords
    This constant specifies the size of dumped memory for each thread.
    \*****************************************************************************/
    static const unsigned int cGen8SIPThreadScratchSize = 0x1800;


    /*****************************************************************************\
    CONST: cGpgpuArfSaveRegDataGen8
    This constant is a map of all arf regisiters stored during GPGPU mid-thread preemption
    \*****************************************************************************/
    static const SArfRegData cGpgpuArfSaveRegDataGen8[] =  
    {
        // regFile, regNum, subRegNum, regType, destRegisterOffset, destSubRegisterOffset, accessExecutionSize, arfScratchSpaceOffset, arfDataReady
        { SIP::EU_ARF_ADDRESS,        0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 0, 0 },
        { SIP::EU_ARF_FLAG,           0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 1, 0, 0 },
        { SIP::EU_ARF_FLAG,           1, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 1, 0, 0 },
        { SIP::EU_ARF_STATE,          1, 0, SIP::EU_REGISTER_TYPE_UD, 3, 4, 4, 0, 0 },
        { SIP::EU_ARF_STATE,          0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 4, 0, 0 },
        { SIP::EU_ARF_CONTROL,        0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 4, 4, 0, 1 },
        { SIP::EU_ARF_NOTIFICATION_COUNT,     0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 4, 4, 0 },
        { SIP::EU_ARF_TDR,            0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 4, 4, 4, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 4, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    1, 0, SIP::EU_REGISTER_TYPE_F,  3, 0, 8, 4, 0 },
        { SIP::EU_ARF_SP,             0, 0, SIP::EU_REGISTER_TYPE_UQ, 4, 0, 2, 4, 1 },
        { SIP::EU_ARF_ACCUMULATOR,    2, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 8, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    3, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 8, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    4, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 8, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    5, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 8, 1 },
        { SIP::EU_ARF_ACCUMULATOR,    6, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 12, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    7, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 12, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    8, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 12, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    9, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 12, 1 },    
        { SIP::EU_ARF_FLOW_CONTROL,   0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 1, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 2, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 3, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 16, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 4, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 20, 0 },  
        { SIP::EU_ARF_FLOW_CONTROL,   0, 5, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 20, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 6, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 20, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 7, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 20, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 8, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 9, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 10, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 11, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 24, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 12, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 13, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 14, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 15, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 28, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 16, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 17, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 18, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 19, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 32, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 20, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 21, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 22, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 23, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 36, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 24, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 25, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 26, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 27, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 40, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 28, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 29, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 30, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 31, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 44, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   1, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 48, 0 },  
        { SIP::EU_ARF_FLOW_CONTROL,   2, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 48, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 48, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 1, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 48, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 2, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 52, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 3, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 52, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   4, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 52, 1 },
        { SIP::EU_ARF_MESSAGE_CONTROL,0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 56, 0 },
        { SIP::EU_ARF_MESSAGE_CONTROL,1, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 56, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,2, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 56, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,3, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 56, 1 },
        { SIP::EU_ARF_MESSAGE_CONTROL,4, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 60, 0 },
        { SIP::EU_ARF_MESSAGE_CONTROL,5, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 60, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,6, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 60, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,7, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 60, 1 },    
    };

    /*****************************************************************************\
    CONST: cGpgpuArfRestoreRegDataGen8
    This constant is a map of all arf regisiters restored during GPGPU mid-thread preemption
    \*****************************************************************************/
    static const SArfRegData cGpgpuArfRestoreRegDataGen8[] =  
    {
        // regFile, regNum, subRegNum, regType, destRegisterOffset, destSubRegisterOffset, accessExecutionSize, arfScratchSpaceOffset, arfDataReady
        { SIP::EU_ARF_CONTROL,        0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 4, 4, 0, 0 },
        { SIP::EU_ARF_STATE,          0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 4, 0, 0 },
        { SIP::EU_ARF_STATE,          1, 0, SIP::EU_REGISTER_TYPE_UD, 3, 4, 4, 0, 0 },
        { SIP::EU_ARF_FLAG,           1, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 1, 0, 0 },
        { SIP::EU_ARF_FLAG,           0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 1, 0, 0 },
        { SIP::EU_ARF_ADDRESS,        0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 0, 1 },
        { SIP::EU_ARF_SP,             0, 0, SIP::EU_REGISTER_TYPE_UQ, 4, 0, 2, 4, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    1, 0, SIP::EU_REGISTER_TYPE_F,  3, 0, 8, 4, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 4, 0 },
        { SIP::EU_ARF_TDR,            0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 4, 4, 4, 0 },
        { SIP::EU_ARF_NOTIFICATION_COUNT,     0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 4, 4, 1 },
        { SIP::EU_ARF_ACCUMULATOR,    5, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 8, 0 },
        { SIP::EU_ARF_ACCUMULATOR,    4, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 8, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    3, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 8, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    2, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 8, 1 },
        { SIP::EU_ARF_ACCUMULATOR,    9, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 12, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    8, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 12, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    7, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 12, 0 },    
        { SIP::EU_ARF_ACCUMULATOR,    6, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 12, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 3, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 2, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 1, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 16, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 16, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 7, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 20, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 6, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 20, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 5, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 20, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 4, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 20, 1 },  
        { SIP::EU_ARF_FLOW_CONTROL,   0, 11, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 10, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 9, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 24, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 8, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 24, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 15, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 14, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 13, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 28, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 12, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 28, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 19, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 18, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 17, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 32, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 16, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 32, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 23, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 22, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 21, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 36, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 20, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 36, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 27, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 26, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 25, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 40, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 24, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 40, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 31, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 30, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 29, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 44, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   0, 28, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 44, 1 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 1, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 48, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 48, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   2, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 48, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   1, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 48, 1 },  
        { SIP::EU_ARF_FLOW_CONTROL,   4, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 52, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 3, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 52, 0 },
        { SIP::EU_ARF_FLOW_CONTROL,   3, 2, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 52, 1 },
        { SIP::EU_ARF_MESSAGE_CONTROL,3, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 56, 0 },
        { SIP::EU_ARF_MESSAGE_CONTROL,2, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 56, 0 },
        { SIP::EU_ARF_MESSAGE_CONTROL,1, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 56, 0 },
        { SIP::EU_ARF_MESSAGE_CONTROL,0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 56, 1 },
        { SIP::EU_ARF_MESSAGE_CONTROL,7, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 60, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,6, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 8, 60, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,5, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 60, 0 },    
        { SIP::EU_ARF_MESSAGE_CONTROL,4, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 60, 1 },
    };

    static_assert( ( sizeof( cGpgpuArfRestoreRegDataGen8 ) / sizeof( SArfRegData ) ) == ( sizeof( cGpgpuArfSaveRegDataGen8 ) / sizeof( SArfRegData ) ),
                   "Restore and Save arrays must be same length!" );

    /*****************************************************************************\
    CONST: cShaderDebugArfRegDataGen8
    This constant is a map of all arf regisiters stored/restored during shader debug
    \*****************************************************************************/
    const SArfRegData cShaderDebugArfRegDataGen8[] =  
    {
        // regFile, regNum, subRegNum, regType, destRegisterOffset, destSubRegisterOffset, accessExecutionSize, arfDebugSurfaceOffsetInBytes, arfDataReady

        { SIP::EU_ARF_ADDRESS,        0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 0x1000, 0 }, // 8 registers at 2 bytes each
        { SIP::EU_ARF_FLAG,           0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 1, 0x1000, 0 }, // 4 registers at 2 bytes each
        { SIP::EU_ARF_FLAG,           1, 0, SIP::EU_REGISTER_TYPE_UD, 2, 1, 1, 0x1000, 1 },
        { SIP::EU_ARF_STATE,          0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 4, 0x1040, 0 }, // 4 registers at 4 bytes each
        { SIP::EU_ARF_CONTROL,        0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 4, 0x1040, 0 }, // 3 registers at 4 bytes each
        { SIP::EU_ARF_IP,             0, 0, SIP::EU_REGISTER_TYPE_UD, 3, 0, 1, 0x1040, 0 }, // 1 register at 4 bytes 
        { SIP::EU_ARF_NOTIFICATION_COUNT,     0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 4, 0x1040, 1 }, // 3 registers at 4 bytes each
        { SIP::EU_ARF_TDR,            0, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 4, 0x10C0, 0 }, // 8 registers at 2 bytes each
        { SIP::EU_ARF_ACCUMULATOR,    0, 0, SIP::EU_REGISTER_TYPE_UD, 2, 0, 8, 0x10C0, 0 }, // 2 registers at 32 bytes each
        { SIP::EU_ARF_ACCUMULATOR,    1, 0, SIP::EU_REGISTER_TYPE_F,  3, 0, 8, 0x10C0, 0 }, 
        { SIP::EU_ARF_TIMESTAMP,      0, 0, SIP::EU_REGISTER_TYPE_UD, 4, 0, 8, 0x10C0, 1 }, // 1 registers at 4 bytes 
        { SIP::EU_ARF_ACCUMULATOR,    2, 0, SIP::EU_REGISTER_TYPE_UD, 1, 0, 8, 0x1140, 0 }, // 8 registers at 32 bytes each
        { SIP::EU_ARF_ACCUMULATOR,    3, 0, SIP::EU_REGISTER_TYPE_UD,  2, 0, 8, 0x1140, 0 }, 
        { SIP::EU_ARF_ACCUMULATOR,    4, 0, SIP::EU_REGISTER_TYPE_UD,  3, 0, 8, 0x1140, 0 }, 
        { SIP::EU_ARF_ACCUMULATOR,    5, 0, SIP::EU_REGISTER_TYPE_UD,  4, 0, 8, 0x1140, 1 }, 
        { SIP::EU_ARF_ACCUMULATOR,    6, 0, SIP::EU_REGISTER_TYPE_UD,  1, 0, 8, 0x1200, 0 }, 
        { SIP::EU_ARF_ACCUMULATOR,    7, 0, SIP::EU_REGISTER_TYPE_UD,  2, 0, 8, 0x1200, 0 }, 
        { SIP::EU_ARF_ACCUMULATOR,    8, 0, SIP::EU_REGISTER_TYPE_UD,  3, 0, 8, 0x1200, 0 }, 
        { SIP::EU_ARF_ACCUMULATOR,    9, 0, SIP::EU_REGISTER_TYPE_UD,  4, 0, 8, 0x1200, 1 }, 
        { SIP::EU_ARF_CHANNEL_ENABLE, 0, 0, SIP::EU_REGISTER_TYPE_UD,  1, 0, 1, 0x1280, 0 }, // 1 registers at 4 bytes 
        { SIP::EU_ARF_SP,             0, 0, SIP::EU_REGISTER_TYPE_UQ,  2, 0, 2, 0x1280, 1 },// 2 registers at 64 bytes each 
    };


}
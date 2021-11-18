/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "visa_igc_common_header.h"
#include "VISADefines.h"
#include "PreDefinedVars.h"
#include "common.h"

static const PreDefinedVarInfo preDefinedVarTable[static_cast<int>(PreDefinedVarsInternal::VAR_LAST)] =
{
    { PreDefinedVarsInternal::VAR_NULL,      ISA_TYPE_UD, 1, true,  false,  0, 1,   "%null" },
    { PreDefinedVarsInternal::X,             ISA_TYPE_UW, 1, true,  false,  4,  1,   "%thread_x" },
    { PreDefinedVarsInternal::Y,             ISA_TYPE_UW, 1, true,  false,  6,  1,   "%thread_y" },
    { PreDefinedVarsInternal::LOCAL_ID_X,    ISA_TYPE_UD, 2, false, true,   16, 1,   "%local_id_x" },
    { PreDefinedVarsInternal::LOCAL_ID_Y,    ISA_TYPE_UD, 2, false, true,   20, 1,   "%local_id_y" },
    { PreDefinedVarsInternal::LOCAL_SIZE_X,  ISA_TYPE_UD, 2, false, true,   0,  1,   "%local_size_x" },
    { PreDefinedVarsInternal::LOCAL_SIZE_Y,  ISA_TYPE_UD, 2, false, true,   4,  1,   "%local_size_y" },
    { PreDefinedVarsInternal::GROUP_ID_X,    ISA_TYPE_UD, 2, true,  true,   4,  1,   "%group_id_x" },
    { PreDefinedVarsInternal::GROUP_ID_Y,    ISA_TYPE_UD, 2, true,  true,   24, 1,   "%group_id_y" },
    { PreDefinedVarsInternal::GROUP_ID_Z,    ISA_TYPE_UD, 3, true,  true,   28, 1,   "%group_id_z" },
    { PreDefinedVarsInternal::GROUP_COUNT_X, ISA_TYPE_UD, 2, false, true,   8,  1,   "%group_count_x" },
    { PreDefinedVarsInternal::GROUP_COUNT_Y, ISA_TYPE_UD, 2, false, true,   12, 1,   "%group_count_y" },
    { PreDefinedVarsInternal::TSC,           ISA_TYPE_UD, 2, false, false,  0,  5,   "%tsc" },
    { PreDefinedVarsInternal::R0,            ISA_TYPE_UD, 2, true,  false,  0,  8,   "%r0" },
    { PreDefinedVarsInternal::ARG,           ISA_TYPE_UD, 3, false, false,  0,  256, "%arg" },
    { PreDefinedVarsInternal::RET,           ISA_TYPE_UD, 3, false, false,  0,  96,  "%retval" },
    { PreDefinedVarsInternal::FE_SP,         ISA_TYPE_UQ, 3, false, false,  0,  1,   "%sp" },
    { PreDefinedVarsInternal::FE_FP,         ISA_TYPE_UQ, 3, false, false,  0,  1,   "%fp" },
    { PreDefinedVarsInternal::HW_TID,        ISA_TYPE_UD, 3, false, false,  0,  1,   "%hw_id" },
    { PreDefinedVarsInternal::SR0,           ISA_TYPE_UD, 3, false, false,  0,  4,   "%sr0" },
    { PreDefinedVarsInternal::CR0,           ISA_TYPE_UD, 3, false, false,  0,  1,   "%cr0" },
    { PreDefinedVarsInternal::CE0,           ISA_TYPE_UD, 3, false, false,  0,  1,   "%ce0" },
    { PreDefinedVarsInternal::DBG,           ISA_TYPE_UD, 3, false, false,  0,  2,   "%dbg0" },
    { PreDefinedVarsInternal::COLOR,         ISA_TYPE_UW, 3, true,  false,  0,  1,   "%color" },
    { PreDefinedVarsInternal::IMPL_ARG_BUF_PTR, ISA_TYPE_UQ, 3, false, false,  0,  1,   "%impl_arg_buf_ptr" },
    { PreDefinedVarsInternal::LOCAL_ID_BUF_PTR, ISA_TYPE_UQ, 3, false, false,  0,  1,   "%local_id_buf_ptr" }
};

enum class PreDefinedVarsInternal_3_4
{
    VAR_NULL = 0,
    X = 1,
    Y = 2,
    GROUP_ID_X = 3,
    GROUP_ID_Y = 4,
    GROUP_ID_Z = 5,
    TSC = 6,
    R0 = 7,
    ARG = 8,
    RET = 9,
    FE_SP = 10,
    FE_FP = 11,
    HW_TID = 12,
    SR0 = 13,
    CR0 = 14,
    CE0 = 15,
    DBG = 16,
    COLOR = 17,
    IMPL_ARG_BUF_PTR = 18,
    LOCAL_ID_BUF_PTR = 19,
    VAR_LAST = LOCAL_ID_BUF_PTR
};

PreDefinedVarsInternal mapExternalToInternalPreDefVar(int id)
{
    PreDefinedVarsInternal newIndex = PreDefinedVarsInternal::VAR_LAST;

    if (id <= static_cast<int>(PreDefinedVarsInternal_3_4::VAR_LAST))
        {
            PreDefinedVarsInternal_3_4 internalIndex = static_cast<PreDefinedVarsInternal_3_4>(id);
            switch (internalIndex)
            {
            case PreDefinedVarsInternal_3_4::VAR_NULL:
                newIndex = PreDefinedVarsInternal::VAR_NULL;
                break;
            case PreDefinedVarsInternal_3_4::X:
                newIndex = PreDefinedVarsInternal::X;
                break;
            case PreDefinedVarsInternal_3_4::Y:
                newIndex = PreDefinedVarsInternal::Y;
                break;
            case PreDefinedVarsInternal_3_4::GROUP_ID_X:
                newIndex = PreDefinedVarsInternal::GROUP_ID_X;
                break;
            case PreDefinedVarsInternal_3_4::GROUP_ID_Y:
                newIndex = PreDefinedVarsInternal::GROUP_ID_Y;
                break;
            case PreDefinedVarsInternal_3_4::GROUP_ID_Z:
                newIndex = PreDefinedVarsInternal::GROUP_ID_Z;
                break;
            case PreDefinedVarsInternal_3_4::TSC:
                newIndex = PreDefinedVarsInternal::TSC;
                break;
            case PreDefinedVarsInternal_3_4::R0:
                newIndex = PreDefinedVarsInternal::R0;
                break;
            case PreDefinedVarsInternal_3_4::ARG:
                newIndex = PreDefinedVarsInternal::ARG;
                break;
            case PreDefinedVarsInternal_3_4::RET:
                newIndex = PreDefinedVarsInternal::RET;
                break;
            case PreDefinedVarsInternal_3_4::FE_SP:
                newIndex = PreDefinedVarsInternal::FE_SP;
                break;
            case PreDefinedVarsInternal_3_4::FE_FP:
                newIndex = PreDefinedVarsInternal::FE_FP;
                break;
            case PreDefinedVarsInternal_3_4::HW_TID:
                newIndex = PreDefinedVarsInternal::HW_TID;
                break;
            case PreDefinedVarsInternal_3_4::SR0:
                newIndex = PreDefinedVarsInternal::SR0;
                break;
            case PreDefinedVarsInternal_3_4::CR0:
                newIndex = PreDefinedVarsInternal::CR0;
                break;
            case PreDefinedVarsInternal_3_4::CE0:
                newIndex = PreDefinedVarsInternal::CE0;
                break;
            case PreDefinedVarsInternal_3_4::DBG:
                newIndex = PreDefinedVarsInternal::DBG;
                break;
            case PreDefinedVarsInternal_3_4::COLOR:
                newIndex = PreDefinedVarsInternal::COLOR;
                break;
            case PreDefinedVarsInternal_3_4::IMPL_ARG_BUF_PTR:
                newIndex = PreDefinedVarsInternal::IMPL_ARG_BUF_PTR;
                break;
            case PreDefinedVarsInternal_3_4::LOCAL_ID_BUF_PTR:
                newIndex = PreDefinedVarsInternal::LOCAL_ID_BUF_PTR;
                break;
            default:
                break;
            }
        }

    return newIndex;
}

VISA_Type getPredefinedVarType(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].type;
}

const char * getPredefinedVarString(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].str;
}

PreDefinedVarsInternal getPredefinedVarID(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].id;
}

bool isPredefinedVarInR0(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].isInR0;
}

bool predefinedVarNeedGRF(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].needsGRF;
}

uint16_t getPredefinedVarByteOffset(PreDefinedVarsInternal id)
{
    return preDefinedVarTable[(int)id].byteOffset;
}

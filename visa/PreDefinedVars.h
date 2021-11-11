/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _PREDEFINEDVARS_H_
#define _PREDEFINEDVARS_H_
#include <array>
#include <stdint.h>
enum class PreDefinedVarsInternal
{
    VAR_NULL             = 0,
    X                = 1,
    Y                = 2,
    LOCAL_ID_X       = 3,
    LOCAL_ID_Y       = 4,
    LOCAL_SIZE_X     = 5,
    LOCAL_SIZE_Y     = 6,
    GROUP_ID_X       = 7,
    GROUP_ID_Y       = 8,
    GROUP_ID_Z       = 9,
    GROUP_COUNT_X    = 10,
    GROUP_COUNT_Y    = 11,
    TSC              = 12,
    R0               = 13,
    ARG              = 14,
    RET              = 15,
    FE_SP            = 16,
    FE_FP            = 17,
    HW_TID           = 18,
    SR0              = 19,
    CR0              = 20,
    CE0              = 21,
    DBG              = 22,
    COLOR            = 23,
    IMPL_ARG_BUF_PTR = 24,
    LOCAL_ID_BUF_PTR = 25,
    VAR_LAST         = 26
};

const std::array<PreDefinedVarsInternal, (int)PreDefinedVarsInternal::VAR_LAST> allPreDefVars = {
    PreDefinedVarsInternal::VAR_NULL,
    PreDefinedVarsInternal::X,
    PreDefinedVarsInternal::Y,
    PreDefinedVarsInternal::LOCAL_ID_X,
    PreDefinedVarsInternal::LOCAL_ID_Y,
    PreDefinedVarsInternal::LOCAL_SIZE_X,
    PreDefinedVarsInternal::LOCAL_SIZE_Y,
    PreDefinedVarsInternal::GROUP_ID_X,
    PreDefinedVarsInternal::GROUP_ID_Y,
    PreDefinedVarsInternal::GROUP_ID_Z,
    PreDefinedVarsInternal::GROUP_COUNT_X,
    PreDefinedVarsInternal::GROUP_COUNT_Y,
    PreDefinedVarsInternal::TSC,
    PreDefinedVarsInternal::R0,
    PreDefinedVarsInternal::ARG,
    PreDefinedVarsInternal::RET,
    PreDefinedVarsInternal::FE_SP,
    PreDefinedVarsInternal::FE_FP,
    PreDefinedVarsInternal::HW_TID,
    PreDefinedVarsInternal::SR0,
    PreDefinedVarsInternal::CR0,
    PreDefinedVarsInternal::CE0,
    PreDefinedVarsInternal::DBG,
    PreDefinedVarsInternal::COLOR,
    PreDefinedVarsInternal::IMPL_ARG_BUF_PTR,
    PreDefinedVarsInternal::LOCAL_ID_BUF_PTR,
};

typedef struct
{
    PreDefinedVarsInternal id;
    VISA_Type type;
    uint8_t majorVersion; // CISA major version when this becomes available
    bool isInR0;  // whether the variable value is stored in r0 or is
    bool needsGRF;
    // appended after kernel input
    uint16_t byteOffset;  // byte offset of the variable's value
    uint32_t num_elements;
    const char* str;
} PreDefinedVarInfo;

extern PreDefinedVarsInternal mapExternalToInternalPreDefVar(int id);

extern VISA_Type getPredefinedVarType(PreDefinedVarsInternal id);
extern const char * getPredefinedVarString(PreDefinedVarsInternal id);
extern PreDefinedVarsInternal getPredefinedVarID(PreDefinedVarsInternal id);
extern bool isPredefinedVarInR0(PreDefinedVarsInternal id);
extern bool predefinedVarNeedGRF(PreDefinedVarsInternal id);
extern uint16_t getPredefinedVarByteOffset(PreDefinedVarsInternal id);
#endif

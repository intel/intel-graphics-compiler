/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

extern int __SubDeviceID = 0;
extern int __MaxHWThreadIDPerSubDevice = 1;

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )(void) {
    return __SubDeviceID;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInHWThreadIDINTEL, , )(void) {
    return __builtin_IB_hw_thread_id() + __SubDeviceID * __MaxHWThreadIDPerSubDevice;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInMaxHWThreadIDPerSubDeviceINTEL, , )(void) {
    return __MaxHWThreadIDPerSubDevice;
}

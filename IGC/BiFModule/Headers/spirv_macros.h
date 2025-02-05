/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_MACROS_H__
#define __SPIRV_MACROS_H__

#define SPIRV_OVERLOADABLE __attribute__((overloadable))
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_BUILTIN_NO_OP(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_OCL_BUILTIN(func, old_mangling, new_mangling) \
    __spirv_ocl_##func##new_mangling

#endif // __SPIRV_MACROS_H__
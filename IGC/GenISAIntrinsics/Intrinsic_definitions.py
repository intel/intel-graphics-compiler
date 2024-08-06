# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

############# Currently Supported Types ######################
#PointerTypes = ["ptr_private","ptr_global","ptr_constant","ptr_local","ptr_generic"]
#FloatingPointTypes = ["half","float","double"]
#IntegerTypes = ["bool","char","short","int","long"]
#IntrinsicsProperties = ["None","NoMem","ReadArgMem","WriteArgMem","ReadMem","ReadWriteArgMem",
#                        "WriteMem", "NoReturn","NoDuplicate", "Convergent"]
#IntrinsicsProperties may be specified as a comma separated list (e.g., "Convergent,NoMem")

# EX
# "GenISA_blah": ["Description goes here",
#     [(return_type,                        "comment on return value"),
#     [(arg_type,                           "comment on arg value")],
#     Property]],

#The "any" type can be followed by a default type if a type is not
#explicitly specified: Ex. "any:int"

# 0 - LLVMMatchType<0>
# 1 - LLVMMatchType<1>
# {int} - LLVMMatchType<{int}>
# See Intrinsics.json file for entries

#### Update helper functions in Source/IGC/Compiler/CISACodeGen/helper.h when adding a new intrinsic
#    if required.
#    For example: Update IsStatelessMemLoadIntrinsic, IsStatelessMemStoreIntrinsic and
#    IsStatelessMemAtomicIntrinsic if it is a stateless memory read/write intrinsic.

Imported_Intrinsics = \
{
}

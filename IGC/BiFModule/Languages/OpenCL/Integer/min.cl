/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

GENERATE_VECTOR_FUNCTIONS_2ARGS( min, char, char )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, uchar, uchar )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, short, short )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, ushort, ushort )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, int, int )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, uint, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, long, long )
GENERATE_VECTOR_FUNCTIONS_2ARGS( min, ulong, ulong )

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, char, char, char )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, uchar, uchar, uchar )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, short, short, short )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, ushort, ushort, ushort )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, int, int, int )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, uint, uint, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, long, long, long )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( min, ulong, ulong, ulong )

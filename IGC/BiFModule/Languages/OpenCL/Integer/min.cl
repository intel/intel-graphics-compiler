/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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

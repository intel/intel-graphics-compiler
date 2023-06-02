# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import sys
from itertools import product

fdiv = {
    'size': [0] + [2 ** x for x in range(4)],
    'alg': ['ieee', 'fast'],
    'nan': ['_nnan', '_'],
    'inf': ['_ninf', '_'],
    'sz': ['_nsz', '_'],
}

arg_num = len(sys.argv)
if arg_num > 1:
    output = sys.argv[1]
    f = open(output, 'w')
    sys.stdout = f

for size, alg, nan, inf, sz in product(fdiv['size'], fdiv['alg'], fdiv['nan'],
                                       fdiv['inf'], fdiv['sz']):
    if size == 0:
        print(f"__IMPL_FDIV_SCALAR({alg}, {nan}, {inf}, {sz})")
    else:
        print(f"__IMPL_FDIV_VECTOR({size}, {alg}, {nan}, {inf}, {sz})")

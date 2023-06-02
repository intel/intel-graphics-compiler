# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import sys
from itertools import product

SignedDivSmall = {
    'types' : [
        { 'src_t' : 'int8_t', 'rem_t' : 'int8_t'},
        { 'src_t' : 'int16_t', 'rem_t' : 'int16_t'},
        { 'src_t' : 'int64_t', 'rem_t' : 'uint64_t'}
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rte'],
    'items' : ['__IREM', '__IDIV']
}
SignedDiv32 = {
    'types' : [
        { 'src_t' : 'int32_t', 'rem_t' : 'uint32_t'},
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rtz'],
    'items' : ['__IREM', '__IDIV']
}
UnsignedDiv = {
    'types' : [
        { 'src_t' : 'uint8_t', 'rem_t' : 'uint8_t'},
        { 'src_t' : 'uint16_t', 'rem_t' : 'uint16_t'},
        { 'src_t' : 'uint32_t', 'rem_t' : 'uint32_t'},
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rtz'],
    'items' : ['__UREM', '__UDIV']
}
Unsigned64Div = {
    'types' : [
        { 'src_t' : 'uint64_t', 'rem_t' : 'uint64_t'}
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rte'],
    'items' : ['__UREM', '__UDIV']
}

arg_num = len(sys.argv)
if arg_num > 1:
  output = sys.argv[1]
  f = open(output, 'w')
  sys.stdout = f

for desc in [SignedDivSmall, SignedDiv32, UnsignedDiv, Unsigned64Div]:
  for fname, size, rounding, types in product(desc['items'], desc['size'],
                                              desc['rounding'], desc['types']):
    if size == 0:
      print("{}_SCALAR_IMPL({}, {}, {}, {})".format(
        fname, types['src_t'], types['rem_t'], 1, rounding))
    else:
      print("{}_VECTOR_IMPL({}, {}, {}, {})".format(
        fname, types['src_t'], types['rem_t'], size, rounding))

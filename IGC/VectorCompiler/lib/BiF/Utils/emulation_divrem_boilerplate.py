#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

import sys
from itertools import product

SignedDivSmall = {
    'types' : [
        { 'src1' : 'int8_t', 'src2' : 'int8_t'},
        { 'src1' : 'int16_t', 'src2' : 'int16_t'}
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rte'],
    'items' : ['__IREM', '__IDIV']
}
SignedDiv32 = {
    'types' : [
        { 'src1' : 'int32_t', 'src2' : 'uint32_t'},
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rtz'],
    'items' : ['__IREM', '__IDIV']
}
UnsignedDiv = {
    'types' : [
        { 'src1' : 'uint8_t', 'src2' : 'uint8_t'},
        { 'src1' : 'uint16_t', 'src2' : 'uint16_t'},
        { 'src1' : 'uint32_t', 'src2' : 'uint32_t'},
    ],
    'size' : [0] + [2 ** x for x in range(6)],
    'rounding' : ['rtz'],
    'items' : ['__UREM', '__UDIV']
}

arg_num = len(sys.argv)
if arg_num > 1:
  output = sys.argv[1]
  f = open(output, 'w')
  sys.stdout = f

for desc in [SignedDivSmall, SignedDiv32, UnsignedDiv]:
  for fname, size, rounding, types in product(desc['items'], desc['size'],
                                              desc['rounding'], desc['types']):
    if size == 0:
      print("{}_SCALAR_IMPL({}, {}, {}, {})".format(
        fname, types['src1'], types['src2'], 1, rounding))
    else:
      print("{}_VECTOR_IMPL({}, {}, {}, {})".format(
        fname, types['src1'], types['src2'], size, rounding))

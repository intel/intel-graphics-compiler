/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

INLINE uchar2 __builtin_spirv_OpenCL_shuffle_v2i8_v2i8(uchar2 v, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle_v4i8_v2i8(uchar4 v, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle_v8i8_v2i8(uchar8 v, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle_v16i8_v2i8(uchar16 v, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle_v2i8_v4i8(uchar2 v, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle_v4i8_v4i8(uchar4 v, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle_v8i8_v4i8(uchar8 v, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle_v16i8_v4i8(uchar16 v, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle_v2i8_v8i8(uchar2 v, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle_v4i8_v8i8(uchar4 v, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle_v8i8_v8i8(uchar8 v, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle_v16i8_v8i8(uchar16 v, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle_v2i8_v16i8(uchar2 v, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle_v4i8_v16i8(uchar4 v, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle_v8i8_v16i8(uchar8 v, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle_v16i8_v16i8(uchar16 v, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}


INLINE ushort2 __builtin_spirv_OpenCL_shuffle_v2i16_v2i16(ushort2 v, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle_v4i16_v2i16(ushort4 v, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle_v8i16_v2i16(ushort8 v, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle_v16i16_v2i16(ushort16 v, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle_v2i16_v4i16(ushort2 v, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle_v4i16_v4i16(ushort4 v, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle_v8i16_v4i16(ushort8 v, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle_v16i16_v4i16(ushort16 v, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle_v2i16_v8i16(ushort2 v, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle_v4i16_v8i16(ushort4 v, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle_v8i16_v8i16(ushort8 v, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle_v16i16_v8i16(ushort16 v, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle_v2i16_v16i16(ushort2 v, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle_v4i16_v16i16(ushort4 v, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle_v8i16_v16i16(ushort8 v, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle_v16i16_v16i16(ushort16 v, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle_v2i32_v2i32(uint2 v, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle_v4i32_v2i32(uint4 v, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle_v8i32_v2i32(uint8 v, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle_v16i32_v2i32(uint16 v, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle_v2i32_v4i32(uint2 v, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle_v4i32_v4i32(uint4 v, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle_v8i32_v4i32(uint8 v, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle_v16i32_v4i32(uint16 v, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle_v2i32_v8i32(uint2 v, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle_v4i32_v8i32(uint4 v, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle_v8i32_v8i32(uint8 v, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle_v16i32_v8i32(uint16 v, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle_v2i32_v16i32(uint2 v, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle_v4i32_v16i32(uint4 v, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle_v8i32_v16i32(uint8 v, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle_v16i32_v16i32(uint16 v, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle_v2i64_v2i64(ulong2 v, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle_v4i64_v2i64(ulong4 v, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle_v8i64_v2i64(ulong8 v, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle_v16i64_v2i64(ulong16 v, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle_v2i64_v4i64(ulong2 v, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle_v4i64_v4i64(ulong4 v, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle_v8i64_v4i64(ulong8 v, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle_v16i64_v4i64(ulong16 v, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle_v2i64_v8i64(ulong2 v, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle_v4i64_v8i64(ulong4 v, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle_v8i64_v8i64(ulong8 v, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle_v16i64_v8i64(ulong16 v, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle_v2i64_v16i64(ulong2 v, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle_v4i64_v16i64(ulong4 v, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle_v8i64_v16i64(ulong8 v, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle_v16i64_v16i64(ulong16 v, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle_v2f32_v2i32(float2 v, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle_v4f32_v2i32(float4 v, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle_v8f32_v2i32(float8 v, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle_v16f32_v2i32(float16 v, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle_v2f32_v4i32(float2 v, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle_v4f32_v4i32(float4 v, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle_v8f32_v4i32(float8 v, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle_v16f32_v4i32(float16 v, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle_v2f32_v8i32(float2 v, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle_v4f32_v8i32(float4 v, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle_v8f32_v8i32(float8 v, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle_v16f32_v8i32(float16 v, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle_v2f32_v16i32(float2 v, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle_v4f32_v16i32(float4 v, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle_v8f32_v16i32(float8 v, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle_v16f32_v16i32(float16 v, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v2i8(uchar2 v0, uchar2 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v2i8(uchar4 v0, uchar4 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v2i8(uchar8 v0, uchar8 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE uchar2 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v2i8(uchar16 v0, uchar16 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v4i8(uchar2 v0, uchar2 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v4i8(uchar4 v0, uchar4 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v4i8(uchar8 v0, uchar8 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE uchar4 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v4i8(uchar16 v0, uchar16 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v8i8(uchar2 v0, uchar2 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v8i8(uchar4 v0, uchar4 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v8i8(uchar8 v0, uchar8 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE uchar8 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v8i8(uchar16 v0, uchar16 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v16i8(uchar2 v0, uchar2 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v16i8(uchar4 v0, uchar4 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v16i8(uchar8 v0, uchar8 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE uchar16 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v16i8(uchar16 v0, uchar16 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v2i16(ushort2 v0, ushort2 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v2i16(ushort4 v0, ushort4 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v2i16(ushort8 v0, ushort8 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE ushort2 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v2i16(ushort16 v0, ushort16 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v4i16(ushort2 v0, ushort2 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v4i16(ushort4 v0, ushort4 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v4i16(ushort8 v0, ushort8 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE ushort4 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v4i16(ushort16 v0, ushort16 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v8i16(ushort2 v0, ushort2 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v8i16(ushort4 v0, ushort4 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v8i16(ushort8 v0, ushort8 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE ushort8 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v8i16(ushort16 v0, ushort16 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v16i16(ushort2 v0, ushort2 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v16i16(ushort4 v0, ushort4 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v16i16(ushort8 v0, ushort8 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE ushort16 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v16i16(ushort16 v0, ushort16 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v2i32(uint2 v0, uint2 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v2i32(uint4 v0, uint4 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v2i32(uint8 v0, uint8 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE uint2 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v2i32(uint16 v0, uint16 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v4i32(uint2 v0, uint2 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v4i32(uint4 v0, uint4 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v4i32(uint8 v0, uint8 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE uint4 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v4i32(uint16 v0, uint16 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v8i32(uint2 v0, uint2 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v8i32(uint4 v0, uint4 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v8i32(uint8 v0, uint8 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE uint8 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v8i32(uint16 v0, uint16 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v16i32(uint2 v0, uint2 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v16i32(uint4 v0, uint4 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v16i32(uint8 v0, uint8 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE uint16 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v16i32(uint16 v0, uint16 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v2i64(ulong2 v0, ulong2 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v2i64(ulong4 v0, ulong4 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v2i64(ulong8 v0, ulong8 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE ulong2 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v2i64(ulong16 v0, ulong16 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v4i64(ulong2 v0, ulong2 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v4i64(ulong4 v0, ulong4 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v4i64(ulong8 v0, ulong8 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE ulong4 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v4i64(ulong16 v0, ulong16 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v8i64(ulong2 v0, ulong2 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v8i64(ulong4 v0, ulong4 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v8i64(ulong8 v0, ulong8 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE ulong8 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v8i64(ulong16 v0, ulong16 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v16i64(ulong2 v0, ulong2 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v16i64(ulong4 v0, ulong4 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v16i64(ulong8 v0, ulong8 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE ulong16 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v16i64(ulong16 v0, ulong16 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v2i32(float2 v0, float2 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v2i32(float4 v0, float4 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v2i32(float8 v0, float8 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE float2 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v2i32(float16 v0, float16 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v4i32(float2 v0, float2 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v4i32(float4 v0, float4 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v4i32(float8 v0, float8 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE float4 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v4i32(float16 v0, float16 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v8i32(float2 v0, float2 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v8i32(float4 v0, float4 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v8i32(float8 v0, float8 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE float8 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v8i32(float16 v0, float16 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v16i32(float2 v0, float2 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v16i32(float4 v0, float4 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v16i32(float8 v0, float8 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE float16 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v16i32(float16 v0, float16 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#if defined(cl_khr_fp16)

/// Half Shuffle functions
INLINE half2 __builtin_spirv_OpenCL_shuffle_v2f16_v2i16(half2 v, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle_v4f16_v2i16(half4 v, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle_v8f16_v2i16(half8 v, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle_v16f16_v2i16(half16 v, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle_v2f16_v4i16(half2 v, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle_v4f16_v4i16(half4 v, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle_v8f16_v4i16(half8 v, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle_v16f16_v4i16(half16 v, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle_v2f16_v8i16(half2 v, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle_v4f16_v8i16(half4 v, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle_v8f16_v8i16(half8 v, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle_v16f16_v8i16(half16 v, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle_v2f16_v16i16(half2 v, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle_v4f16_v16i16(half4 v, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle_v8f16_v16i16(half8 v, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle_v16f16_v16i16(half16 v, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

// Shuffle2
INLINE half2 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v2i16(half2 v0, half2 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v2i16(half4 v0, half4 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v2i16(half8 v0, half8 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE half2 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v2i16(half16 v0, half16 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v4i16(half2 v0, half2 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v4i16(half4 v0, half4 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v4i16(half8 v0, half8 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE half4 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v4i16(half16 v0, half16 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v8i16(half2 v0, half2 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v8i16(half4 v0, half4 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v8i16(half8 v0, half8 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE half8 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v8i16(half16 v0, half16 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v16i16(half2 v0, half2 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v16i16(half4 v0, half4 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v16i16(half8 v0, half8 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE half16 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v16i16(half16 v0, half16 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

/// Double Shuffle functions
INLINE double2 __builtin_spirv_OpenCL_shuffle_v2f64_v2i64(double2 v, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle_v4f64_v2i64(double4 v, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle_v8f64_v2i64(double8 v, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle_v16f64_v2i64(double16 v, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle_v2f64_v4i64(double2 v, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle_v4f64_v4i64(double4 v, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle_v8f64_v4i64(double8 v, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle_v16f64_v4i64(double16 v, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle_v2f64_v8i64(double2 v, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle_v4f64_v8i64(double4 v, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle_v8f64_v8i64(double8 v, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle_v16f64_v8i64(double16 v, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle_v2f64_v16i64(double2 v, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x1) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1) == 0x1) ? v.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x1) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1) == 0x1) ? v.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x1) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1) == 0x1) ? v.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x1) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1) == 0x1) ? v.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x1) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1) == 0x1) ? v.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x1) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1) == 0x1) ? v.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x1) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1) == 0x1) ? v.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x1) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1) == 0x1) ? v.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x1) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1) == 0x1) ? v.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x1) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1) == 0x1) ? v.s1 : ret.s9;

  ret.sa = ((m.sa & 0x1) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1) == 0x1) ? v.s1 : ret.sa;

  ret.sb = ((m.sb & 0x1) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1) == 0x1) ? v.s1 : ret.sb;

  ret.sc = ((m.sc & 0x1) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1) == 0x1) ? v.s1 : ret.sc;

  ret.sd = ((m.sd & 0x1) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1) == 0x1) ? v.s1 : ret.sd;

  ret.se = ((m.se & 0x1) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x1) == 0x1) ? v.s1 : ret.se;

  ret.sf = ((m.sf & 0x1) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1) == 0x1) ? v.s1 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle_v4f64_v16i64(double4 v, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v.s3 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v.s3 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v.s3 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v.s3 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v.s3 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v.s3 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v.s3 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle_v8f64_v16i64(double8 v, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v.s7 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v.s7 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v.s7 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v.s7 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v.s7 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v.s7 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v.s7 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle_v16f64_v16i64(double16 v, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v.sa : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v.sb : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v.sc : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v.sd : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v.se : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v.sf : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v.sa : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v.sb : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v.sc : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v.sd : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v.se : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v.sf : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v.sa : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v.sb : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v.sc : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v.sd : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v.se : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v.sf : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v.sa : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v.sb : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v.sc : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v.sd : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v.se : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v.sf : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v.sa : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v.sb : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v.sc : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v.sd : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v.se : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v.sf : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v.sa : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v.sb : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v.sc : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v.sd : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v.se : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v.sf : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v.sa : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v.sb : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v.sc : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v.sd : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v.se : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v.sf : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v.sa : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v.sb : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v.sc : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v.sd : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v.se : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v.sf : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v.sa : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v.sb : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v.sc : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v.sd : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v.se : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v.sf : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v.sa : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v.sb : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v.sc : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v.sd : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v.se : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v.sf : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v.s8 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v.s9 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v.sa : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v.sb : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v.sc : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v.sd : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v.se : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v.sf : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v.s8 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v.s9 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v.sa : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v.sb : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v.sc : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v.sd : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v.se : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v.sf : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v.s8 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v.s9 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v.sa : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v.sb : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v.sc : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v.sd : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v.se : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v.sf : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v.s8 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v.s9 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v.sa : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v.sb : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v.sc : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v.sd : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v.se : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v.sf : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v.s8 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v.s9 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v.sa : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v.sb : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v.sc : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v.sd : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v.se : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v.sf : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v.s8 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v.s9 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v.sa : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v.sb : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v.sc : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v.sd : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v.se : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v.sf : ret.sf;

  return ret;
}

// Shuffle2
INLINE double2 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v2i64(double2 v0, double2 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v2i64(double4 v0, double4 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v2i64(double8 v0, double8 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE double2 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v2i64(double16 v0, double16 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v4i64(double2 v0, double2 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v4i64(double4 v0, double4 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v4i64(double8 v0, double8 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE double4 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v4i64(double16 v0, double16 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v8i64(double2 v0, double2 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v8i64(double4 v0, double4 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v8i64(double8 v0, double8 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE double8 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v8i64(double16 v0, double16 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v16i64(double2 v0, double2 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x3) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x3) == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = ((m.s1 & 0x3) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x3) == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = ((m.s2 & 0x3) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x3) == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = ((m.s3 & 0x3) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x3) == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = ((m.s4 & 0x3) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x3) == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = ((m.s5 & 0x3) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x3) == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = ((m.s6 & 0x3) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x3) == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = ((m.s7 & 0x3) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x3) == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = ((m.s8 & 0x3) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x3) == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = ((m.s9 & 0x3) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x3) == 0x3) ? v1.s1 : ret.s9;

  ret.sa = ((m.sa & 0x3) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x2) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x3) == 0x3) ? v1.s1 : ret.sa;

  ret.sb = ((m.sb & 0x3) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x2) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x3) == 0x3) ? v1.s1 : ret.sb;

  ret.sc = ((m.sc & 0x3) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x2) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x3) == 0x3) ? v1.s1 : ret.sc;

  ret.sd = ((m.sd & 0x3) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x2) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x3) == 0x3) ? v1.s1 : ret.sd;

  ret.se = ((m.se & 0x3) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x3) == 0x2) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x3) == 0x3) ? v1.s1 : ret.se;

  ret.sf = ((m.sf & 0x3) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x2) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x3) == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v16i64(double4 v0, double4 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x7) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x7) == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = ((m.s1 & 0x7) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x7) == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = ((m.s2 & 0x7) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x7) == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = ((m.s3 & 0x7) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x7) == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = ((m.s4 & 0x7) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x7) == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = ((m.s5 & 0x7) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x7) == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = ((m.s6 & 0x7) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x7) == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = ((m.s7 & 0x7) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x7) == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = ((m.s8 & 0x7) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x7) == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = ((m.s9 & 0x7) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x7) == 0x7) ? v1.s3 : ret.s9;

  ret.sa = ((m.sa & 0x7) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x4) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x5) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x6) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x7) == 0x7) ? v1.s3 : ret.sa;

  ret.sb = ((m.sb & 0x7) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x4) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x5) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x6) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x7) == 0x7) ? v1.s3 : ret.sb;

  ret.sc = ((m.sc & 0x7) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x4) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x5) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x6) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x7) == 0x7) ? v1.s3 : ret.sc;

  ret.sd = ((m.sd & 0x7) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x4) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x5) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x6) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x7) == 0x7) ? v1.s3 : ret.sd;

  ret.se = ((m.se & 0x7) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x7) == 0x4) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x7) == 0x5) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x7) == 0x6) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x7) == 0x7) ? v1.s3 : ret.se;

  ret.sf = ((m.sf & 0x7) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x4) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x5) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x6) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x7) == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v16i64(double8 v0, double8 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0xf) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0xf) == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = ((m.s1 & 0xf) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0xf) == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = ((m.s2 & 0xf) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0xf) == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = ((m.s3 & 0xf) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0xf) == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = ((m.s4 & 0xf) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0xf) == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = ((m.s5 & 0xf) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0xf) == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = ((m.s6 & 0xf) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0xf) == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = ((m.s7 & 0xf) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0xf) == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = ((m.s8 & 0xf) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0xf) == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = ((m.s9 & 0xf) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0xf) == 0xf) ? v1.s7 : ret.s9;

  ret.sa = ((m.sa & 0xf) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x8) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0x9) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xa) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xb) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xc) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xd) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xe) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0xf) == 0xf) ? v1.s7 : ret.sa;

  ret.sb = ((m.sb & 0xf) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x8) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0x9) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xa) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xb) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xc) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xd) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xe) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0xf) == 0xf) ? v1.s7 : ret.sb;

  ret.sc = ((m.sc & 0xf) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x8) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0x9) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xa) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xb) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xc) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xd) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xe) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0xf) == 0xf) ? v1.s7 : ret.sc;

  ret.sd = ((m.sd & 0xf) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x8) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0x9) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xa) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xb) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xc) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xd) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xe) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0xf) == 0xf) ? v1.s7 : ret.sd;

  ret.se = ((m.se & 0xf) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0xf) == 0x8) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0xf) == 0x9) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0xf) == 0xa) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0xf) == 0xb) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0xf) == 0xc) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0xf) == 0xd) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0xf) == 0xe) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0xf) == 0xf) ? v1.s7 : ret.se;

  ret.sf = ((m.sf & 0xf) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x8) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0x9) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xa) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xb) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xc) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xd) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xe) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0xf) == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE double16 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v16i64(double16 v0, double16 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = ((m.s0 & 0x1f) == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xa) ? v0.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xb) ? v0.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xc) ? v0.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xd) ? v0.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xe) ? v0.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0xf) ? v0.sf : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1e) ? v1.se : ret.s0;
  ret.s0 = ((m.s0 & 0x1f) == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = ((m.s1 & 0x1f) == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xa) ? v0.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xb) ? v0.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xc) ? v0.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xd) ? v0.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xe) ? v0.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0xf) ? v0.sf : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1e) ? v1.se : ret.s1;
  ret.s1 = ((m.s1 & 0x1f) == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = ((m.s2 & 0x1f) == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xa) ? v0.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xb) ? v0.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xc) ? v0.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xd) ? v0.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xe) ? v0.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0xf) ? v0.sf : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1e) ? v1.se : ret.s2;
  ret.s2 = ((m.s2 & 0x1f) == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = ((m.s3 & 0x1f) == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xa) ? v0.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xb) ? v0.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xc) ? v0.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xd) ? v0.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xe) ? v0.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0xf) ? v0.sf : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1e) ? v1.se : ret.s3;
  ret.s3 = ((m.s3 & 0x1f) == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = ((m.s4 & 0x1f) == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xa) ? v0.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xb) ? v0.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xc) ? v0.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xd) ? v0.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xe) ? v0.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0xf) ? v0.sf : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1e) ? v1.se : ret.s4;
  ret.s4 = ((m.s4 & 0x1f) == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = ((m.s5 & 0x1f) == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xa) ? v0.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xb) ? v0.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xc) ? v0.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xd) ? v0.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xe) ? v0.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0xf) ? v0.sf : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1e) ? v1.se : ret.s5;
  ret.s5 = ((m.s5 & 0x1f) == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = ((m.s6 & 0x1f) == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xa) ? v0.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xb) ? v0.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xc) ? v0.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xd) ? v0.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xe) ? v0.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0xf) ? v0.sf : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1e) ? v1.se : ret.s6;
  ret.s6 = ((m.s6 & 0x1f) == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = ((m.s7 & 0x1f) == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xa) ? v0.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xb) ? v0.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xc) ? v0.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xd) ? v0.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xe) ? v0.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0xf) ? v0.sf : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1e) ? v1.se : ret.s7;
  ret.s7 = ((m.s7 & 0x1f) == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = ((m.s8 & 0x1f) == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xa) ? v0.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xb) ? v0.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xc) ? v0.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xd) ? v0.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xe) ? v0.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0xf) ? v0.sf : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1e) ? v1.se : ret.s8;
  ret.s8 = ((m.s8 & 0x1f) == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = ((m.s9 & 0x1f) == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xa) ? v0.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xb) ? v0.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xc) ? v0.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xd) ? v0.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xe) ? v0.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0xf) ? v0.sf : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1e) ? v1.se : ret.s9;
  ret.s9 = ((m.s9 & 0x1f) == 0x1f) ? v1.sf : ret.s9;

  ret.sa = ((m.sa & 0x1f) == 0x0) ? v0.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1) ? v0.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x2) ? v0.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x3) ? v0.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x4) ? v0.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x5) ? v0.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x6) ? v0.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x7) ? v0.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x8) ? v0.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x9) ? v0.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xa) ? v0.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xb) ? v0.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xc) ? v0.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xd) ? v0.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xe) ? v0.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0xf) ? v0.sf : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x10) ? v1.s0 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x11) ? v1.s1 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x12) ? v1.s2 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x13) ? v1.s3 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x14) ? v1.s4 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x15) ? v1.s5 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x16) ? v1.s6 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x17) ? v1.s7 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x18) ? v1.s8 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x19) ? v1.s9 : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1a) ? v1.sa : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1b) ? v1.sb : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1c) ? v1.sc : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1d) ? v1.sd : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1e) ? v1.se : ret.sa;
  ret.sa = ((m.sa & 0x1f) == 0x1f) ? v1.sf : ret.sa;

  ret.sb = ((m.sb & 0x1f) == 0x0) ? v0.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1) ? v0.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x2) ? v0.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x3) ? v0.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x4) ? v0.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x5) ? v0.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x6) ? v0.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x7) ? v0.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x8) ? v0.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x9) ? v0.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xa) ? v0.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xb) ? v0.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xc) ? v0.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xd) ? v0.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xe) ? v0.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0xf) ? v0.sf : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x10) ? v1.s0 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x11) ? v1.s1 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x12) ? v1.s2 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x13) ? v1.s3 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x14) ? v1.s4 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x15) ? v1.s5 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x16) ? v1.s6 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x17) ? v1.s7 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x18) ? v1.s8 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x19) ? v1.s9 : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1a) ? v1.sa : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1b) ? v1.sb : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1c) ? v1.sc : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1d) ? v1.sd : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1e) ? v1.se : ret.sb;
  ret.sb = ((m.sb & 0x1f) == 0x1f) ? v1.sf : ret.sb;

  ret.sc = ((m.sc & 0x1f) == 0x0) ? v0.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1) ? v0.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x2) ? v0.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x3) ? v0.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x4) ? v0.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x5) ? v0.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x6) ? v0.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x7) ? v0.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x8) ? v0.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x9) ? v0.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xa) ? v0.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xb) ? v0.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xc) ? v0.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xd) ? v0.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xe) ? v0.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0xf) ? v0.sf : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x10) ? v1.s0 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x11) ? v1.s1 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x12) ? v1.s2 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x13) ? v1.s3 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x14) ? v1.s4 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x15) ? v1.s5 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x16) ? v1.s6 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x17) ? v1.s7 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x18) ? v1.s8 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x19) ? v1.s9 : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1a) ? v1.sa : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1b) ? v1.sb : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1c) ? v1.sc : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1d) ? v1.sd : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1e) ? v1.se : ret.sc;
  ret.sc = ((m.sc & 0x1f) == 0x1f) ? v1.sf : ret.sc;

  ret.sd = ((m.sd & 0x1f) == 0x0) ? v0.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1) ? v0.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x2) ? v0.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x3) ? v0.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x4) ? v0.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x5) ? v0.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x6) ? v0.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x7) ? v0.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x8) ? v0.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x9) ? v0.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xa) ? v0.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xb) ? v0.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xc) ? v0.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xd) ? v0.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xe) ? v0.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0xf) ? v0.sf : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x10) ? v1.s0 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x11) ? v1.s1 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x12) ? v1.s2 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x13) ? v1.s3 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x14) ? v1.s4 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x15) ? v1.s5 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x16) ? v1.s6 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x17) ? v1.s7 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x18) ? v1.s8 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x19) ? v1.s9 : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1a) ? v1.sa : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1b) ? v1.sb : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1c) ? v1.sc : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1d) ? v1.sd : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1e) ? v1.se : ret.sd;
  ret.sd = ((m.sd & 0x1f) == 0x1f) ? v1.sf : ret.sd;

  ret.se = ((m.se & 0x1f) == 0x0) ? v0.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1) ? v0.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x2) ? v0.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x3) ? v0.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x4) ? v0.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x5) ? v0.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x6) ? v0.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x7) ? v0.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x8) ? v0.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x9) ? v0.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0xa) ? v0.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0xb) ? v0.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0xc) ? v0.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0xd) ? v0.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0xe) ? v0.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0xf) ? v0.sf : ret.se;
  ret.se = ((m.se & 0x1f) == 0x10) ? v1.s0 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x11) ? v1.s1 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x12) ? v1.s2 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x13) ? v1.s3 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x14) ? v1.s4 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x15) ? v1.s5 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x16) ? v1.s6 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x17) ? v1.s7 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x18) ? v1.s8 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x19) ? v1.s9 : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1a) ? v1.sa : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1b) ? v1.sb : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1c) ? v1.sc : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1d) ? v1.sd : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1e) ? v1.se : ret.se;
  ret.se = ((m.se & 0x1f) == 0x1f) ? v1.sf : ret.se;

  ret.sf = ((m.sf & 0x1f) == 0x0) ? v0.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1) ? v0.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x2) ? v0.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x3) ? v0.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x4) ? v0.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x5) ? v0.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x6) ? v0.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x7) ? v0.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x8) ? v0.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x9) ? v0.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xa) ? v0.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xb) ? v0.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xc) ? v0.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xd) ? v0.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xe) ? v0.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0xf) ? v0.sf : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x10) ? v1.s0 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x11) ? v1.s1 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x12) ? v1.s2 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x13) ? v1.s3 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x14) ? v1.s4 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x15) ? v1.s5 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x16) ? v1.s6 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x17) ? v1.s7 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x18) ? v1.s8 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x19) ? v1.s9 : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1a) ? v1.sa : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1b) ? v1.sb : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1c) ? v1.sc : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1d) ? v1.sd : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1e) ? v1.se : ret.sf;
  ret.sf = ((m.sf & 0x1f) == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#endif // defined(cl_khr_fp64)

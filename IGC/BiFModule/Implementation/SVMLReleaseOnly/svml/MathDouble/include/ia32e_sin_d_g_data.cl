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

#ifndef __IA32E_SIN_D_G_DATA_CL__
#define __IA32E_SIN_D_G_DATA_CL__

#include "ia32e_common_trig_data.cl"

typedef struct
{
  VUINT32 _dAbsMask[2];
  VUINT32 _dRangeVal[2];
  VUINT32 _dInvPI[2];
  VUINT32 _dRShifter[2];
  VUINT32 _dZero[2];
  VUINT32 _lNZero[2];
  VUINT32 _dPI1[2];
  VUINT32 _dPI2[2];
  VUINT32 _dPI3[2];
  VUINT32 _dPI4[2];
  VUINT32 _dPI1_FMA[2];
  VUINT32 _dPI2_FMA[2];
  VUINT32 _dPI3_FMA[2];
  VUINT32 _dC1[2];
  VUINT32 _dC2[2];
  VUINT32 _dC3[2];
  VUINT32 _dC4[2];
  VUINT32 _dC5[2];
  VUINT32 _dC6[2];
  VUINT32 _dC7[2];
} dSin_Table_Type;

__constant dSin_Table_Type __ocl_svml_dsin_data = {

  {(VUINT32) ((0x7FFFFFFFFFFFFFFFuLL) >> 0),
   (VUINT32) ((0x7FFFFFFFFFFFFFFFuLL) >> 32)},
  {(VUINT32) ((0x4170000000000000uLL) >> 0),
   (VUINT32) ((0x4170000000000000uLL) >> 32)},
  {(VUINT32) ((0x3FD45F306DC9C883uLL) >> 0),
   (VUINT32) ((0x3FD45F306DC9C883uLL) >> 32)},
  {(VUINT32) ((0x4338000000000000uLL) >> 0),
   (VUINT32) ((0x4338000000000000uLL) >> 32)},
  {(VUINT32) ((0x0000000000000000uLL) >> 0),
   (VUINT32) ((0x0000000000000000uLL) >> 32)},
  {(VUINT32) ((0x8000000000000000uLL) >> 0),
   (VUINT32) ((0x8000000000000000uLL) >> 32)},

  {(VUINT32) ((0x400921FB40000000uLL) >> 0),
   (VUINT32) ((0x400921FB40000000uLL) >> 32)},
  {(VUINT32) ((0x3E84442D00000000uLL) >> 0),
   (VUINT32) ((0x3E84442D00000000uLL) >> 32)},
  {(VUINT32) ((0x3D08469880000000uLL) >> 0),
   (VUINT32) ((0x3D08469880000000uLL) >> 32)},
  {(VUINT32) ((0x3B88CC51701B839AuLL) >> 0),
   (VUINT32) ((0x3B88CC51701B839AuLL) >> 32)},

  {(VUINT32) ((0x400921fb54442d18uLL) >> 0),
   (VUINT32) ((0x400921fb54442d18uLL) >> 32)},
  {(VUINT32) ((0x3ca1a62633145c06uLL) >> 0),
   (VUINT32) ((0x3ca1a62633145c06uLL) >> 32)},
  {(VUINT32) ((0x395c1cd129024e09uLL) >> 0),
   (VUINT32) ((0x395c1cd129024e09uLL) >> 32)},

  {(VUINT32) ((0xBFC55555555554A8uLL) >> 0),
   (VUINT32) ((0xBFC55555555554A8uLL) >> 32)},
  {(VUINT32) ((0x3F8111111110A573uLL) >> 0),
   (VUINT32) ((0x3F8111111110A573uLL) >> 32)},
  {(VUINT32) ((0xBF2A01A019A659DDuLL) >> 0),
   (VUINT32) ((0xBF2A01A019A659DDuLL) >> 32)},
  {(VUINT32) ((0x3EC71DE3806ADD1AuLL) >> 0),
   (VUINT32) ((0x3EC71DE3806ADD1AuLL) >> 32)},
  {(VUINT32) ((0xBE5AE6355AAA4A53uLL) >> 0),
   (VUINT32) ((0xBE5AE6355AAA4A53uLL) >> 32)},
  {(VUINT32) ((0x3DE60E6BEE01D83EuLL) >> 0),
   (VUINT32) ((0x3DE60E6BEE01D83EuLL) >> 32)},
  {(VUINT32) ((0xBD69F1517E9F65F0uLL) >> 0),
   (VUINT32) ((0xBD69F1517E9F65F0uLL) >> 32)},
};

#endif

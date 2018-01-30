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

#ifndef __IA32E_TANPI_D_G_COUT_CL__
#define __IA32E_TANPI_D_G_COUT_CL__

#include "ia32e_common_trig_data.cl"

#define _vmldSinHATab _vmldAcoshAsinhHATab

__attribute__((always_inline))
int
__ocl_svml_dtanpi_cout_rare (__private const double *a, __private double *r)
{
  int nRet = 0;

  double absx;

  absx = (*a);
  (((__private _iml_dp_union_t *) & absx)->dwords.hi_dword =
   (((__private _iml_dp_union_t *) & absx)->dwords.
    hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
  {
    if ((((__private _iml_dp_union_t *) & (absx))->hex[0] ==
         ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldSinHATab)[2]))->
         hex[0])
        && (((__private _iml_dp_union_t *) & (absx))->hex[1] ==
            ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldSinHATab)[2]))->
            hex[1]))
      {

        (*r) = (*a) * ((__constant double *) _vmldSinHATab)[1];

        nRet = 1;
        return nRet;
      }
    else
      {

        (*r) = (*a) * (*a);
        return nRet;
      }
  }

  return nRet;
}

#undef _vmldSinHATab

#endif

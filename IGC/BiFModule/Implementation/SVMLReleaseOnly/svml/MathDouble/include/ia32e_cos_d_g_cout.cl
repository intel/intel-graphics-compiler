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

#ifndef __IA32E_COS_D_G_COUT_CL__
#define __IA32E_COS_D_G_COUT_CL__

#include "ia32e_common_trig_data.cl"

// Temporary alias to common table, _iml_dp_union_t _vmldSinCosHATab[274]
#define _vmldCosHATab _vmldSinCosHATab

int
__ocl_svml_dcos_cout_rare (__private const double *a, __private double *r)
{
  int nRet = 0;

  double yM;
  double absx, y, yN;
  double m_hi, m_lo, m_tail, c, c1, c2, z, z1, z2;
  double polS, polC, x, rr[2];
  double d, rs, med, res_int, corr, res_hi, k0, k1, k2, k3, res_lo;

  int N, iAbsExp, NR;

  absx = (*a);
  (((__private _iml_dp_union_t *) & absx)->dwords.hi_dword =
   (((__private _iml_dp_union_t *) & absx)->dwords.
    hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

  if ((((((__private _iml_dp_union_t *) & (*a))->dwords.hi_dword >> 20) & 0x7FF) !=
       0x7FF))
    {

      iAbsExp =
        ((((__private _iml_dp_union_t *) & absx)->dwords.hi_dword >> 20) & 0x7FF);

      if (iAbsExp >= 0x303)
        {
          if (iAbsExp >= 0x410)
            {

              NR = reduce_pio2d ((*a), rr);

              x = rr[0];
            }
          else
            {

              x = (*a);

              rr[1] = ((__constant double *) _vmldCosHATab)[270];

              NR = 0;
            }

          y = ((__constant double *) _vmldCosHATab)[256] * x;

          yM = y + ((__constant double *) _vmldCosHATab)[257];
          yN = yM - ((__constant double *) _vmldCosHATab)[257];

          N = (int)(((__private _iml_dp_union_t *) & yM)->dwords.lo_dword);

          N += (NR << 4);

          N += 0x1C7610;

          N &= 0x3F;

          m_hi = yN * ((__constant double *) _vmldCosHATab)[258];
          m_lo = yN * ((__constant double *) _vmldCosHATab)[259];
          m_tail = yN * ((__constant double *) _vmldCosHATab)[260];

          z1 = x - m_hi;
          z = z1 - m_lo;
          c1 = z1 - z;
          c2 = c1 - m_lo;
          c = c2 - m_tail;

          c += rr[1];

          z2 = z * z;

          polS =
            (((((__constant double *) _vmldCosHATab)[264] * z2 +
               ((__constant double *) _vmldCosHATab)[263]) * z2 +
              ((__constant double *) _vmldCosHATab)[262]) * z2 +
             ((__constant double *) _vmldCosHATab)[261]) * z2 * z *
            (((__constant double *) _vmldCosHATab)[N * 4 + 0] +
             ((__constant double *) _vmldCosHATab)[N * 4 + 3]);
          polC =
            (((((__constant double *) _vmldCosHATab)[268] * z2 +
               ((__constant double *) _vmldCosHATab)[267]) * z2 +
              ((__constant double *) _vmldCosHATab)[266]) * z2 +
             ((__constant double *) _vmldCosHATab)[265]) * z2 *
            ((__constant double *) _vmldCosHATab)[N * 4 + 1];

          rs = ((__constant double *) _vmldCosHATab)[N * 4 + 3] * z;
          med = ((__constant double *) _vmldCosHATab)[N * 4 + 0] * z;
          res_int = ((__constant double *) _vmldCosHATab)[N * 4 + 1] + rs;
          res_hi = res_int + med;

          d =
            (((__constant double *) _vmldCosHATab)[N * 4 + 0] +
             ((__constant double *) _vmldCosHATab)[N * 4 + 3]);
          d = d - ((__constant double *) _vmldCosHATab)[N * 4 + 1] * z;
          corr = c * d + ((__constant double *) _vmldCosHATab)[N * 4 + 2];

          k0 = ((__constant double *) _vmldCosHATab)[N * 4 + 1] - res_int;
          k1 = res_int - res_hi;
          k2 = k0 + rs;
          k3 = k1 + med;

          res_lo = corr + k2 + k3 + polS + polC;

          (*r) = res_hi + res_lo;
          return nRet;
        }
      else
        {

          (*r) = ((__constant double *) _vmldCosHATab)[269+2] - absx;
          return nRet;
        }
    }
  else
    {
      if ((((__private _iml_dp_union_t *) & (absx))->hex[0] ==
           ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldCosHATab)[271+2]))->
           hex[0])
          && (((__private _iml_dp_union_t *) & (absx))->hex[1] ==
              ((__constant _iml_dp_union_t *) &
               (((__constant double *) _vmldCosHATab)[271+2]))->hex[1]))
        {

          (*r) = (*a) * ((__constant double *) _vmldCosHATab)[270+2];

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

#undef _vmldCosHATab

#endif

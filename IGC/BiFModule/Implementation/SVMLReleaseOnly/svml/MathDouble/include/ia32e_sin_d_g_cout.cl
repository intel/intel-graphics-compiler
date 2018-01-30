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

#ifndef __IA32E_SIN_D_G_COUT_CL__
#define __IA32E_SIN_D_G_COUT_CL__

#include "ia32e_common_trig_data.cl"

__attribute__((always_inline))
int
reduce_pio2d (double x, __private double *d)
{
  double tv, y_hi;

  double x_hi, x_lo, t, z, s, tt, y, y_lo;
  double z0, z1, z2, z3, z4;

  double t1, t2, t3;
  int i, j, k, bitpos, exp, sign;

  sign = (((__private _iml_dp_union_t *) & x)->dwords.hi_dword >> 31);
  (((__private _iml_dp_union_t *) & x)->dwords.hi_dword =
   (((__private _iml_dp_union_t *) & x)->dwords.
    hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));

  if (((((__private _iml_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) <
      0x3FF + 30)
    {

      y = x * ((__constant double *) _vmlReductionTab)[8];
      k =
        (int)( (((0x100000 |
           (((__private _iml_dp_union_t *) & y)->dwords.
            hi_dword & 0x000FFFFF)) << 11) | ((((__private _iml_dp_union_t *) & y)->
                                               dwords.lo_dword) >> (32 -
                                                                    11))) >>
        (0x3FF + 31 -
         ((((__private _iml_dp_union_t *) & y)->dwords.hi_dword >> 20) & 0x7FF)) );
      j = k + (k & 1);
      k = (((k + 1) >> 1) * (1 - (sign << 1))) & 3;

      s = (double) j;

      if (((((__private _iml_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF) <
          0x3FF + 23)
        {
          z0 = s * (&(((__constant double *) _vmlReductionTab)[10]))[0];
          y_hi = (x - z0);
          z1 = s * (&(((__constant double *) _vmlReductionTab)[10]))[1];
          t1 = y_hi;
          y_hi -= z1;
          t1 -= y_hi;
          t1 -= z1;
          y_lo = t1;
          z2 = s * (&(((__constant double *) _vmlReductionTab)[10]))[2];
          t2 = y_hi;
          y_hi -= z2;
          t2 -= y_hi;
          t2 -= z2;
          y_lo += t2;

          z3 = s * (&(((__constant double *) _vmlReductionTab)[10]))[3];
          y_lo -= z3;
        }
      else
        {
          z0 = s * (&(((__constant double *) _vmlReductionTab)[14]))[0];
          y_hi = (x - z0);
          z1 = s * (&(((__constant double *) _vmlReductionTab)[14]))[1];
          t1 = y_hi;
          y_hi -= z1;
          t1 -= y_hi;
          t1 -= z1;
          y_lo = t1;
          z2 = s * (&(((__constant double *) _vmlReductionTab)[14]))[2];
          t2 = y_hi;
          y_hi -= z2;
          t2 -= y_hi;
          t2 -= z2;
          y_lo += t2;
          t = y_hi;
          y_hi += y_lo;
          t -= y_hi;
          y_lo += t;
          z3 = s * (&(((__constant double *) _vmlReductionTab)[14]))[3];
          t3 = y_hi;
          y_hi -= z3;
          t3 -= y_hi;
          t3 -= z3;
          y_lo += t3;
          z4 = s * (&(((__constant double *) _vmlReductionTab)[14]))[4];
          y_lo -= z4;
        }

      s = (&(((__constant double *) _vmlReductionTab)[2]))[sign];
      y_hi = s * y_hi;
      y_lo = s * y_lo;

      d[0] = y_hi;
      d[1] = y_lo;

      t = d[0] + d[1];
      tt = d[0] - t;
      d[1] = tt + d[1];
      d[0] = t;

      return k;
    }
  else
    {

      exp = ((((__private _iml_dp_union_t *) & x)->dwords.hi_dword >> 20) & 0x7FF);
      (((__private _iml_dp_union_t *) & x)->dwords.hi_dword =
       (((__private _iml_dp_union_t *) & x)->dwords.
        hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (exp - 200) & 0x7FF) <<
                                  20));

      x_hi = x;
      ((__private _iml_uint32_t *) & x_hi)[0] &= 0xf8000000;
      x_lo = (x - x_hi);

      bitpos = exp - (29 + 0x3FF);
      j = (bitpos * 0x000147ae) >> 21;
      bitpos = bitpos - j * 25;

      y_hi = x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 0];
      z = x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 1];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      y_lo = (t + z);
      y = (y_hi + y_lo);
      (*(__private _iml_uint64_t *) & y) &=
        (((((__constant _iml_uint64_t *) _vmlReductionTab)[19])) << (38 - bitpos));
      y_hi -= y;

      z = x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 1];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;
      z = x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 2];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;
      y = (y_hi + y_lo);

      z1 = x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 2];
      y += z1;
      z2 = x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 3];
      y += z2;

      i =
        (0x3FF + 52 -
         ((((__private _iml_dp_union_t *) & y)->dwords.hi_dword >> 20) & 0x7FF));
      (*(__private _iml_uint64_t *) & y) >>= i;
      k = (int)(((__private _iml_uint32_t *) & y)[0]);
      (*(__private _iml_uint64_t *) & y) <<= i;

      y_hi -= y;
      t = y_hi;
      y_hi += y_lo;
      t -= y_hi;
      y_lo += t;

      y_hi += (&(((__constant double *) _vmlReductionTab)[0]))[k & 1];
      k = (((k + 1) >> 1) * (1 - (sign << 1))) & 3;

      t = y_hi;
      y_hi += z1;
      t -= y_hi;
      t += z1;
      y_lo += t;
      t = y_hi;
      y_hi += z2;
      t -= y_hi;
      t += z2;
      y_lo += t;

      z = x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 3];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;
      z = x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 4];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;

      z = x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 4];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;
      z = x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 5];
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;

      z =
        (x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 5] +
         x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 6]);
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;

      z =
        (x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 6] +
         x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 7]);
      t = y_hi;
      y_hi += z;
      t -= y_hi;
      t += z;
      y_lo += t;

      z =
        (x_lo * (&(((__constant double *) _vmlReductionTab)[20]))[j + 7] +
         x_hi * (&(((__constant double *) _vmlReductionTab)[20]))[j + 8]);
      t = y_hi;
      y_hi += z;
      s = y_hi * ((__constant double *) _vmlReductionTab)[4];
      tv = (y_hi + s);
      y_hi = (tv - s);
      t -= y_hi;
      t += z;
      y_lo += t;

      y_lo =
        (&(((__constant double *) _vmlReductionTab)[6]))[0] * y_lo +
        (&(((__constant double *) _vmlReductionTab)[6]))[1] * (y_hi + y_lo);
      y_hi = (&(((__constant double *) _vmlReductionTab)[6]))[0] * y_hi;
      t = y_hi;
      y_hi += y_lo;
      t -= y_hi;
      y_lo += t;

      s = (&(((__constant double *) _vmlReductionTab)[2]))[sign];
      d[0] = s * y_hi;
      d[1] = s * y_lo;

      return k;
    }
}

// Alias to _iml_dp_union_t _vmldSinCosHATab[274]
#define _vmldSinHATab _vmldSinCosHATab

__attribute__((always_inline))
int
__ocl_svml_dsin_cout_rare (__private const double *a, __private double *r)
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

              rr[1] = ((__constant double *) _vmldSinHATab)[272];

              NR = 0;
            }

          y = ((__constant double *) _vmldSinHATab)[256] * x;

          yM = y + ((__constant double *) _vmldSinHATab)[257];
          yN = yM - ((__constant double *) _vmldSinHATab)[257];

          N = (int)((((__private _iml_dp_union_t *) & yM)->dwords.lo_dword));

          N += (NR << 4);

          N &= 0x3F;

          m_hi = yN * ((__constant double *) _vmldSinHATab)[258];
          m_lo = yN * ((__constant double *) _vmldSinHATab)[259];
          m_tail = yN * ((__constant double *) _vmldSinHATab)[260];

          z1 = x - m_hi;
          z = z1 - m_lo;
          c1 = z1 - z;
          c2 = c1 - m_lo;
          c = c2 - m_tail;

          c += rr[1];

          z2 = z * z;

          polS =
            (((((__constant double *) _vmldSinHATab)[264] * z2 +
               ((__constant double *) _vmldSinHATab)[263]) * z2 +
              ((__constant double *) _vmldSinHATab)[262]) * z2 +
             ((__constant double *) _vmldSinHATab)[261]) * z2 * z *
            (((__constant double *) _vmldSinHATab)[N * 4 + 0] +
             ((__constant double *) _vmldSinHATab)[N * 4 + 3]);
          polC =
            (((((__constant double *) _vmldSinHATab)[268] * z2 +
               ((__constant double *) _vmldSinHATab)[267]) * z2 +
              ((__constant double *) _vmldSinHATab)[266]) * z2 +
             ((__constant double *) _vmldSinHATab)[265]) * z2 *
            ((__constant double *) _vmldSinHATab)[N * 4 + 1];

          rs = ((__constant double *) _vmldSinHATab)[N * 4 + 3] * z;
          med = ((__constant double *) _vmldSinHATab)[N * 4 + 0] * z;
          res_int = ((__constant double *) _vmldSinHATab)[N * 4 + 1] + rs;
          res_hi = res_int + med;

          d =
            (((__constant double *) _vmldSinHATab)[N * 4 + 0] +
             ((__constant double *) _vmldSinHATab)[N * 4 + 3]);
          d = d - ((__constant double *) _vmldSinHATab)[N * 4 + 1] * z;
          corr = c * d + ((__constant double *) _vmldSinHATab)[N * 4 + 2];

          k0 = ((__constant double *) _vmldSinHATab)[N * 4 + 1] - res_int;
          k1 = res_int - res_hi;
          k2 = k0 + rs;
          k3 = k1 + med;

          res_lo = corr + k2 + k3 + polS + polC;

          (*r) = res_hi + res_lo;

          return nRet;
        }
      else
        {
          if ((*a) != ((__constant double *) _vmldSinHATab)[272])
            {

              (*r) = ((__constant double *) _vmldSinHATab)[269] * (*a) - (*a);
              (*r) = ((__constant double *) _vmldSinHATab)[270] * (*r);
              return nRet;
            }
          else
            {

              (*r) = (*a);
              return nRet;
            }
        }
    }
  else
    {
      if ((((__private _iml_dp_union_t *) & (absx))->hex[0] ==
           ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldSinHATab)[273]))->
           hex[0])
          && (((__private _iml_dp_union_t *) & (absx))->hex[1] ==
              ((__constant _iml_dp_union_t *) &
               (((__constant double *) _vmldSinHATab)[273]))->hex[1]))
        {

          (*r) = (*a) * ((__constant double *) _vmldSinHATab)[272];

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

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

#ifndef __IA32E_TAN_D_G_COUT_CL__
#define __IA32E_TAN_D_G_COUT_CL__

#include "ia32e_common_trig_data.cl"

__attribute__((always_inline))
int
own_reduce_pio2d (double x, __private double *d)
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
        (((0x100000 |
           (((__private _iml_dp_union_t *) & y)->dwords.
            hi_dword & 0x000FFFFF)) << 11) | ((((__private _iml_dp_union_t *) & y)->
                                               dwords.lo_dword) >> (32 -
                                                                    11))) >>
        (0x3FF + 31 -
         ((((__private _iml_dp_union_t *) & y)->dwords.hi_dword >> 20) & 0x7FF));
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
      k = ((__private _iml_uint32_t *) & y)[0];
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

__constant _iml_dp_union_t _vmldTanHATab[16 + 640] = {
  {{0x6DC9C883u, 0x40245F30u}},
  {{0x00000000u, 0x43380000u}},
  {{0x00000000u, 0x43780000u}},
  {{0x54444000u, 0x3FB921FBu}},
  {{0x67674000u, 0xBD32E7B9u}},
  {{0x3707344Au, 0x3AA8A2E0u}},
  {{0x54440000u, 0x3FB921FBu}},
  {{0x4C4C0000u, 0x3D468C23u}},
  {{0x03707345u, 0x3AE98A2Eu}},
  {{0x676733AFu, 0x3D32E7B9u}},
  {{0xFFFC0000u, 0xFFFFFFFFu}},
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x7FF00000u}},
  {{0x00000000u, 0x43600000u}},
  {{0x00000000u, 0x3C800000u}},
  {{0x00000000u, 0x00000000u}}, {{0x882C10FAu, 0x3F9664F4u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x55E6C23Du, 0x3F8226E3u}}, {{0x55555555u, 0x3FD55555u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x0E157DE0u, 0x3F6D6D3Du}}, {{0x11111111u, 0x3FC11111u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x452B75E3u, 0x3F57DA36u}}, {{0x1BA1BA1Cu, 0x3FABA1BAu}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x4E435F9Bu, 0x3F953F83u}}, {{0x3C6E8E46u, 0x3F9B74EAu}},
  {{0xDA5B7511u, 0x3F85AD63u}}, {{0xDC230B9Bu, 0x3FB97558u}},
  {{0x26CB3788u, 0x3F881308u}}, {{0x76FC4985u, 0x3FD62AC9u}},
  {{0x77BB08BAu, 0x3F757C85u}}, {{0xB6247521u, 0x3FB1381Eu}},
  {{0x5922170Cu, 0x3F754E95u}}, {{0x8746482Du, 0x3FC27F83u}},
  {{0x11055B30u, 0x3F64E391u}}, {{0x3E666320u, 0x3FA3E609u}},
  {{0x0DE9DAE3u, 0x3F6301DFu}}, {{0x1F1DCA06u, 0x3FAFA8AEu}},
  {{0x8C5B2DA2u, 0x3FB936BBu}}, {{0x4E88F7A5u, 0x3C587D05u}},
  {{0x00000000u, 0x3FF00000u}}, {{0xA8935DD9u, 0x3F83DDE2u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x5A279EA3u, 0x3FAA3407u}}, {{0x432D65FAu, 0x3FA70153u}},
  {{0x891A4602u, 0x3F9D03EFu}}, {{0xD62CA5F8u, 0x3FCA77D9u}},
  {{0xB35F4628u, 0x3F97A265u}}, {{0x433258FAu, 0x3FD8CF51u}},
  {{0xB58FD909u, 0x3F8F88E3u}}, {{0x01771CEAu, 0x3FC2B154u}},
  {{0xF3562F8Eu, 0x3F888F57u}}, {{0xC028A723u, 0x3FC7370Fu}},
  {{0x20B7F9F0u, 0x3F80F44Cu}}, {{0x214368E9u, 0x3FB6DFAAu}},
  {{0x28891863u, 0x3F79B4B6u}}, {{0x172DBBF0u, 0x3FB6CB8Eu}},
  {{0xE0553158u, 0x3FC975F5u}}, {{0x593FE814u, 0x3C2EF5D3u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x03DEC550u, 0x3FA44203u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x9314533Eu, 0x3FBB8EC5u}}, {{0x09AA36D0u, 0x3FB6D3F4u}},
  {{0xDCB427FDu, 0x3FB13950u}}, {{0xD87AB0BBu, 0x3FD5335Eu}},
  {{0xCE0AE8A5u, 0x3FABB382u}}, {{0x79143126u, 0x3FDDBA41u}},
  {{0x5F2B28D4u, 0x3FA552F1u}}, {{0x59F21A6Du, 0x3FD015ABu}},
  {{0x22C27D95u, 0x3FA0E984u}}, {{0xE19FC6AAu, 0x3FD0576Cu}},
  {{0x8F2C2950u, 0x3F9A4898u}}, {{0xC0B3F22Cu, 0x3FC59462u}},
  {{0x1883A4B8u, 0x3F94B61Cu}}, {{0x3F838640u, 0x3FC30EB8u}},
  {{0x355C63DCu, 0x3FD36A08u}}, {{0x1DCE993Du, 0xBC6D704Du}},
  {{0x00000000u, 0x3FF00000u}}, {{0x2B82AB63u, 0x3FB78E92u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x56F37042u, 0x3FCCFC56u}}, {{0xAA563951u, 0x3FC90125u}},
  {{0x3D0E7C5Du, 0x3FC50533u}}, {{0x9BED9B2Eu, 0x3FDF0ED9u}},
  {{0x5FE7C47Cu, 0x3FC1F250u}}, {{0x96C125E5u, 0x3FE2EDD9u}},
  {{0x5A02BBD8u, 0x3FBE5C71u}}, {{0x86362C20u, 0x3FDA08B7u}},
  {{0x4B4435EDu, 0x3FB9D342u}}, {{0x4B494091u, 0x3FD911BDu}},
  {{0xB56658BEu, 0x3FB5E4C7u}}, {{0x93A2FD76u, 0x3FD3C092u}},
  {{0xDA271794u, 0x3FB29910u}}, {{0x3303DF2Bu, 0x3FD189BEu}},
  {{0x99FCEF32u, 0x3FDA8279u}}, {{0xB68C1467u, 0x3C708B2Fu}},
  {{0x00000000u, 0x3FF00000u}}, {{0x980C4337u, 0x3FC5F619u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xCC03E501u, 0x3FDFF10Fu}}, {{0x44A4E845u, 0x3FDDB63Bu}},
  {{0x3768AD9Fu, 0x3FDB72A4u}}, {{0x3DD01CCAu, 0x3FE5FDB9u}},
  {{0xA61D2811u, 0x3FD972B2u}}, {{0x5645AD0Bu, 0x3FE977F9u}},
  {{0xD013B3ABu, 0x3FD78CA3u}}, {{0xBF0BF914u, 0x3FE4F192u}},
  {{0x4D53E730u, 0x3FD5D060u}}, {{0x3F8B9000u, 0x3FE49933u}},
  {{0xE2B82F08u, 0x3FD4322Au}}, {{0x5936A835u, 0x3FE27AE1u}},
  {{0xB1C61C9Bu, 0x3FD2B3FBu}}, {{0xEF478605u, 0x3FE1659Eu}},
  {{0x190834ECu, 0x3FE11AB7u}}, {{0xCDB625EAu, 0xBC8E564Bu}},
  {{0x00000000u, 0x3FF00000u}}, {{0xB07217E3u, 0x3FD248F1u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x2B2C49D0u, 0x3FF2DE9Cu}}, {{0x2655BC98u, 0x3FF33E58u}},
  {{0xFF691FA2u, 0x3FF3972Eu}}, {{0xE93463BDu, 0x3FEEED87u}},
  {{0x070E10A0u, 0x3FF3F5B2u}}, {{0xF4D790A4u, 0x3FF20C10u}},
  {{0xA04E8EA3u, 0x3FF4541Au}}, {{0x386ACCD3u, 0x3FF1369Eu}},
  {{0x222A66DDu, 0x3FF4B521u}}, {{0x22A9777Eu, 0x3FF20817u}},
  {{0x52A04A6Eu, 0x3FF5178Fu}}, {{0xDDAA0031u, 0x3FF22137u}},
  {{0x4447D47Cu, 0x3FF57C01u}}, {{0x1E9C7F1Du, 0x3FF29311u}},
  {{0x2AB7F990u, 0x3FE561B8u}}, {{0x209C7DF1u, 0x3C87A8C5u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x4170BCC6u, 0x3FDC92D8u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xC7AB4D5Au, 0x40085E24u}}, {{0xE93EA75Du, 0x400B963Du}},
  {{0x94A7F25Au, 0x400F37E2u}}, {{0x4B6261CBu, 0x3FF5F984u}},
  {{0x5A9DD812u, 0x4011AAB0u}}, {{0x74C30018u, 0x3FFAF5A5u}},
  {{0x7F2CE8E3u, 0x4013FE8Bu}}, {{0xFE8E54FAu, 0x3FFD7334u}},
  {{0x670D618Du, 0x4016A10Cu}}, {{0x4DB97058u, 0x4000E012u}},
  {{0x24DF44DDu, 0x40199C5Fu}}, {{0x697D6ECEu, 0x4003006Eu}},
  {{0x83298B82u, 0x401CFC4Du}}, {{0x19D490D6u, 0x40058C19u}},
  {{0x2AE42850u, 0x3FEA4300u}}, {{0x118E20E6u, 0xBC7A6DB8u}},
  {{0x00000000u, 0x40000000u}}, {{0xE33345B8u, 0xBFD4E526u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x65965966u, 0x40219659u}}, {{0x882C10FAu, 0x402664F4u}},
  {{0x83CD3723u, 0x402C8342u}}, {{0x00000000u, 0x40000000u}},
  {{0x55E6C23Du, 0x403226E3u}}, {{0x55555555u, 0x40055555u}},
  {{0x34451939u, 0x40371C96u}}, {{0xAAAAAAABu, 0x400AAAAAu}},
  {{0x0E157DE0u, 0x403D6D3Du}}, {{0x11111111u, 0x40111111u}},
  {{0xA738201Fu, 0x4042BBCEu}}, {{0x05B05B06u, 0x4015B05Bu}},
  {{0x452B75E3u, 0x4047DA36u}}, {{0x1BA1BA1Cu, 0x401BA1BAu}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x40000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x4F48B8D3u, 0xBF33EAF9u}}, {{0x0CF7586Fu, 0x3F20B8EAu}},
  {{0xD0258911u, 0xBF0ABAF3u}}, {{0x23E49FE9u, 0xBFAB5A8Cu}},
  {{0x2D53222Eu, 0x3EF60D15u}}, {{0x21169451u, 0x3FA172B2u}},
  {{0xBB254DBCu, 0xBEE1D3B5u}}, {{0xDBF93B8Eu, 0xBF84C7DBu}},
  {{0x05B4630Bu, 0x3ECD3364u}}, {{0xEE9AADA7u, 0x3F743924u}},
  {{0x794A8297u, 0xBEB7B7B9u}}, {{0xE015F797u, 0xBF5D41F5u}},
  {{0xE41A4A56u, 0x3EA35DFBu}}, {{0xE4C2A251u, 0x3F49A2ABu}},
  {{0x5AF9E000u, 0xBFCE49CEu}}, {{0x8C743719u, 0x3D1EB860u}},
  {{0x00000000u, 0x00000000u}}, {{0x1B4863CFu, 0x3FD78294u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x535AD890u, 0xBF2B9320u}}, {{0x018FDF1Fu, 0x3F16D61Du}},
  {{0x0359F1BEu, 0xBF0139E4u}}, {{0xA4317C6Du, 0xBFA67E17u}},
  {{0x82672D0Fu, 0x3EEBB405u}}, {{0x2F1B621Eu, 0x3F9F455Bu}},
  {{0x51CCF238u, 0xBED55317u}}, {{0xF437B9ACu, 0xBF804BEEu}},
  {{0xC791A2B5u, 0x3EC0E993u}}, {{0x919A1DB2u, 0x3F7080C2u}},
  {{0x336A5B0Eu, 0xBEAA48A2u}}, {{0x0A268358u, 0xBF55A443u}},
  {{0xDFD978E4u, 0x3E94B61Fu}}, {{0xD7767A58u, 0x3F431806u}},
  {{0x2AEA0000u, 0xBFC9BBE8u}}, {{0x7723EA61u, 0xBD3A2369u}},
  {{0x00000000u, 0x00000000u}}, {{0xDF7796FFu, 0x3FD6E642u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0xB9FF07CEu, 0xBF231C78u}}, {{0xA5517182u, 0x3F0FF0E0u}},
  {{0x790B4CBCu, 0xBEF66191u}}, {{0x848A46C6u, 0xBFA21AC0u}},
  {{0xB16435FAu, 0x3EE1D3ECu}}, {{0x2A1AA832u, 0x3F9C71EAu}},
  {{0xFDD299EFu, 0xBEC9DD1Au}}, {{0x3F8DBAAFu, 0xBF793363u}},
  {{0x309FC6EAu, 0x3EB415D6u}}, {{0xBEE60471u, 0x3F6B83BAu}},
  {{0x94A0A697u, 0xBE9DAE11u}}, {{0x3E5C67B3u, 0xBF4FD07Bu}},
  {{0x9A8F3E3Eu, 0x3E86BD75u}}, {{0xA4BEB7A4u, 0x3F3D1EB1u}},
  {{0x29CFC000u, 0xBFC549CEu}}, {{0xBF159358u, 0xBD397B33u}},
  {{0x00000000u, 0x00000000u}}, {{0x871FEE6Cu, 0x3FD666F0u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x00000000u, 0x00000000u}}, {{0x11CD6C69u, 0x3FD601FDu}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x1A154B97u, 0xBF116B01u}}, {{0x2D427630u, 0x3F0147BFu}},
  {{0xB93820C8u, 0xBEE264D4u}}, {{0xBB6CBB18u, 0xBF94AB8Cu}},
  {{0x888D4D92u, 0x3ED0568Bu}}, {{0x60730F7Cu, 0x3F98B19Bu}},
  {{0xE4B1FB11u, 0xBEB2F950u}}, {{0x22CF9F74u, 0xBF6B21CDu}},
  {{0x4A3FF0A6u, 0x3E9F499Eu}}, {{0xFD2B83CEu, 0x3F64AAD7u}},
  {{0x637B73AFu, 0xBE83487Cu}}, {{0xE522591Au, 0xBF3FC092u}},
  {{0xA158E8BCu, 0x3E6E3AAEu}}, {{0xE5E82FFAu, 0x3F329D2Fu}},
  {{0xD636A000u, 0xBFB9477Fu}}, {{0xC2C2D2BCu, 0xBD135EF9u}},
  {{0x00000000u, 0x00000000u}}, {{0xF2FDB123u, 0x3FD5B566u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0xC41ACB64u, 0xBF05448Du}}, {{0xDBB03D6Fu, 0x3EFB7AD2u}},
  {{0x9E42962Du, 0xBED5AEA5u}}, {{0x2579F8EFu, 0xBF8B2398u}},
  {{0x288A1ED9u, 0x3EC81441u}}, {{0xB0198DC5u, 0x3F979A3Au}},
  {{0x2FDFE253u, 0xBEA57CD3u}}, {{0x5766336Fu, 0xBF617CAAu}},
  {{0x600944C3u, 0x3E954ED6u}}, {{0xA4E0AAF8u, 0x3F62C646u}},
  {{0x6B8FB29Cu, 0xBE74E3A3u}}, {{0xDC4C0409u, 0xBF33F952u}},
  {{0x9BFFE365u, 0x3E6301ECu}}, {{0xB8869E44u, 0x3F2FC566u}},
  {{0xE1E04000u, 0xBFB0CC62u}}, {{0x016B907Fu, 0xBD119CBCu}},
  {{0x00000000u, 0x00000000u}}, {{0xE6B9D8FAu, 0x3FD57FB3u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x5DAF22A6u, 0xBEF429D7u}}, {{0x06BCA545u, 0x3EF7A27Du}},
  {{0x7211C19Au, 0xBEC41C3Eu}}, {{0x956ED53Eu, 0xBF7AE3F4u}},
  {{0xEE750E72u, 0x3EC3901Bu}}, {{0x91D443F5u, 0x3F96F713u}},
  {{0x36661E6Cu, 0xBE936E09u}}, {{0x506F9381u, 0xBF5122E8u}},
  {{0xCB6DD43Fu, 0x3E9041B9u}}, {{0x6698B2FFu, 0x3F61B0C7u}},
  {{0x576BF12Bu, 0xBE625A8Au}}, {{0xE5A0E9DCu, 0xBF23499Du}},
  {{0x110384DDu, 0x3E5B1C2Cu}}, {{0x68D43DB6u, 0x3F2CB899u}},
  {{0x6ECAC000u, 0xBFA0C414u}}, {{0xCD7DD58Cu, 0x3D13500Fu}},
  {{0x00000000u, 0x00000000u}}, {{0x85A2C8FBu, 0x3FD55FE0u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x00000000u, 0x00000000u}}, {{0x2BF70EBEu, 0x3EF66A8Fu}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xD644267Fu, 0x3EC22805u}}, {{0x16C16C17u, 0x3F96C16Cu}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xC4E09162u, 0x3E8D6DB2u}}, {{0xBC011567u, 0x3F61566Au}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x1F79955Cu, 0x3E57DA4Eu}}, {{0x9334EF0Bu, 0x3F2BBD77u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}}, {{0x55555555u, 0x3FD55555u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x5DAF22A6u, 0x3EF429D7u}}, {{0x06BCA545u, 0x3EF7A27Du}},
  {{0x7211C19Au, 0x3EC41C3Eu}}, {{0x956ED53Eu, 0x3F7AE3F4u}},
  {{0xEE750E72u, 0x3EC3901Bu}}, {{0x91D443F5u, 0x3F96F713u}},
  {{0x36661E6Cu, 0x3E936E09u}}, {{0x506F9381u, 0x3F5122E8u}},
  {{0xCB6DD43Fu, 0x3E9041B9u}}, {{0x6698B2FFu, 0x3F61B0C7u}},
  {{0x576BF12Bu, 0x3E625A8Au}}, {{0xE5A0E9DCu, 0x3F23499Du}},
  {{0x110384DDu, 0x3E5B1C2Cu}}, {{0x68D43DB6u, 0x3F2CB899u}},
  {{0x6ECAC000u, 0x3FA0C414u}}, {{0xCD7DD58Cu, 0xBD13500Fu}},
  {{0x00000000u, 0x00000000u}}, {{0x85A2C8FBu, 0x3FD55FE0u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0xC41ACB64u, 0x3F05448Du}}, {{0xDBB03D6Fu, 0x3EFB7AD2u}},
  {{0x9E42962Du, 0x3ED5AEA5u}}, {{0x2579F8EFu, 0x3F8B2398u}},
  {{0x288A1ED9u, 0x3EC81441u}}, {{0xB0198DC5u, 0x3F979A3Au}},
  {{0x2FDFE253u, 0x3EA57CD3u}}, {{0x5766336Fu, 0x3F617CAAu}},
  {{0x600944C3u, 0x3E954ED6u}}, {{0xA4E0AAF8u, 0x3F62C646u}},
  {{0x6B8FB29Cu, 0x3E74E3A3u}}, {{0xDC4C0409u, 0x3F33F952u}},
  {{0x9BFFE365u, 0x3E6301ECu}}, {{0xB8869E44u, 0x3F2FC566u}},
  {{0xE1E04000u, 0x3FB0CC62u}}, {{0x016B907Fu, 0x3D119CBCu}},
  {{0x00000000u, 0x00000000u}}, {{0xE6B9D8FAu, 0x3FD57FB3u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x1A154B97u, 0x3F116B01u}}, {{0x2D427630u, 0x3F0147BFu}},
  {{0xB93820C8u, 0x3EE264D4u}}, {{0xBB6CBB18u, 0x3F94AB8Cu}},
  {{0x888D4D92u, 0x3ED0568Bu}}, {{0x60730F7Cu, 0x3F98B19Bu}},
  {{0xE4B1FB11u, 0x3EB2F950u}}, {{0x22CF9F74u, 0x3F6B21CDu}},
  {{0x4A3FF0A6u, 0x3E9F499Eu}}, {{0xFD2B83CEu, 0x3F64AAD7u}},
  {{0x637B73AFu, 0x3E83487Cu}}, {{0xE522591Au, 0x3F3FC092u}},
  {{0xA158E8BCu, 0x3E6E3AAEu}}, {{0xE5E82FFAu, 0x3F329D2Fu}},
  {{0xD636A000u, 0x3FB9477Fu}}, {{0xC2C2D2BCu, 0x3D135EF9u}},
  {{0x00000000u, 0x00000000u}}, {{0xF2FDB123u, 0x3FD5B566u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x7D98A556u, 0x3F1A3958u}}, {{0x9D88DC01u, 0x3F0704C2u}},
  {{0x73742A2Bu, 0x3EED054Au}}, {{0x58844587u, 0x3F9C2A13u}},
  {{0x55688A79u, 0x3ED7A326u}}, {{0xEE33F1D6u, 0x3F9A48F4u}},
  {{0xA8DC9888u, 0x3EBF8939u}}, {{0xAAD4B5B8u, 0x3F72F746u}},
  {{0x9102EFA1u, 0x3EA88F82u}}, {{0xDABC29CFu, 0x3F678228u}},
  {{0x9289AFB8u, 0x3E90F456u}}, {{0x741FB4EDu, 0x3F46F3A3u}},
  {{0xA97F6663u, 0x3E79B4BFu}}, {{0xCA89FF3Fu, 0x3F36DB70u}},
  {{0xA8A2A000u, 0x3FC0EE13u}}, {{0x3DA24BE1u, 0x3D338B9Fu}},
  {{0x00000000u, 0x00000000u}}, {{0x11CD6C69u, 0x3FD601FDu}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0xB9FF07CEu, 0x3F231C78u}}, {{0xA5517182u, 0x3F0FF0E0u}},
  {{0x790B4CBCu, 0x3EF66191u}}, {{0x848A46C6u, 0x3FA21AC0u}},
  {{0xB16435FAu, 0x3EE1D3ECu}}, {{0x2A1AA832u, 0x3F9C71EAu}},
  {{0xFDD299EFu, 0x3EC9DD1Au}}, {{0x3F8DBAAFu, 0x3F793363u}},
  {{0x309FC6EAu, 0x3EB415D6u}}, {{0xBEE60471u, 0x3F6B83BAu}},
  {{0x94A0A697u, 0x3E9DAE11u}}, {{0x3E5C67B3u, 0x3F4FD07Bu}},
  {{0x9A8F3E3Eu, 0x3E86BD75u}}, {{0xA4BEB7A4u, 0x3F3D1EB1u}},
  {{0x29CFC000u, 0x3FC549CEu}}, {{0xBF159358u, 0x3D397B33u}},
  {{0x00000000u, 0x00000000u}}, {{0x871FEE6Cu, 0x3FD666F0u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x535AD890u, 0x3F2B9320u}}, {{0x018FDF1Fu, 0x3F16D61Du}},
  {{0x0359F1BEu, 0x3F0139E4u}}, {{0xA4317C6Du, 0x3FA67E17u}},
  {{0x82672D0Fu, 0x3EEBB405u}}, {{0x2F1B621Eu, 0x3F9F455Bu}},
  {{0x51CCF238u, 0x3ED55317u}}, {{0xF437B9ACu, 0x3F804BEEu}},
  {{0xC791A2B5u, 0x3EC0E993u}}, {{0x919A1DB2u, 0x3F7080C2u}},
  {{0x336A5B0Eu, 0x3EAA48A2u}}, {{0x0A268358u, 0x3F55A443u}},
  {{0xDFD978E4u, 0x3E94B61Fu}}, {{0xD7767A58u, 0x3F431806u}},
  {{0x2AEA0000u, 0x3FC9BBE8u}}, {{0x7723EA61u, 0x3D3A2369u}},
  {{0x00000000u, 0x00000000u}}, {{0xDF7796FFu, 0x3FD6E642u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x4F48B8D3u, 0x3F33EAF9u}}, {{0x0CF7586Fu, 0x3F20B8EAu}},
  {{0xD0258911u, 0x3F0ABAF3u}}, {{0x23E49FE9u, 0x3FAB5A8Cu}},
  {{0x2D53222Eu, 0x3EF60D15u}}, {{0x21169451u, 0x3FA172B2u}},
  {{0xBB254DBCu, 0x3EE1D3B5u}}, {{0xDBF93B8Eu, 0x3F84C7DBu}},
  {{0x05B4630Bu, 0x3ECD3364u}}, {{0xEE9AADA7u, 0x3F743924u}},
  {{0x794A8297u, 0x3EB7B7B9u}}, {{0xE015F797u, 0x3F5D41F5u}},
  {{0xE41A4A56u, 0x3EA35DFBu}}, {{0xE4C2A251u, 0x3F49A2ABu}},
  {{0x5AF9E000u, 0x3FCE49CEu}}, {{0x8C743719u, 0xBD1EB860u}},
  {{0x00000000u, 0x00000000u}}, {{0x1B4863CFu, 0x3FD78294u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x00000000u, 0xFFFFFFF8u}},
  {{0x65965966u, 0xC0219659u}}, {{0x882C10FAu, 0x402664F4u}},
  {{0x83CD3723u, 0xC02C8342u}}, {{0x00000000u, 0xC0000000u}},
  {{0x55E6C23Du, 0x403226E3u}}, {{0x55555555u, 0x40055555u}},
  {{0x34451939u, 0xC0371C96u}}, {{0xAAAAAAABu, 0xC00AAAAAu}},
  {{0x0E157DE0u, 0x403D6D3Du}}, {{0x11111111u, 0x40111111u}},
  {{0xA738201Fu, 0xC042BBCEu}}, {{0x05B05B06u, 0xC015B05Bu}},
  {{0x452B75E3u, 0x4047DA36u}}, {{0x1BA1BA1Cu, 0x401BA1BAu}},
  {{0x00000000u, 0xBFF00000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x40000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xC7AB4D5Au, 0xC0085E24u}}, {{0xE93EA75Du, 0x400B963Du}},
  {{0x94A7F25Au, 0xC00F37E2u}}, {{0x4B6261CBu, 0xBFF5F984u}},
  {{0x5A9DD812u, 0x4011AAB0u}}, {{0x74C30018u, 0x3FFAF5A5u}},
  {{0x7F2CE8E3u, 0xC013FE8Bu}}, {{0xFE8E54FAu, 0xBFFD7334u}},
  {{0x670D618Du, 0x4016A10Cu}}, {{0x4DB97058u, 0x4000E012u}},
  {{0x24DF44DDu, 0xC0199C5Fu}}, {{0x697D6ECEu, 0xC003006Eu}},
  {{0x83298B82u, 0x401CFC4Du}}, {{0x19D490D6u, 0x40058C19u}},
  {{0x2AE42850u, 0xBFEA4300u}}, {{0x118E20E6u, 0x3C7A6DB8u}},
  {{0x00000000u, 0x40000000u}}, {{0xE33345B8u, 0xBFD4E526u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x2B2C49D0u, 0xBFF2DE9Cu}}, {{0x2655BC98u, 0x3FF33E58u}},
  {{0xFF691FA2u, 0xBFF3972Eu}}, {{0xE93463BDu, 0xBFEEED87u}},
  {{0x070E10A0u, 0x3FF3F5B2u}}, {{0xF4D790A4u, 0x3FF20C10u}},
  {{0xA04E8EA3u, 0xBFF4541Au}}, {{0x386ACCD3u, 0xBFF1369Eu}},
  {{0x222A66DDu, 0x3FF4B521u}}, {{0x22A9777Eu, 0x3FF20817u}},
  {{0x52A04A6Eu, 0xBFF5178Fu}}, {{0xDDAA0031u, 0xBFF22137u}},
  {{0x4447D47Cu, 0x3FF57C01u}}, {{0x1E9C7F1Du, 0x3FF29311u}},
  {{0x2AB7F990u, 0xBFE561B8u}}, {{0x209C7DF1u, 0xBC87A8C5u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x4170BCC6u, 0x3FDC92D8u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0xCC03E501u, 0xBFDFF10Fu}}, {{0x44A4E845u, 0x3FDDB63Bu}},
  {{0x3768AD9Fu, 0xBFDB72A4u}}, {{0x3DD01CCAu, 0xBFE5FDB9u}},
  {{0xA61D2811u, 0x3FD972B2u}}, {{0x5645AD0Bu, 0x3FE977F9u}},
  {{0xD013B3ABu, 0xBFD78CA3u}}, {{0xBF0BF914u, 0xBFE4F192u}},
  {{0x4D53E730u, 0x3FD5D060u}}, {{0x3F8B9000u, 0x3FE49933u}},
  {{0xE2B82F08u, 0xBFD4322Au}}, {{0x5936A835u, 0xBFE27AE1u}},
  {{0xB1C61C9Bu, 0x3FD2B3FBu}}, {{0xEF478605u, 0x3FE1659Eu}},
  {{0x190834ECu, 0xBFE11AB7u}}, {{0xCDB625EAu, 0x3C8E564Bu}},
  {{0x00000000u, 0x3FF00000u}}, {{0xB07217E3u, 0x3FD248F1u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x56F37042u, 0xBFCCFC56u}}, {{0xAA563951u, 0x3FC90125u}},
  {{0x3D0E7C5Du, 0xBFC50533u}}, {{0x9BED9B2Eu, 0xBFDF0ED9u}},
  {{0x5FE7C47Cu, 0x3FC1F250u}}, {{0x96C125E5u, 0x3FE2EDD9u}},
  {{0x5A02BBD8u, 0xBFBE5C71u}}, {{0x86362C20u, 0xBFDA08B7u}},
  {{0x4B4435EDu, 0x3FB9D342u}}, {{0x4B494091u, 0x3FD911BDu}},
  {{0xB56658BEu, 0xBFB5E4C7u}}, {{0x93A2FD76u, 0xBFD3C092u}},
  {{0xDA271794u, 0x3FB29910u}}, {{0x3303DF2Bu, 0x3FD189BEu}},
  {{0x99FCEF32u, 0xBFDA8279u}}, {{0xB68C1467u, 0xBC708B2Fu}},
  {{0x00000000u, 0x3FF00000u}}, {{0x980C4337u, 0x3FC5F619u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x9314533Eu, 0xBFBB8EC5u}}, {{0x09AA36D0u, 0x3FB6D3F4u}},
  {{0xDCB427FDu, 0xBFB13950u}}, {{0xD87AB0BBu, 0xBFD5335Eu}},
  {{0xCE0AE8A5u, 0x3FABB382u}}, {{0x79143126u, 0x3FDDBA41u}},
  {{0x5F2B28D4u, 0xBFA552F1u}}, {{0x59F21A6Du, 0xBFD015ABu}},
  {{0x22C27D95u, 0x3FA0E984u}}, {{0xE19FC6AAu, 0x3FD0576Cu}},
  {{0x8F2C2950u, 0xBF9A4898u}}, {{0xC0B3F22Cu, 0xBFC59462u}},
  {{0x1883A4B8u, 0x3F94B61Cu}}, {{0x3F838640u, 0x3FC30EB8u}},
  {{0x355C63DCu, 0xBFD36A08u}}, {{0x1DCE993Du, 0x3C6D704Du}},
  {{0x00000000u, 0x3FF00000u}}, {{0x2B82AB63u, 0x3FB78E92u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x5A279EA3u, 0xBFAA3407u}}, {{0x432D65FAu, 0x3FA70153u}},
  {{0x891A4602u, 0xBF9D03EFu}}, {{0xD62CA5F8u, 0xBFCA77D9u}},
  {{0xB35F4628u, 0x3F97A265u}}, {{0x433258FAu, 0x3FD8CF51u}},
  {{0xB58FD909u, 0xBF8F88E3u}}, {{0x01771CEAu, 0xBFC2B154u}},
  {{0xF3562F8Eu, 0x3F888F57u}}, {{0xC028A723u, 0x3FC7370Fu}},
  {{0x20B7F9F0u, 0xBF80F44Cu}}, {{0x214368E9u, 0xBFB6DFAAu}},
  {{0x28891863u, 0x3F79B4B6u}}, {{0x172DBBF0u, 0x3FB6CB8Eu}},
  {{0xE0553158u, 0xBFC975F5u}}, {{0x593FE814u, 0xBC2EF5D3u}},
  {{0x00000000u, 0x3FF00000u}}, {{0x03DEC550u, 0x3FA44203u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}},
  {{0x4E435F9Bu, 0xBF953F83u}}, {{0x3C6E8E46u, 0x3F9B74EAu}},
  {{0xDA5B7511u, 0xBF85AD63u}}, {{0xDC230B9Bu, 0xBFB97558u}},
  {{0x26CB3788u, 0x3F881308u}}, {{0x76FC4985u, 0x3FD62AC9u}},
  {{0x77BB08BAu, 0xBF757C85u}}, {{0xB6247521u, 0xBFB1381Eu}},
  {{0x5922170Cu, 0x3F754E95u}}, {{0x8746482Du, 0x3FC27F83u}},
  {{0x11055B30u, 0xBF64E391u}}, {{0x3E666320u, 0xBFA3E609u}},
  {{0x0DE9DAE3u, 0x3F6301DFu}}, {{0x1F1DCA06u, 0x3FAFA8AEu}},
  {{0x8C5B2DA2u, 0xBFB936BBu}}, {{0x4E88F7A5u, 0xBC587D05u}},
  {{0x00000000u, 0x3FF00000u}}, {{0xA8935DD9u, 0x3F83DDE2u}},
  {{0x00000000u, 0x00000000u}}, {{0x00000000u, 0x00000000u}}
};

int
__ocl_svml_dtan_cout_rare (const double *a, __private double *r)
{
  int nRet = 0;
  double qv, qvv, v1, v2;
  double x, absx, q;
  double m, mm, m1, mm1, m2, mm2, m3, mm3;
  double z, zz, z1, zz1;
  double c, cc, c1, cc1, c2, cc2;
  double st, rcp, zzhi, y, y1, rm1, rm2, rm3, rhi, rr[2];
  double zm, zs, phi, plo, rlo;
  double poly, reshi, reslo;
  __constant double *pTable;
  int indx, iAbsExp, arg_hibits, NR;

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

          arg_hibits = (((__private _iml_dp_union_t *) & (absx))->hex[1]) >> 16;

          if (arg_hibits >= 0x40c9)
            {

              NR = own_reduce_pio2d ((*a), rr);

              x = rr[0];
            }
          else
            {

              x = (*a);

              rr[1] = ((__constant double *) _vmldTanHATab)[12];

              NR = 0;
            }

          q = ((__constant double *) _vmldTanHATab)[0] * x;

          qv = q + ((__constant double *) _vmldTanHATab)[1];
          m = qv - ((__constant double *) _vmldTanHATab)[1];

          qvv = q + ((__constant double *) _vmldTanHATab)[2];
          mm = qvv - ((__constant double *) _vmldTanHATab)[2];

          indx = (((__private _iml_dp_union_t *) & qv)->dwords.lo_dword);

          indx += (NR << 4);

          indx &= 0x1F;

          indx = indx * 20;

          pTable = &((&(((__constant double *) _vmldTanHATab)[16]))[indx]);

          m1 = m * ((__constant double *) _vmldTanHATab)[6];
          m2 = m * ((__constant double *) _vmldTanHATab)[7];
          m3 = m * ((__constant double *) _vmldTanHATab)[8];
          mm1 = mm * ((__constant double *) _vmldTanHATab)[3];
          mm2 = mm * ((__constant double *) _vmldTanHATab)[4];
          mm3 = mm * ((__constant double *) _vmldTanHATab)[5];

          z1 = x - m1;
          z = z1 - m2;
          c1 = z1 - z;
          c2 = c1 - m2;
          c = c2 - m3;

          zz1 = x - mm1;
          zz = zz1 - mm2;
          cc1 = zz1 - zz;
          cc2 = cc1 - mm2;
          cc = cc2 - mm3;

          c += rr[1];
          cc += rr[1];

          v1 = z + c;
          v2 = z - v1;
          c = v2 + c;
          z = v1;

          v1 = zz + cc;
          v2 = zz - v1;
          cc = v2 + cc;
          zz = v1;

          ((__private _iml_dp_union_t *) & (zzhi))->hex[0] =
            ((__private _iml_dp_union_t *) & (zz))->
            hex[0] & ((__constant _iml_dp_union_t *) &
                      (((__constant double *) _vmldTanHATab)[10]))->hex[0];
          ((__private _iml_dp_union_t *) & (zzhi))->hex[1] =
            ((__private _iml_dp_union_t *) & (zz))->
            hex[1] & ((__constant _iml_dp_union_t *) &
                      (((__constant double *) _vmldTanHATab)[10]))->hex[1];;

          rcp = ((__constant double *) _vmldTanHATab)[11] / zzhi;

          ((__private _iml_dp_union_t *) & (rhi))->hex[0] =
            ((__private _iml_dp_union_t *) & (rcp))->
            hex[0] & ((__constant _iml_dp_union_t *) & (pTable[19]))->hex[0];
          ((__private _iml_dp_union_t *) & (rhi))->hex[1] =
            ((__private _iml_dp_union_t *) & (rcp))->
            hex[1] & ((__constant _iml_dp_union_t *) & (pTable[19]))->hex[1];;

          y1 = zz - zzhi;
          y = y1 + cc;
          rm2 = rcp * y;
          rm1 = zzhi * rhi - ((__constant double *) _vmldTanHATab)[11];
          st = pTable[18] * rcp;
          rm3 = rm1 + rm2;
          rlo = st * rm3;

          poly = z * z * (pTable[3] + z * (pTable[5] + z * (pTable[7] +
                                                            z * (pTable[9] +
                                                                 z *
                                                                 (pTable[11] +
                                                                  z *
                                                                  (pTable[13]
                                                                   +
                                                                   z *
                                                                   (pTable[0]
                                                                    +
                                                                    z *
                                                                    (pTable[1]
                                                                     +
                                                                     z *
                                                                     (pTable
                                                                      [2] +
                                                                      z *
                                                                      (pTable
                                                                       [4] +
                                                                       z *
                                                                       (pTable
                                                                        [6] +
                                                                        z *
                                                                        (pTable
                                                                         [8] +
                                                                         z *
                                                                         (pTable
                                                                          [10]
                                                                          +
                                                                          z *
                                                                          pTable
                                                                          [12])))))))))))));

          zs = z * pTable[16];
          zm = z * pTable[17];
          v1 = zs + zm;
          v2 = zs - v1;
          phi = v1;
          plo = zm + v2;

          plo += poly;

          reshi = pTable[14] - rhi;

          v1 = reshi + phi;
          v2 = reshi - v1;
          reshi = v1;
          reslo = phi + v2;

          v1 = reslo + plo;
          v2 = v1 + rlo;
          v1 = v2 + pTable[17] * c;
          v2 = v1 + pTable[15];
          reslo = v2 + pTable[16] * c;

          (*r) = reshi + reslo;
        }
      else
        {
          if ((*a) != ((__constant double *) _vmldTanHATab)[12])
            {

              (*r) = ((__constant double *) _vmldTanHATab)[14] * (*a) - (*a);
              (*r) = ((__constant double *) _vmldTanHATab)[15] * (*r);
            }
          else
            {

              (*r) = (*a);
            }
        }
    }
  else
    {
      if ((((__private _iml_dp_union_t *) & (absx))->hex[0] ==
           ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldTanHATab)[13]))->
           hex[0])
          && (((__private _iml_dp_union_t *) & (absx))->hex[1] ==
              ((__constant _iml_dp_union_t *) & (((__constant double *) _vmldTanHATab)[13]))->
              hex[1]))
        {

          (*r) = (*a) * ((__constant double *) _vmldTanHATab)[12];

          nRet = 1;
        }
      else
        {

          (*r) = (*a) * (*a);
        }
    }

  return nRet;
}

#endif

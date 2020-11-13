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
// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

#include "include/svml_tanpi_data.cl"
#include "include/svml_gen_rcp.cl"

float1_gen __ocl_svml_px_tanpif1 (float1_gen a);
float1_gen __ocl_svml_px_tanpif1 (float1_gen a)
{
    float va1;
    float vr1;
    float1_gen r;
    VUINT32 vm;

    va1 = a;

    float sX;
    float sN;
    VUINT32 iIndex;
    float sE2;
    float sR;

    float sAbsMask;
    float sC2;

    VUINT32 iIndexPeriodMask;
    float sB_hi;
    float sB_lo;
    float sR_full;
    float sR_tmp;
    float sR_hi;
    float sR_lo;
    float sHalfMask;
    float sOne;
    float sTau;
    float sRecip_hi;
    float sRecip_lo;
    float sEr;
    float dR_RE;
    float dRE;
    float sRecip_ok;
    float dD_E;
    float sD2;
    float sZ2;
    float sH1;
    float sH2;
    float sH3;
    float sH4;
    float sH5;
    float sH6;
    float sH7;
    float sH8;
    float sH9;
    float sC0_hi;
    float sC0_lo;
    float sC1_hi;
    float sC1_lo;
    float sC3;
    float sC4;
    float sP1;
    float sP2;
    float sP3;
    float sP4;
    float sP5;
    float sP6;
    float sP8;
    float sP9;
    float sP10;

    float sExpMask;
    float sEMax;
    float sSpecialMask;
    VUINT32 iSpecialMask;

    float sShifter;
    float sShifterThreshold;
    float sShifterMask;
    float sShifterPos;
    float sZero;
    float sShiftedN;
    float sPi;

    float sSignMask;
    float sSingular;
    float sCotangent;
    float sPi2Multiple;
    float sP17;
    float sInfinity;
    float sSignedInfinity;

    sExpMask     = as_float( 0x7f800000u );
    sX           = as_float( as_uint( va1 ) & as_uint( sExpMask ) );
    sEMax        = as_float( 0x7F800000u );
    sSpecialMask = as_float( (VUINT32) (-(VSINT32) (sX == sEMax)) );
    iSpecialMask = as_uint( sSpecialMask );
    vm           = (((VUINT32) iSpecialMask >> 31) & 1);

    sAbsMask          = as_float( __ocl_svml_stanpi_data._sAbsMask );
    sX                = as_float( as_uint( va1 ) & as_uint( sAbsMask ) );
    sShifterThreshold = as_float( 0x4F000000u );
    sShifterMask      = as_float( (VUINT32) (-(VSINT32) (sX < sShifterThreshold)) );

    sShifterPos = as_float( 0x4FC00000u );
    sShifter    = as_float( as_uint( sShifterMask ) & as_uint( sShifterPos ) );

    sShiftedN = (sShifter + va1);
    sN        = (sShiftedN - sShifter);
    sR        = (va1 - sN);

    sShifter  = as_float( 0x47C00000u );
    sShiftedN = (sShifter + sR);
    sN        = (sShiftedN - sShifter);
    sR        = (sR - sN);

    sSignedInfinity = as_float( (VUINT32) ( as_uint( sShiftedN ) ) << 24 );
    sSignMask       = as_float( 0x80000000u );
    sSignedInfinity = as_float( as_uint( sSignedInfinity ) & as_uint( sSignMask ) );
    sInfinity       = as_float( 0x7F800000u );
    sSignedInfinity = as_float( as_uint( sSignedInfinity ) | as_uint( sInfinity ) );

    iIndex           = as_uint( sShiftedN );
    iIndexPeriodMask = as_uint( 0x0000007Fu );
    iIndex           = (iIndex & iIndexPeriodMask);

    sPi = as_float( 0x40490FDBu );
    sR  = (sR * sPi);

    {
        __constant VUINT32 *_vlt_pPtr_;
        VUINT32 _vlt_nIndex_;

        _vlt_nIndex_ = ((VUINT32) iIndex << 2);
        _vlt_nIndex_ = (_vlt_nIndex_ + iIndex);
        _vlt_nIndex_ = ((VUINT32) _vlt_nIndex_ << 1);

        switch( _vlt_nIndex_ )
        {
        case (0 * 40):
        {
            const int sCoeffs_index = 0;
            sB_hi  = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][0] );
            sB_lo  = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][1] );
            sTau   = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][2] );
            sC0_hi = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][3] );
            sC0_lo = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][4] );
            sC1_hi = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][5] );
            sC1_lo = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][6] );
            sC2    = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][7] );
            sC3    = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][8] );
            sC4    = as_float( __ocl_svml_stanpi_data._sCoeffs[sCoeffs_index][9] );
            break;
        }
        default:
            _vlt_pPtr_   = ((__constant VUINT32 *) (__ocl_svml_stanpi_data._sCoeffs)) + _vlt_nIndex_;

            sB_hi  = ((__constant float *) (_vlt_pPtr_))[0];
            sB_lo  = ((__constant float *) (_vlt_pPtr_))[1];
            sTau   = ((__constant float *) (_vlt_pPtr_))[2];
            sC0_hi = ((__constant float *) (_vlt_pPtr_))[3];
            sC0_lo = ((__constant float *) (_vlt_pPtr_))[4];
            sC1_hi = ((__constant float *) (_vlt_pPtr_))[5];
            sC1_lo = ((__constant float *) (_vlt_pPtr_))[6];
            sC2    = ((__constant float *) (_vlt_pPtr_))[7];
            sC3    = ((__constant float *) (_vlt_pPtr_))[8];
            sC4    = ((__constant float *) (_vlt_pPtr_))[9];
            break;
        }
    }

    sR_full   = (sB_hi - sR);
    sHalfMask = as_float( 0xFFFFF000u );
    sR_hi     = as_float( as_uint( sR_full ) & as_uint( sHalfMask ) );

    sR_lo = (sB_hi - sR_full);
    sR_lo = (sR_lo - sR);

    sR_tmp = (sR_full - sR_hi);
    sR_tmp = (sR_tmp + sB_lo);

    sR_lo = (sR_lo + sR_tmp);

    sZero        = as_float( 0 );
    sPi2Multiple = as_float( (VUINT32) (-(VSINT32) (sR_full == sZero)) );
    sCotangent   = as_float( (VUINT32) (-(VSINT32) (!(sTau == sZero))) );
    sSingular    = as_float( as_uint( sPi2Multiple ) & as_uint( sCotangent ) );

    sRecip_hi = __gen_rcp_1 (sR_hi);
    sRecip_hi = as_float( as_uint( sRecip_hi ) & as_uint( sHalfMask ) );

    sOne = as_float( 0x3F800000u );
    sEr  = (sR_hi * sRecip_hi);
    sEr  = (sOne - sEr);

    dRE       = (sRecip_hi * sEr);
    sE2       = (sEr * sEr);
    dR_RE     = (sRecip_hi + dRE);
    sE2       = (sOne + sE2);
    sRecip_ok = (dR_RE * sE2);

    sR_lo = (sR_lo * sRecip_ok);

    dD_E      = (sR_lo - sEr);
    sD2       = (sR_lo * sR_lo);
    sRecip_lo = (sD2 - dD_E);
    sRecip_lo = (sRecip_lo * sRecip_ok);

    sRecip_hi = (sRecip_hi * sTau);
    sRecip_lo = (sRecip_lo * sTau);

    sH1 = (sC1_hi * sR);
    sH2 = (sC0_hi + sH1);
    sH3 = (sC0_hi - sH2);
    sH4 = (sH2 + sRecip_hi);
    sH5 = (sH3 + sH1);
    sH6 = (sRecip_hi - sH4);
    sH7 = (sH5 + sRecip_lo);
    sH8 = (sH6 + sH2);
    sH9 = (sH7 + sH8);

    sZ2 = (sR * sR);
    sP1 = (sC2 * sR);
    sP2 = (sC4 * sR);
    sP3 = (sC1_lo + sP1);
    sP4 = (sC3 + sP2);
    sP5 = (sZ2 * sP4);
    sP6 = (sP3 + sP5);
    sP8 = (sP6 * sR);
    sP9 = (sC0_lo + sH9);
    sP10 = (sP8 + sP9);
    sP17 = (sH4 + sP10);

    vr1 =  as_float( ( ~as_uint(sSingular) & as_uint(sP17) ) |
                     (  as_uint(sSingular) & as_uint(sSignedInfinity) ) );

    if ((vm & 0x00000001) != 0)
    {
        vr1 = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }

    r = vr1;
    return r;
}

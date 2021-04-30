/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IBiF_VME_VA.cl - VME, VA helper functions               -===//
//
// This file defines helper builtins of OpenCL VME, VA extension functions.
//
//===----------------------------------------------------------------------===//


/*****************************************************************************/
/*                       VME (Vide Motion Estimation)                        */
/*****************************************************************************/

INLINE uint OVERLOADABLE intel_get_accelerator_mb_block_type( sampler_t accelerator )
{
   return __builtin_IB_vme_mb_block_type();
}

INLINE uint OVERLOADABLE intel_get_accelerator_mb_sub_pixel_mode( sampler_t accelerator )
{
   return __builtin_IB_vme_subpixel_mode();
}

INLINE uint OVERLOADABLE intel_get_accelerator_mb_sad_sdjust_mode( sampler_t accelerator )
{
   return __builtin_IB_vme_sad_adjust_mode();
}

INLINE uint OVERLOADABLE intel_get_accelerator_mb_search_path_type( sampler_t accelerator )
{
   return __builtin_IB_vme_search_path_type();
}

#ifdef __IGC_BUILD__
// TODO: Should we include this header somehow?
// These defines should be syncronizes with the values of the following enums
// defined in mainline\Source\USC\state\SamplerTypes.h
// USC::SAMPLER_VME_MB_BLOCK_TYPE
// USC::SAMPLER_VME_SUBPIXEL_MODE
// USC::SAMPLER_VME_SAD_ADJUST_MODE
// USC::SAMPLER_VME_SEARCH_PATH_TYPE

#define SAMPLER_VME_MB_BLOCK_TYPE_16x16     0x0
#define SAMPLER_VME_MB_BLOCK_TYPE_8x8       0x1
#define SAMPLER_VME_MB_BLOCK_TYPE_4x4       0x2

#define SAMPLER_VME_SUBPIXEL_MODE_INTEGER   0x0
#define SAMPLER_VME_SUBPIXEL_MODE_HPEL      0x1
#define SAMPLER_VME_SUBPIXEL_MODE_QPEL      0x2

#define SAMPLER_VME_SAD_ADJUST_MODE_NONE    0x0
#define SAMPLER_VME_SAD_ADJUST_MODE_HAAR    0x1

#define SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_2_2     0x0
#define SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_4_4     0x1
// 0x2 - Reserved for the SMALL search path
// 0x3 - Reserved for the SMALL_DIAMOND search path
// 0x4 - Reserved for the LARGE_DIAMOND search path
#define SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_16_12    0x5

#define EU_VME_SHAPEMASK_4x4            0x3F
#define EU_VME_SHAPEMASK_8x8            0x77
#define EU_VME_SHAPEMASK_16x16          0x7E

#define EU_VME_SADADJUST_MODE_NONE      0
#define EU_VME_SADADJUST_MODE_HAAR      2

#define EU_VME_SUBPEL_MODE_INT_PEL      0x0
#define EU_VME_SUBPEL_MODE_H_PEL        0x1
#define EU_VME_SUBPEL_MODE_Q_PEL        0x3

#define UNIVERSAL_INPUT_MESSAGE_NUM_GRFS 4
#define INPUT_MESSAGE_SIC_NUM_GRFS       4
#define RETURN_MESSAGE_NUM_GRFS          7

#define REF0_SKIP_CENTER_0    0x01
#define REF0_ALL_SKIP_CENTERS 0x55

#define HAAR_TRANSFORM_ADJUSTED 0x2

#define INTRA_COMPUTE_TYPE_DISABLED  0x02
#define INTRA_COMPUTE_TYPE_LUMA_ONLY 0x01

#define NUM_DWORD_IN_GRF 8

enum {
    VME_STREAM_DISABLE = 0,
    VME_STREAM_OUT     = 1,
    VME_STREAM_IN      = 2,
    VME_STREAM_INOUT   = 3
};

enum {
    VME_MAJOR_16x16 = 0,
    VME_MAJOR_16x8  = 1,
    VME_MAJOR_8x16  = 2,
    VME_MAJOR_8x8   = 3
};

void create_universal_input_message(GRFHandle   universalInputMsg,
                                    __private ushort2* ref0Coord,
                                    __private ushort2* ref1Coord,
                                    int2               srcCoord,
                                    int2               refCoord) {

  uint M00 = 0;
  uint M01 = 0;
  uint M02 = __builtin_IB_get_message_phase_dw(universalInputMsg, 0, 2);
  uint M03 = __builtin_IB_get_message_phase_dw(universalInputMsg, 0, 3);
  uint M05 = __builtin_IB_get_message_phase_dw(universalInputMsg, 0, 5);
  uint M12 = __builtin_IB_get_message_phase_dw(universalInputMsg, 1, 2);

  // Setup MaxNumMVs - low byte of m1.1.
  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 1, 1, 0x00000010);

  uint blockType = __builtin_IB_vme_mb_block_type();
  if (blockType == SAMPLER_VME_MB_BLOCK_TYPE_4x4) {
      M03 |= 0x3f000000;
  }
  if (blockType == SAMPLER_VME_MB_BLOCK_TYPE_8x8) {
      M03 |= 0x77000000;
  }
  if (blockType == SAMPLER_VME_MB_BLOCK_TYPE_16x16) {
      M03 |= 0x7e000000;
  }

  uint subpixelMode = __builtin_IB_vme_subpixel_mode();
  if (subpixelMode == SAMPLER_VME_SUBPIXEL_MODE_HPEL) {
      M03 |= 0x00001000;
  }
  if (subpixelMode == SAMPLER_VME_SUBPIXEL_MODE_QPEL) {
      M03 |= 0x00003000;
  }

  uint sadAdjustMode = __builtin_IB_vme_sad_adjust_mode();
  if (sadAdjustMode == SAMPLER_VME_SAD_ADJUST_MODE_HAAR) {
      M03 |= 0x00200000;
  }

  uint searchPathType = __builtin_IB_vme_search_path_type();

  if (searchPathType == SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_2_2) {
      M00 |= 0xfffefffe;
      M01 |= 0xfffefffe;
      M05 |= 0x14140000;
      M12 |= 0x00000101;
  }
  if (searchPathType == SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_4_4) {
      M00 |= 0xfffcfffc;
      M01 |= 0xfffcfffc;
      M05 |= 0x18180000;
      M12 |= 0x00000404;
  }
  if (searchPathType == SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_16_12) {
      M00 |= 0xfff4fff0;
      M01 |= 0xfff4fff0;
      M05 |= 0x28300000;
      M12 |= 0x00003030;
  }

  // Set srcX and srcY in the message payload
  M02 += as_uint(convert_ushort2(srcCoord));

  // Set refX and refY in the message payload
  // refCoords (M0.0, M0.1) will set by the IME send message
  *ref0Coord = as_ushort2(M00) + convert_ushort2(refCoord);
  *ref1Coord = as_ushort2(M01);

  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 0, 2, M02);
  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 0, 3, M03);
  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 0, 5, M05);
  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 1, 2, M12);
}

void create_ime_message(GRFHandle imeMsg) {

  // Program the TINY search path by default
  __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 0, 0x000f1001);

  // If we are NOT the TINY search path, program the DEFAULT search path
  uint searchPathType = __builtin_IB_vme_search_path_type();
  if (searchPathType != SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_4_4) {
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 0, 0x01010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 1, 0x10010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 2, 0x0f0f0f0f);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 3, 0x100f0f0f);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 4, 0x01010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 5, 0x10010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 6, 0x0f0f0f0f);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 0, 7, 0x100f0f0f);

      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 1, 0, 0x01010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 1, 1, 0x10010101);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 1, 2, 0x0f0f0f0f);
      __builtin_IB_set_message_phase_legacy_dw(imeMsg, 1, 3, 0x000f0f0f);
  }
}

void create_fbr_message(GRFHandle universalInputMsg, GRFHandle fbrMsg, GRFHandle imeRes) {

  // Disable BME
  uint M03 = __builtin_IB_get_message_phase_dw(universalInputMsg, 0, 3);
  M03 |= 0x00040000;
  __builtin_IB_set_message_phase_legacy_dw(universalInputMsg, 0, 3, M03);

  // Copy result of IME into FBR header
  //fbrMsg[0] = imeRes[1];
  //fbrMsg[1] = imeRes[2];
  //fbrMsg[2] = imeRes[3];
  //fbrMsg[3] = imeRes[4];
  __builtin_IB_set_message_phase_legacy(fbrMsg, 0, __builtin_IB_get_message_phase(imeRes, 1));
  __builtin_IB_set_message_phase_legacy(fbrMsg, 1, __builtin_IB_get_message_phase(imeRes, 2));
  __builtin_IB_set_message_phase_legacy(fbrMsg, 2, __builtin_IB_get_message_phase(imeRes, 3));
  __builtin_IB_set_message_phase_legacy(fbrMsg, 3, __builtin_IB_get_message_phase(imeRes, 4));

}

#endif // __IGC_BUILD__


INLINE void OVERLOADABLE intel_work_group_vme_mb_query(
    __local uint* dst,
    int2      srcCoord,
    int2      refCoord,
    __read_only image2d_t srcImage,
    __read_only image2d_t refImage,
    sampler_t vmeAccelerator )
{
   GRFHandle res = __builtin_IB_create_message_phases(7);

   ushort2 ref0Coord = 0;
   ushort2 ref1Coord = 0;
   GRFHandle universalInputMsg = __builtin_IB_create_message_phases(4);

   create_universal_input_message(universalInputMsg, &ref0Coord, &ref1Coord, srcCoord, refCoord);

   GRFHandle imeMsg = __builtin_IB_create_message_phases(2);
   create_ime_message(imeMsg);
   GRFHandle zeroCenter = __builtin_IB_create_message_phases(1);
   long srcImageInt = (long) __builtin_astype(srcImage, void*);
   long refImageInt = (long) __builtin_astype(refImage, void*);
   __builtin_IB_vme_send_ime(res, universalInputMsg, imeMsg, srcImageInt, refImageInt, as_uint(ref0Coord), as_uint(ref1Coord), zeroCenter);

   if (__builtin_IB_vme_subpixel_mode() != SAMPLER_VME_SUBPIXEL_MODE_INTEGER) {
       GRFHandle fbrMsg = __builtin_IB_create_message_phases(4);
       create_fbr_message(universalInputMsg, fbrMsg, res);

       uint W00                   = __builtin_IB_get_message_phase_dw(res, 0, 0);
       uint W06                   = __builtin_IB_get_message_phase_dw(res, 0, 6);
       __private uchar* W00Bytes  = (__private uchar*)&W00;
       __private uchar* W06Bytes  = (__private uchar*)&W06;

       uint interMbMode        = W00Bytes[0];
       uint subMbShape         = W06Bytes[1];
       // No support for FBRSubPredModeVar yet, create new zeroed var
       uint subMbPredMode     = 0;
       __builtin_IB_vme_send_fbr(res, universalInputMsg, fbrMsg, srcImageInt, refImageInt, interMbMode, subMbShape, subMbPredMode);
   }

   __local uint* tmpDst = dst + __builtin_IB_simd_lane_id();
   tmpDst[0]     = __builtin_IB_get_message_phase(res, 0);
   tmpDst[8]     = __builtin_IB_get_message_phase(res, 1);
   tmpDst[16]    = __builtin_IB_get_message_phase(res, 2);
   tmpDst[24]    = __builtin_IB_get_message_phase(res, 3);
   tmpDst[32]    = __builtin_IB_get_message_phase(res, 4);
   tmpDst[40]    = __builtin_IB_get_message_phase(res, 5);
   // TODO : do we need this copy??? did not appear in original USC code
   tmpDst[48]    = __builtin_IB_get_message_phase(res, 6);
}

#ifdef __IGC_BUILD__
INLINE void OVERLOADABLE DoMultiQuery(
    __local uint* dstSearch,
    uint countPredMVs,
    uint MVCostPrecision,
    uint2 searchCostTable,
    int2 srcCoord,
    int2 predMV,
    image2d_t srcImg,
    image2d_t refImg,
    sampler_t accelerator,
    unsigned MaxPredMVs) // either 4 or 8
{
    GRFHandle VMEUniversal  = __builtin_IB_create_message_phases(UNIVERSAL_INPUT_MESSAGE_NUM_GRFS);
    // Setup MaxNumMVs - M1.1 (bits 5:0).
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 1, 1, 16 /*MaxNumMVs*/);

    // Controls based on mb_block_type
    int dw3_SubMacroblockSubPartitionMask = 0x00000000;
    switch( __builtin_IB_vme_mb_block_type() )
    {
        case SAMPLER_VME_MB_BLOCK_TYPE_16x16:
            dw3_SubMacroblockSubPartitionMask = EU_VME_SHAPEMASK_16x16;
            break;
        case SAMPLER_VME_MB_BLOCK_TYPE_8x8:
            dw3_SubMacroblockSubPartitionMask = EU_VME_SHAPEMASK_8x8;
            break;
        case SAMPLER_VME_MB_BLOCK_TYPE_4x4:
            dw3_SubMacroblockSubPartitionMask = EU_VME_SHAPEMASK_4x4;
            break;
    }
    // Sub-Macroblock Sub-Partition Mask (SubMbPartMask) - M0.3 (bits 30:24)
    dw3_SubMacroblockSubPartitionMask <<= 24;
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 3, dw3_SubMacroblockSubPartitionMask);

    // Controls based on subpixel mode
    int dw3_SubPelMode = 0x00000000;
    switch( __builtin_IB_vme_subpixel_mode() )
    {
        case SAMPLER_VME_SUBPIXEL_MODE_INTEGER:
            dw3_SubPelMode = EU_VME_SUBPEL_MODE_INT_PEL;
            break;
        case SAMPLER_VME_SUBPIXEL_MODE_HPEL:
            dw3_SubPelMode = EU_VME_SUBPEL_MODE_H_PEL;
            break;
        case SAMPLER_VME_SUBPIXEL_MODE_QPEL:
            dw3_SubPelMode = EU_VME_SUBPEL_MODE_Q_PEL;
            break;
    }
    // Sub-Pel Mode (SubPelMode) - M0.3 (bits 13:12)
    dw3_SubPelMode <<= 12;
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 3, dw3_SubPelMode | __builtin_IB_get_message_phase_dw(VMEUniversal, 0, 3) );

    // Controls based on sad adjust mode
    int dw3_InterSADMeasureAdjustment = 0x00000000;
    switch( __builtin_IB_vme_sad_adjust_mode() )
    {
        case SAMPLER_VME_SAD_ADJUST_MODE_NONE:
            dw3_InterSADMeasureAdjustment = EU_VME_SADADJUST_MODE_NONE;
            break;
        case SAMPLER_VME_SAD_ADJUST_MODE_HAAR:
            dw3_InterSADMeasureAdjustment = EU_VME_SADADJUST_MODE_HAAR;
            break;
    }
    // Inter SAD Measure Adjustment (InterSAD) - M0.3 (bits 21:20)
    dw3_InterSADMeasureAdjustment <<= 20;
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 3, dw3_InterSADMeasureAdjustment | __builtin_IB_get_message_phase_dw(VMEUniversal, 0, 3) );

    // Controls based on search path type
    int dw01_refOffset = 0x00000000;
    int dw05_refWindow = 0x00000000;
    int dw12_MaxFixedSearchPathLength_MaximumSearchPathLength = 0x00000000;
    switch( __builtin_IB_vme_search_path_type() )
    {
        case SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_2_2:
            dw01_refOffset = 0xFFFEFFFE; // -2, -2
            dw05_refWindow = 0x14140000; // 20, 20
            dw12_MaxFixedSearchPathLength_MaximumSearchPathLength = 0x00000101;
            break;
        case SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_4_4:
            dw01_refOffset = 0xFFFCFFFC; // -4, -4
            dw05_refWindow = 0x18180000; // 24, 24
            dw12_MaxFixedSearchPathLength_MaximumSearchPathLength = 0x00000404;
            break;
        case SAMPLER_VME_SEARCH_PATH_TYPE_RADIUS_16_12:
            dw01_refOffset = 0xFFF4FFF0; // -16, -12
            dw05_refWindow = 0x28300000; // 48, 40
            dw12_MaxFixedSearchPathLength_MaximumSearchPathLength = 0x00003030;
            break;
    }
    // Ref0 and Ref1 will be setup below for every IME send.
    // Reference Region Height (RefHeight) - M0.5 (bits 31:24)
    // Reference Region Width (RefWidth)   - M0.5 (bits 23:16)
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 5, dw05_refWindow );
    // Maximum Search Path Length (MaxNumSU) - M1.2 (bits 15:8)
    // Max Fixed Search Path Length (LenSP)  - M1.2 (bits 7:0)
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 1, 2, dw12_MaxFixedSearchPathLength_MaximumSearchPathLength );

    // Set srcX and srcY in the message payload
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 2, as_uint(convert_ushort2(srcCoord)));
    // Set the cost precision DW
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 1, 7, MVCostPrecision | __builtin_IB_get_message_phase_dw(VMEUniversal, 1, 7));
    // Set the cost penalty table DWs.
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 2, 3, searchCostTable.x);
    __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 2, 4, searchCostTable.y);

    // Get the cost center for the ith IME/FBR from the 0th SIMD channel
    // of predMVs (the 1st predictor)
    const ushort2 CostCoord = as_ushort2(sub_group_broadcast(as_uint(convert_short2(predMV)), 0));
    // Set CostX and CostY in the message payload
    // The cost center need to be specified in QPEL precision
    // and so we need to perform PEL to QPEL conversion here
    const uint CostCenter = as_uint(CostCoord << (ushort2)2);

    //Build the VME Search Path
    GRFHandle VMESearchPath = __builtin_IB_create_message_phases(2);
    create_ime_message(VMESearchPath);

    // Do a runtime check to see if we need to issue the ith IME.
    // Get the predictor for the ith IME from the ith SIMD channel
    // of predMVs.
    GRFHandle MVMin  = __builtin_IB_create_message_phases_no_init(4);
    GRFHandle SADMin = __builtin_IB_create_message_phases_no_init(1);
    GRFHandle result = __builtin_IB_create_message_phases(RETURN_MESSAGE_NUM_GRFS);
    long srcImgInt = (long)__builtin_astype(srcImg, void*);
    long refImgInt = (long)__builtin_astype(refImg, void*);

    // i = 0
    {
        ushort2 ref0Coord = CostCoord + as_ushort2(dw01_refOffset);
        __builtin_IB_vme_send_ime(result, VMEUniversal, VMESearchPath, srcImgInt, refImgInt, as_uint(ref0Coord), dw01_refOffset, CostCenter);
        __builtin_IB_extract_mv_and_sad(MVMin, SADMin, result, __builtin_IB_vme_mb_block_type());
    }
    for (int i=1; i < MaxPredMVs; i++)
    {
        if (i < countPredMVs)
        {
            ushort2 vRefCoord = as_ushort2(sub_group_broadcast(as_uint(convert_short2(predMV)), i));
            ushort2 ref0Coord = vRefCoord + as_ushort2(dw01_refOffset);

            __builtin_IB_vme_send_ime(result, VMEUniversal, VMESearchPath, srcImgInt, refImgInt, as_uint(ref0Coord), dw01_refOffset, CostCenter);

            GRFHandle MVCurr  = __builtin_IB_create_message_phases_no_init(4);
            GRFHandle SADCurr = __builtin_IB_create_message_phases_no_init(1);
            __builtin_IB_extract_mv_and_sad(MVCurr, SADCurr, result, __builtin_IB_vme_mb_block_type());
            __builtin_IB_cmp_sads(MVCurr, SADCurr, MVMin, SADMin);
        }
    }

    // Set up the check to see if we need to do an FBR
    if( __builtin_IB_vme_subpixel_mode() != SAMPLER_VME_SUBPIXEL_MODE_INTEGER )
    {
        // Disable BME
        __builtin_IB_set_message_phase_legacy_dw(VMEUniversal, 0, 3, 0x00040000 | __builtin_IB_get_message_phase_dw(VMEUniversal, 0, 3));

        uint W00                   = __builtin_IB_get_message_phase_dw(result, 0, 0);
        uint W06                   = __builtin_IB_get_message_phase_dw(result, 0, 6);
        __private uchar* W00Bytes  = (__private uchar*)&W00;
        __private uchar* W06Bytes  = (__private uchar*)&W06;

        uint interMbMode        = W00Bytes[0];
        uint subMbShape         = W06Bytes[1];
        // No support for FBRSubPredModeVar yet, create new zeroed var
        uint subMbPredMode      = 0;
        __builtin_IB_vme_send_fbr(result, VMEUniversal, MVMin, srcImgInt, refImgInt, interMbMode, subMbShape, subMbPredMode);
    }
    else
    {
        __builtin_IB_set_message_phase_legacy(result, 1, __builtin_IB_get_message_phase(MVMin, 0));
        __builtin_IB_set_message_phase_legacy(result, 2, __builtin_IB_get_message_phase(MVMin, 1));
        __builtin_IB_set_message_phase_legacy(result, 3, __builtin_IB_get_message_phase(MVMin, 2));
        __builtin_IB_set_message_phase_legacy(result, 4, __builtin_IB_get_message_phase(MVMin, 3));

        __builtin_IB_set_message_phase_legacy(result, 5, __builtin_IB_get_message_phase(SADMin, 0));
    }

    __local uint* tmpDst = dstSearch + __builtin_IB_simd_lane_id();
    tmpDst[8]  = __builtin_IB_get_message_phase(result, 1);
    tmpDst[16] = __builtin_IB_get_message_phase(result, 2);
    tmpDst[24] = __builtin_IB_get_message_phase(result, 3);
    tmpDst[32] = __builtin_IB_get_message_phase(result, 4);
    tmpDst[40] = __builtin_IB_get_message_phase(result, 5);
}
#endif // __IGC_BUILD__

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_query_8(
    __local uint* dstSearch,
    uint countPredMVs,
    uint MVCostPrecision,
    uint2 searchCostTable,
    int2 srcCoord,
    int2 predMV,
    read_only image2d_t srcImg,
    read_only image2d_t refImg,
    sampler_t accelerator )
{
    DoMultiQuery(
        dstSearch,
        countPredMVs,
        MVCostPrecision,
        searchCostTable,
        srcCoord,
        predMV,
        srcImg,
        refImg,
        accelerator,
        8);
}

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_query_4(
    __local uint* dstSearch,
    uint countPredMVs,
    uint MVCostPrecision,
    uint2 searchCostTable,
    int2 srcCoord,
    int2 predMV,
    read_only image2d_t srcImg,
    read_only image2d_t refImg,
    sampler_t accelerator )
{
    DoMultiQuery(
        dstSearch,
        countPredMVs,
        MVCostPrecision,
        searchCostTable,
        srcCoord,
        predMV,
        srcImg,
        refImg,
        accelerator,
        4);
}

#ifdef __IGC_BUILD__
static GRFHandle create_universal_input_sic_message(bool is16x16, ushort edgeMask, int2 srcCoord)
{
    GRFHandle msg = __builtin_IB_create_message_phases(UNIVERSAL_INPUT_MESSAGE_NUM_GRFS);

    // Set the skip block type (M0.3, bit 14).
    __builtin_IB_set_message_phase_legacy_ub(msg, 0, 3*4 + 1, ((uchar)(!is16x16)) << 6);

    // Set the SAD adjustment mode - M0.3 (23:20).
    {
        uint mode = __builtin_IB_vme_sad_adjust_mode();

        if (mode == SAMPLER_VME_SAD_ADJUST_MODE_HAAR)
        {
            // Intra and Inter SAD Measure Adjustment
            uchar val = ((HAAR_TRANSFORM_ADJUSTED << 2) | HAAR_TRANSFORM_ADJUSTED) << 4;
            __builtin_IB_set_message_phase_legacy_ub(msg, 0, 3*4 + 2, val);
        }
    }

    // Set the MBIntraStruct - set the edge mask. (M1.7 - 15:8).
    {
        // the calling kernel already shifts this up 8 bits.
        __builtin_IB_set_message_phase_legacy_uw(msg, 1, 7*2 + 0, edgeMask);
    }

    // Set srcX in the message payload - M0.2 (low word).
    // Set srcY in the message payload - M0.2 (high word).
    __builtin_IB_set_message_phase_legacy_dw(msg, 0, 2, as_uint(convert_ushort2(srcCoord)));

    return msg;
}

static INLINE GRFHandle ReadPixels(read_only image2d_t intraSrcImage, int2 coord, int width, int height)
{
    // allocate a block of 64 bytes to store the read data into.
    long intraSrcImageInt = (long)__builtin_astype(intraSrcImage, void*);
    GRFHandle dst = __builtin_IB_create_message_phases((width * height) / (NUM_DWORD_IN_GRF * 4));
    __builtin_IB_media_block_rectangle_read(intraSrcImageInt, coord, width, height, dst);

    return dst;
}

static INLINE void SetSkipMVs(
    bool is16x16,
    bool isBidir,
    int  skipMV1,
    int4 skipMV4,
    int  skipMV1Bidir,
    int2 skipMV2Bidir,
    int  i,
    GRFHandle sicInput)
{
    // The 16x16 case has only one skip MV per set.
    if (is16x16)
    {
        if (isBidir)
        {
            __builtin_IB_simdMediaRegionCopy(sicInput, 0, 1, 2, skipMV1Bidir, 8*i, 2, 2, 1, 4, 2, 16);
        }
        else
        {
            int skip = intel_sub_group_shuffle(skipMV1, i);
            __builtin_IB_set_message_phase_legacy_dw(sicInput, 0, 0, skip);
        }
    }
    // 8x8 has 4 skip MVs per set.
    else
    {
        if (isBidir)
        {
            // For the bidir case, skip MVs are loaded into registers in the manner that
            // hardware expects it so, instead of shuffling, we just directly do a mov to
            // push the skip MVs into the SIC payload.
            if (i < 2)
            {
                __builtin_IB_simdMediaRegionCopy(sicInput, 0, 1, 8, skipMV2Bidir.x, 8*4*i, 8, 8, 1, 4, 8, 16);
            }
            else
            {
                __builtin_IB_simdMediaRegionCopy(sicInput, 0, 1, 8, skipMV2Bidir.y, 8*4*(i-2), 8, 8, 1, 4, 8, 16);
            }
        }
        else
        {
            int4 skip = intel_sub_group_shuffle(skipMV4, i);
            __builtin_IB_set_message_phase_legacy_dw(sicInput, 0, 0, skip.x);
            __builtin_IB_set_message_phase_legacy_dw(sicInput, 0, 2, skip.y);
            __builtin_IB_set_message_phase_legacy_dw(sicInput, 0, 4, skip.z);
            __builtin_IB_set_message_phase_legacy_dw(sicInput, 0, 6, skip.w);
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// This is the core implementation for skip checks.  8x8 and 16x16 call
// into this function with small variations.
//
// All arguments here are uniform except for skipMV and skipMVs.
//
static INLINE void DoMultiCheck(
    bool is16x16,
    bool isBidir,
    // The bottom 8 GRF are loaded with distortion values and
    // the top 3 GRF are for intra prediction modes (16x16, 8x8, 4x4).
    __local uint* dstSkipIntra, // __local uint[64 + 24];
    // How many skip MV sets are coming in.  We key off of this
    // to determine how many CRE sends to do.
    uint countSkipMVs,
    // Has the user requested intra prediction?
    uint doIntra,
    // The edge mask.  The calling kernel sets this up and will mask
    // off reading from neighbor macroblocks when we're on the edges
    // of the image.
    uint intraEdges,
    // Location of the current macroblock.
    int2 srcCoord,

    uchar bidir_weight,
    uchar skipMode,

    int  skipMV1,      // short2, used by intel_work_group_vme_mb_multi_check_16x16()
    int4 skipMV4,      // 4 short2s, used by intel_work_group_vme_mb_multi_check_8x8()
    int  skipMV1Bidir, // used by intel_work_group_vme_mb_multi_bidir_check_16x16(), laid out for hardware
    int2 skipMV2Bidir, // used by intel_work_group_vme_mb_multi_bidir_check_8x8(), laid out for hardware

    // The source and reference frames from the video.
    read_only image2d_t srcImage,
    read_only image2d_t refImage0,
    read_only image2d_t refImage1,

    // This is just an alias to srcImage.  It is set up with render surface state
    // so that we can do media block reads from it.
    read_only image2d_t intraSrcImage,

    uint MaxSkipMVs)
{
    GRFHandle universalMsg = create_universal_input_sic_message(is16x16, (ushort)intraEdges, srcCoord);

    GRFHandle sicInput     = __builtin_IB_create_message_phases(INPUT_MESSAGE_SIC_NUM_GRFS);

    GRFHandle result       = __builtin_IB_create_message_phases(RETURN_MESSAGE_NUM_GRFS);

    long srcImageInt = (long)__builtin_astype(srcImage, void*);
    long refImage0Int = (long)__builtin_astype(refImage0, void*);
    long refImage1Int = (long)__builtin_astype(refImage1, void*);

    // Set the Intra Compute Type (M1.1, bits 9:8).
    // Initially disable and turn it on if we need to do intra prediction.
    __builtin_IB_set_message_phase_legacy_ub(sicInput, 1, 1*4 + 1, INTRA_COMPUTE_TYPE_DISABLED);

    if (isBidir)
    {
        // Set the Bidirectional Weight (BiWeight) (M1.1, bits 21:16).
        __builtin_IB_set_message_phase_legacy_ub(universalMsg, 1, 1*4 + 2, bidir_weight);

        uchar val = (is16x16 ? skipMode & 0x3 : skipMode);
        // Set the skip block mask (M1.7 - 31:24).
        __builtin_IB_set_message_phase_legacy_ub(universalMsg, 1, 7*4 + 3, val);
    }
    else
    {
        uchar val = (is16x16 ? REF0_SKIP_CENTER_0 : REF0_ALL_SKIP_CENTERS);
        // Set the skip block mask (M1.7 - 31:24).
        __builtin_IB_set_message_phase_legacy_ub(universalMsg, 1, 7*4 + 3, val);
    }

    if (doIntra)
    {
        __builtin_IB_set_message_phase_legacy_ub(sicInput, 1, 1*4 + 1, INTRA_COMPUTE_TYPE_LUMA_ONLY);

        // Each block is 16x16 pixels
        // +--------+--------+--------+
        // |        |        |        |
        // |   D    |   B    |   C    |
        // |        |        |        |
        // +--------------------------+
        // |        |  Curr  |
        // |   A    |   E    |
        // |        |        |
        // +--------+--------+

        // Read pixels from the top three neighbors of the current macroblock.  Reading:
        //   Bottom right corner pixel of D
        // + All 16 bottom edge pixels of B
        // + First 8 bottom edge pixels of C
        // = 25 pixels
        // Src coord is offset -4 in the x so the read remains dword aligned
        GRFHandle topEdgePixels  = ReadPixels(intraSrcImage, srcCoord + (int2)(-4, -1), /*width*/ 32, /*height*/ 2);
        // The 16 pixels on the right edge of A (aka the left neighbor of E) are read.
        GRFHandle leftEdgePixels = ReadPixels(intraSrcImage, srcCoord + (int2)(-4, 0),  /*width*/ 4,  /*height*/ 16);

        // load left edge pixels.
        // shift three bytes over to align on the right edge of the block load then stride 4 across
        // to read each pixel down the edge.
        // mov (8) r64.0<1>:ub r10.3<32;8,4>:ub {Align1, Q1, NoMask} // #??:$193:%211
        __builtin_IB_simdMediaRegionCopy(sicInput, 64, 1, 8, leftEdgePixels, 3, 32, 8, 4, 1, 8, 64-3);
        // same as above but read the last 8 pixels down the right edge.
        // mov (8) r64.8<1>:ub r11.3<32;8,4>:ub {Align1, Q1, NoMask} // #??:$194:%212
        __builtin_IB_simdMediaRegionCopy(sicInput, 64+8, 1, 8, leftEdgePixels, 3+32, 32, 8, 4, 1, 8, 64-3-32);

        // load top edge pixels.
        // load the first 4 dwords (16 pixels) from the top edge.
        // mov (4) r63.2<1>:ud r8.1<4;4,1>:ud {Align1, Q1, NoMask} // #??:$195:%213
        __builtin_IB_simdMediaRegionCopy(sicInput, 32+8, 1, 4, topEdgePixels, 4, 4, 4, 1, 4, 4, 4);
        // load last 2 dwords (last 8 pixels).
        // mov (2) r63.6<1>:ud r8.5<2;2,1>:ud {Align1, Q1, NoMask} // #??:$196:%214
        __builtin_IB_simdMediaRegionCopy(sicInput, 32+8+16, 1, 2, topEdgePixels, 4+16, 2, 2, 1, 4, 2, 2);
        // load left corner pixel.
        // mov (1) r63.7<2>:ub r8.3<0;1,0>:ub {Align1, Q1, NoMask} // #??:$197:%215
        __builtin_IB_simdMediaRegionCopy(sicInput, 32+7, 1, 1, topEdgePixels, 3, 0, 1, 0, 1, 1, 1);
    }

    local uint *pDistortionBase = dstSkipIntra;
    local uint *pIntraBase      = &dstSkipIntra[NUM_DWORD_IN_GRF * MaxSkipMVs];

    // We will do a either 3 intra operations (16x16, 8x8, 4x4) or none.  This is why
    // the 8 iterations over the skip MV sets are split into a loop of 3 and a loop of 5.
    // The hardware is capable of doing skip and intra simultaneously.
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
    __attribute__((opencl_unroll_hint(8)))
#endif
    for (int i=0; i < 3; i++)
    {
        bool doSkip = (i < countSkipMVs);

        if (doSkip | (bool)doIntra)
        {
            {
                uchar IntraLumaPartMask = 0x7; // none
                switch (i)
                {
                    case 0: IntraLumaPartMask = 0x6; break; // 16x16 intra
                    case 1: IntraLumaPartMask = 0x5; break; //   8x8 intra
                    case 2: IntraLumaPartMask = 0x3; break; //   4x4 intra
                }

                // Luma Intra Partition Mask (IntraPartMask) (M1.7 - 4:0)
                __builtin_IB_set_message_phase_legacy_ub(universalMsg, 1, 7*4 + 0, IntraLumaPartMask);
            }

            // Set the skip mode enable bit (M1.0 - 0:0).
            __builtin_IB_set_message_phase_legacy_ub(universalMsg, 1, 0*4 + 0, (uchar)doSkip);

            SetSkipMVs(
                is16x16,
                isBidir,
                skipMV1,
                skipMV4,
                skipMV1Bidir,
                skipMV2Bidir,
                i,
                sicInput);

            // combine the universal and sic inputs into one big 8 GRF payload and fire
            // off to the CRE.  Result will contain information about distortion values
            // and intra prediction modes.
            __builtin_IB_vme_send_sic(result, universalMsg, sicInput, srcImageInt, refImage0Int, refImage1Int);

            // load results into SLM

            // TODO: I'd like to emit a send (8) here instead of having to
            // shut off the top channels in simd16 since we're only writing
            // one GRF.
            if (get_sub_group_local_id() < NUM_DWORD_IN_GRF)
            {
                // Store W5.0 - 8 DWORDs (1 reg) containing distortion values.
                pDistortionBase[NUM_DWORD_IN_GRF * i + get_sub_group_local_id()] =
                    __builtin_IB_get_message_phase(result, 5);
                // Store W0.0 - 8 DWORDs (1 reg) containing intra prediction modes.
                pIntraBase[NUM_DWORD_IN_GRF * i + get_sub_group_local_id()] =
                    __builtin_IB_get_message_phase(result, 0);
            }
        }
    }

    // We've read our three intra prediction modes.  Turn off intra prediction.
    __builtin_IB_set_message_phase_legacy_ub(sicInput, 1, 1*4 + 1, INTRA_COMPUTE_TYPE_DISABLED);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
    __attribute__((opencl_unroll_hint(8)))
#endif
    for (int i=3; i < MaxSkipMVs; i++)
    {
        if (i < countSkipMVs)
        {
            SetSkipMVs(
                is16x16,
                isBidir,
                skipMV1,
                skipMV4,
                skipMV1Bidir,
                skipMV2Bidir,
                i,
                sicInput);

            __builtin_IB_vme_send_sic(result, universalMsg, sicInput, srcImageInt, refImage0Int, refImage1Int);

            // load results into SLM
            // Store W5.0 - 8 DWORDs (1 reg) containing distortion values.
            if (get_sub_group_local_id() < NUM_DWORD_IN_GRF)
            {
                pDistortionBase[NUM_DWORD_IN_GRF * i + get_sub_group_local_id()] =
                    __builtin_IB_get_message_phase(result, 5);
            }
        }
    }
}
#endif // __IGC_BUILD__

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_check_16x16(
    __local uint* dstSkipIntra, // __local uint[64 + 24];
    uint countSkipMVs,
    uint doIntra,
    uint intraEdges,
    int2 srcCoord,
    int  skipMV, // short2
    read_only image2d_t srcImage,
    read_only image2d_t refImage,
    read_only image2d_t intraSrcImage,
    sampler_t vmeAccelerator )
{
    DoMultiCheck(
        true, // is16x16
        false, // bidir
        dstSkipIntra,
        countSkipMVs,
        doIntra,
        intraEdges,
        srcCoord,
        0, // unused
        0, // unused
        skipMV,
        0,0,0, // unused
        srcImage,
        refImage,
        refImage,
        intraSrcImage,
        8);
}

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_check_8x8(
    __local uint* dstSkipIntra, // __local uint[64 + 24];
    uint countSkipMVs,
    uint doIntra,
    uint intraEdges,
    int2 srcCoord,
    int4 skipMVs, // 4 short2
    read_only image2d_t srcImage,
    read_only image2d_t refImage,
    read_only image2d_t intraSrcImage,
    sampler_t vmeAccelerator )
{
    DoMultiCheck(
        false, // is16x16
        false, // bidir
        dstSkipIntra,
        countSkipMVs,
        doIntra,
        intraEdges,
        srcCoord,
        0, 0, // unused
        0,
        skipMVs,
        0,0,
        srcImage,
        refImage,
        refImage,
        intraSrcImage,
        8);
}

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_bidir_check_16x16(
    __local uint* dstSkipIntra,
    uint countSkipMVs,
    uint doIntra,
    uint intraEdges,
    int2 srcCoord,
    uchar bidirWeight,
    uchar skipModes,
    int  skipMV,   // short2
    read_only image2d_t srcImage,
    read_only image2d_t refFwdImage,
    read_only image2d_t refBwdImage,
    read_only image2d_t intraSrcImage,
    sampler_t vmeAccelerator )
{
    DoMultiCheck(
        true, // is16x16
        true, // bidir
        dstSkipIntra,
        countSkipMVs,
        doIntra,
        intraEdges,
        srcCoord,
        bidirWeight,
        skipModes,
        0,0,
        skipMV,
        0, // unused
        srcImage,
        refFwdImage,
        refBwdImage,
        intraSrcImage,
        4);
}

INLINE void OVERLOADABLE intel_work_group_vme_mb_multi_bidir_check_8x8(
    __local uint* dstSkipIntra,
    uint countSkipMVs,
    uint doIntra,
    uint intraEdges,
    int2 srcCoord,
    uchar bidirWeight,
    uchar skipModes,
    int2 skipMVs,    // 4 short2
    read_only image2d_t srcImage,
    read_only image2d_t refFwdImage,
    read_only image2d_t refBwdImage,
    read_only image2d_t intraSrcImage,
    sampler_t vmeAccelerator )
{
    DoMultiCheck(
        false, // is16x16
        true, // bidir
        dstSkipIntra,
        countSkipMVs,
        doIntra,
        intraEdges,
        srcCoord,
        bidirWeight,
        skipModes,
        0,0,0,
        skipMVs,
        srcImage,
        refFwdImage,
        refBwdImage,
        intraSrcImage,
        4);
}

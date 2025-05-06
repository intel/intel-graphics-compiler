/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//reflect_101
#if defined (cl_intel_mirrored_repeat_101)
#define CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL 0xA
#endif
//
// Float Atomics (SKL feature)
//
#if defined (float_atomics_enable)

#ifdef __opencl_c_generic_address_space
float __attribute__((overloadable)) atomic_fetch_min(volatile generic atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_float *object, float operand, memory_order order, memory_scope scope);

float __attribute__((overloadable)) atomic_fetch_max(volatile generic atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_float *object, float operand, memory_order order, memory_scope scope);
#endif // __opencl_c_generic_address_space

// atom_min
float __attribute__((overloadable)) atom_min(volatile __global float *p, float val);
float __attribute__((overloadable)) atom_min(volatile __local float *p, float val);

float __attribute__((overloadable)) atomic_min(volatile __global float *p, float val);
float __attribute__((overloadable)) atomic_min(volatile __local float *p, float val);

// atom_max
float __attribute__((overloadable)) atom_max(volatile __global float *p, float val);
float __attribute__((overloadable)) atom_max(volatile __local float *p, float val);

float __attribute__((overloadable)) atomic_max(volatile __global float *p, float val);
float __attribute__((overloadable)) atomic_max(volatile __local float *p, float val);

// atom_cmpxchg
float __attribute__((overloadable)) atom_cmpxchg(volatile __global float *p, float cmp, float val);
float __attribute__((overloadable)) atom_cmpxchg(volatile __local float *p, float cmp, float val);

float __attribute__((overloadable)) atomic_cmpxchg(volatile __global float *p, float cmp, float val);
float __attribute__((overloadable)) atomic_cmpxchg(volatile __local float *p, float cmp, float val);

// +atomic_fetch_min/max handled elsewhere
// +atomic_compare_exchange_* float variant already defined in an OCL 2.0 specification.
#endif


// Planar YUV.
#define cl_intel_planar_yuv 0x1
#define CLK_NV12_INTEL 0x410E

// Packed YUV
#define cl_intel_packed_yuv 0x1
#define CLK_YUYV_INTEL 0x4076
#define CLK_UYVY_INTEL 0x4077
#define CLK_YVYU_INTEL 0x4078
#define CLK_VYUY_INTEL 0x4079

#if defined(cl_intel_device_side_avc_motion_estimation)
// Device side VME not defined in Clang.
#define CLK_AVC_ME_SEARCH_WINDOW_16x12_RADIUS_INTEL        0x9
#define CLK_AVC_ME_SEARCH_WINDOW_4x4_RADIUS_INTEL          0x2
#define CLK_AVC_ME_SEARCH_WINDOW_2x2_RADIUS_INTEL          0xa

#ifndef __VME_TYPES_DEFINED__
// Externally exposed device side VME.

// ... Defines ...

#define CLK_AVC_ME_MAJOR_16x16_INTEL                       0x0
#define CLK_AVC_ME_MAJOR_16x8_INTEL                        0x1
#define CLK_AVC_ME_MAJOR_8x16_INTEL                        0x2
#define CLK_AVC_ME_MAJOR_8x8_INTEL                         0x3

#define CLK_AVC_ME_MINOR_8x8_INTEL                         0x0
#define CLK_AVC_ME_MINOR_8x4_INTEL                         0x1
#define CLK_AVC_ME_MINOR_4x8_INTEL                         0x2
#define CLK_AVC_ME_MINOR_4x4_INTEL                         0x3

#define CLK_AVC_ME_MAJOR_FORWARD_INTEL                     0x0
#define CLK_AVC_ME_MAJOR_BACKWARD_INTEL                    0x1
#define CLK_AVC_ME_MAJOR_BIDIRECTIONAL_INTEL               0x2

#define CLK_AVC_ME_PARTITION_MASK_ALL_INTEL                0x0
#define CLK_AVC_ME_PARTITION_MASK_16x16_INTEL              0x7E
#define CLK_AVC_ME_PARTITION_MASK_16x8_INTEL               0x7D
#define CLK_AVC_ME_PARTITION_MASK_8x16_INTEL               0x7B
#define CLK_AVC_ME_PARTITION_MASK_8x8_INTEL                0x77
#define CLK_AVC_ME_PARTITION_MASK_8x4_INTEL                0x6F
#define CLK_AVC_ME_PARTITION_MASK_4x8_INTEL                0x5F
#define CLK_AVC_ME_PARTITION_MASK_4x4_INTEL                0x3F

#define CLK_AVC_ME_SEARCH_WINDOW_EXHAUSTIVE_INTEL          0x0
#define CLK_AVC_ME_SEARCH_WINDOW_SMALL_INTEL               0x1
#define CLK_AVC_ME_SEARCH_WINDOW_TINY_INTEL                0x2
#define CLK_AVC_ME_SEARCH_WINDOW_EXTRA_TINY_INTEL          0x3
#define CLK_AVC_ME_SEARCH_WINDOW_DIAMOND_INTEL             0x4
#define CLK_AVC_ME_SEARCH_WINDOW_LARGE_DIAMOND_INTEL       0x5
#define CLK_AVC_ME_SEARCH_WINDOW_RESERVED0_INTEL           0x6
#define CLK_AVC_ME_SEARCH_WINDOW_RESERVED1_INTEL           0x7
#define CLK_AVC_ME_SEARCH_WINDOW_CUSTOM_INTEL              0x8

#define CLK_AVC_ME_SAD_ADJUST_MODE_NONE_INTEL              0x0
#define CLK_AVC_ME_SAD_ADJUST_MODE_HAAR_INTEL              0x2

#define CLK_AVC_ME_SUBPIXEL_MODE_INTEGER_INTEL             0x0
#define CLK_AVC_ME_SUBPIXEL_MODE_HPEL_INTEL                0x1
#define CLK_AVC_ME_SUBPIXEL_MODE_QPEL_INTEL                0x3

#define CLK_AVC_ME_COST_PRECISION_QPEL_INTEL               0x0
#define CLK_AVC_ME_COST_PRECISION_HPEL_INTEL               0x1
#define CLK_AVC_ME_COST_PRECISION_PEL_INTEL                0x2
#define CLK_AVC_ME_COST_PRECISION_DPEL_INTEL               0x3

#define CLK_AVC_ME_BIDIR_WEIGHT_QUARTER_INTEL                      0x10
#define CLK_AVC_ME_BIDIR_WEIGHT_THIRD_INTEL                           0x15
#define CLK_AVC_ME_BIDIR_WEIGHT_HALF_INTEL                         0x20
#define CLK_AVC_ME_BIDIR_WEIGHT_TWO_THIRD_INTEL                    0x2B
#define CLK_AVC_ME_BIDIR_WEIGHT_THREE_QUARTER_INTEL                0x30

#define CLK_AVC_ME_BORDER_REACHED_LEFT_INTEL                       0x0
#define CLK_AVC_ME_BORDER_REACHED_RIGHT_INTEL                      0x2
#define CLK_AVC_ME_BORDER_REACHED_TOP_INTEL                        0x4
#define CLK_AVC_ME_BORDER_REACHED_BOTTOM_INTEL                     0x8

#define CLK_AVC_ME_SKIP_BLOCK_PARTITION_16x16_INTEL                0x0
#define CLK_AVC_ME_SKIP_BLOCK_PARTITION_8x8_INTEL                  0x4000

#define CLK_AVC_ME_SKIP_BLOCK_16x16_FORWARD_ENABLE_INTEL           ( 0x1 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_16x16_BACKWARD_ENABLE_INTEL          ( 0x2 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_16x16_DUAL_ENABLE_INTEL              ( 0x3 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_FORWARD_ENABLE_INTEL             ( 0x55 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_BACKWARD_ENABLE_INTEL            ( 0xAA << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_DUAL_ENABLE_INTEL                ( 0xFF << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_0_FORWARD_ENABLE_INTEL           ( 0x1 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_0_BACKWARD_ENABLE_INTEL          ( 0x2 << 24 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_1_FORWARD_ENABLE_INTEL           ( 0x1 << 26 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_1_BACKWARD_ENABLE_INTEL          ( 0x2 << 26 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_2_FORWARD_ENABLE_INTEL           ( 0x1 << 28 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_2_BACKWARD_ENABLE_INTEL          ( 0x2 << 28 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_3_FORWARD_ENABLE_INTEL           ( 0x1 << 30 )
#define CLK_AVC_ME_SKIP_BLOCK_8x8_3_BACKWARD_ENABLE_INTEL          ( 0x2 << 30 )

#define CLK_AVC_ME_BLOCK_BASED_SKIP_4x4_INTEL                      0x00
#define CLK_AVC_ME_BLOCK_BASED_SKIP_8x8_INTEL                      0x80

#define CLK_AVC_ME_INTRA_16x16_INTEL                               0x0
#define CLK_AVC_ME_INTRA_8x8_INTEL                                 0x1
#define CLK_AVC_ME_INTRA_4x4_INTEL                                 0x2

#define CLK_AVC_ME_INTRA_LUMA_PARTITION_MASK_ALL_INTEL             0x0
#define CLK_AVC_ME_INTRA_LUMA_PARTITION_MASK_16x16_INTEL           0x6
#define CLK_AVC_ME_INTRA_LUMA_PARTITION_MASK_8x8_INTEL             0x5
#define CLK_AVC_ME_INTRA_LUMA_PARTITION_MASK_4x4_INTEL             0x3

#define CLK_AVC_ME_INTRA_NEIGHBOR_LEFT_MASK_ENABLE_INTEL           0x60
#define CLK_AVC_ME_INTRA_NEIGHBOR_UPPER_MASK_ENABLE_INTEL          0x10
#define CLK_AVC_ME_INTRA_NEIGHBOR_UPPER_RIGHT_MASK_ENABLE_INTEL    0x8
#define CLK_AVC_ME_INTRA_NEIGHBOR_UPPER_LEFT_MASK_ENABLE_INTEL     0x4

#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_VERTICAL_INTEL              0x0
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_HORIZONTAL_INTEL            0x1
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_DC_INTEL                    0x2
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_DIAGONAL_DOWN_LEFT_INTEL    0x3
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_DIAGONAL_DOWN_RIGHT_INTEL   0x4
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_PLANE_INTEL                 0x4
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_VERTICAL_RIGHT_INTEL        0x5
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_HORIZONTAL_DOWN_INTEL       0x6
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_VERTICAL_LEFT_INTEL         0x7
#define CLK_AVC_ME_LUMA_PREDICTOR_MODE_HORIZONTAL_UP_INTEL         0x8
#define CLK_AVC_ME_CHROMA_PREDICTOR_MODE_DC_INTEL                  0x0
#define CLK_AVC_ME_CHROMA_PREDICTOR_MODE_HORIZONTAL_INTEL          0x1
#define CLK_AVC_ME_CHROMA_PREDICTOR_MODE_VERTICAL_INTEL            0x2
#define CLK_AVC_ME_CHROMA_PREDICTOR_MODE_PLANE_INTEL               0x3

#define CLK_AVC_ME_FRAME_FORWARD_INTEL                             0x1
#define CLK_AVC_ME_FRAME_BACKWARD_INTEL                            0x2
#define CLK_AVC_ME_FRAME_DUAL_INTEL                                0x3

#define CLK_AVC_ME_SLICE_TYPE_PRED_INTEL                           0x0
#define CLK_AVC_ME_SLICE_TYPE_BPRED_INTEL                          0x1
#define CLK_AVC_ME_SLICE_TYPE_INTRA_INTEL                          0x2

#define CLK_AVC_ME_INTERLACED_SCAN_TOP_FIELD_INTEL                 0x0
#define CLK_AVC_ME_INTERLACED_SCAN_BOTTOM_FIELD_INTEL              0x1

#define CLK_AVC_ME_INITIALIZE_INTEL                                0x0

#define CLK_AVC_IME_PAYLOAD_INITIALIZE_INTEL                        {0}
#define CLK_AVC_REF_PAYLOAD_INITIALIZE_INTEL                        {0}
#define CLK_AVC_SIC_PAYLOAD_INITIALIZE_INTEL                        {0}

#define CLK_AVC_IME_RESULT_INITIALIZE_INTEL                         {0}
#define CLK_AVC_REF_RESULT_INITIALIZE_INTEL                         {0}
#define CLK_AVC_SIC_RESULT_INITIALIZE_INTEL                         {0}

// ... Helper macros ...
// (Internal only helpers for setting skip partition masks)

#define CLK_AVC_ME_INTERNAL_16x16_CLK_AVC_ME_MAJOR_FORWARD_INTEL        \
    (CLK_AVC_ME_SKIP_BLOCK_16x16_FORWARD_ENABLE_INTEL)
#define CLK_AVC_ME_INTERNAL_16x16_CLK_AVC_ME_MAJOR_BACKWARD_INTEL       \
    (CLK_AVC_ME_SKIP_BLOCK_16x16_BACKWARD_ENABLE_INTEL)
#define CLK_AVC_ME_INTERNAL_16x16_CLK_AVC_ME_MAJOR_BIDIRECTIONAL_INTEL  \
    (CLK_AVC_ME_SKIP_BLOCK_16x16_FORWARD_ENABLE_INTEL |                 \
     CLK_AVC_ME_SKIP_BLOCK_16x16_BACKWARD_ENABLE_INTEL)

#define CLK_AVC_ME_INTERNAL_8x8_CLK_AVC_ME_MAJOR_FORWARD_INTEL(PARTITION)       \
    (CLK_AVC_ME_SKIP_BLOCK_8x8_ ## PARTITION ## _FORWARD_ENABLE_INTEL)
#define CLK_AVC_ME_INTERNAL_8x8_CLK_AVC_ME_MAJOR_BACKWARD_INTEL(PARTITION)      \
    (CLK_AVC_ME_SKIP_BLOCK_8x8_ ## PARTITION ## _BACKWARD_ENABLE_INTEL)
#define CLK_AVC_ME_INTERNAL_8x8_CLK_AVC_ME_MAJOR_BIDIRECTIONAL_INTEL(PARTITION) \
    (CLK_AVC_ME_SKIP_BLOCK_8x8_ ## PARTITION ## _FORWARD_ENABLE_INTEL |         \
     CLK_AVC_ME_SKIP_BLOCK_8x8_ ## PARTITION ## _BACKWARD_ENABLE_INTEL)

// (External helpers for setting skip partition masks)

#define CLK_AVC_ME_SKIP_BLOCK_16x16_INTEL(DIRECTION)                                         \
    (CLK_AVC_ME_INTERNAL_16x16_ ## DIRECTION)
#define CLK_AVC_ME_SKIP_BLOCK_8x8_INTEL(DIRECTION0, DIRECTION1, DIRECTION2, DIRECTION3)      \
    (CLK_AVC_ME_INTERNAL_8x8_ ## DIRECTION0(0) | CLK_AVC_ME_INTERNAL_8x8_ ## DIRECTION1(1) | \
     CLK_AVC_ME_INTERNAL_8x8_ ## DIRECTION2(2) | CLK_AVC_ME_INTERNAL_8x8_ ## DIRECTION3(3))

// ... Types ...

struct intel_sub_group_avc_mce_payload_t;
typedef __private struct intel_sub_group_avc_mce_payload_t* intel_sub_group_avc_mce_payload_t;

struct intel_sub_group_avc_ime_payload_t;
typedef __private struct intel_sub_group_avc_ime_payload_t* intel_sub_group_avc_ime_payload_t;

struct intel_sub_group_avc_ref_payload_t;
typedef __private struct intel_sub_group_avc_ref_payload_t* intel_sub_group_avc_ref_payload_t;

struct intel_sub_group_avc_sic_payload_t;
typedef __private struct intel_sub_group_avc_sic_payload_t* intel_sub_group_avc_sic_payload_t;

struct intel_sub_group_avc_idm_payload_t;
typedef __private struct intel_sub_group_avc_idm_payload_t* intel_sub_group_avc_idm_payload_t;

struct intel_sub_group_avc_mce_result_t;
typedef __private struct intel_sub_group_avc_mce_result_t* intel_sub_group_avc_mce_result_t;

struct intel_sub_group_avc_ime_result_t;
typedef __private struct intel_sub_group_avc_ime_result_t* intel_sub_group_avc_ime_result_t;

struct intel_sub_group_avc_ime_result_single_reference_streamout_t;
typedef __private struct intel_sub_group_avc_ime_result_single_reference_streamout_t* intel_sub_group_avc_ime_result_single_reference_streamout_t;

struct intel_sub_group_avc_ime_result_dual_reference_streamout_t;
typedef __private struct intel_sub_group_avc_ime_result_dual_reference_streamout_t* intel_sub_group_avc_ime_result_dual_reference_streamout_t;

struct intel_sub_group_avc_ime_single_reference_streamin_t;
typedef __private struct intel_sub_group_avc_ime_single_reference_streamin_t* intel_sub_group_avc_ime_single_reference_streamin_t;

struct intel_sub_group_avc_ime_dual_reference_streamin_t;
typedef __private struct intel_sub_group_avc_ime_dual_reference_streamin_t* intel_sub_group_avc_ime_dual_reference_streamin_t;

struct intel_sub_group_avc_ref_result_t;
typedef __private struct intel_sub_group_avc_ref_result_t* intel_sub_group_avc_ref_result_t;

struct intel_sub_group_avc_sic_result_t;
typedef __private struct intel_sub_group_avc_sic_result_t* intel_sub_group_avc_sic_result_t;
#endif // !__VME_TYPES_DEFINED__

// ... Common VME operations ...
#pragma OPENCL EXTENSION cl_intel_device_side_avc_motion_estimation : enable

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_motion_vector_cost_function(
      ulong packed_cost_center_delta,
      uint2 packed_cost_table,
      uchar cost_precision,
      intel_sub_group_avc_mce_payload_t payload );

uint2 __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_high_penalty_cost_table(void);

uint2 __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_medium_penalty_cost_table(void);

uint2 __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_low_penalty_cost_table(void);

uint2 __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_inter_motion_vector_cost_table(
    uchar slice_type,
    uchar qp );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_inter_base_multi_reference_penalty(
    uchar slice_type,
    uchar qp );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_inter_base_multi_reference_penalty(
    uchar reference_penalty,
    intel_sub_group_avc_mce_payload_t payload );

ulong __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_inter_shape_penalty(
    uchar slice_type,
    uchar qp );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_mce_payload_t payload );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_inter_direction_penalty(
    uchar slice_type,
    uchar qp );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_intra_luma_shape_penalty(
     uint packed_shape_cost,
     intel_sub_group_avc_mce_payload_t payload );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_intra_luma_mode_penalty(
    uchar slice_type,
    uchar qp );

uint __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_intra_luma_shape_penalty(
    uchar slice_type,
    uchar qp );

uint __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_non_dc_luma_intra_penalty( void );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_intra_luma_mode_cost_function(
     uchar luma_mode_penalty,
     uint  luma_packed_neighbor_modes,
     uint  luma_packed_non_dc_penalty,
     intel_sub_group_avc_mce_payload_t payload );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_default_intra_chroma_mode_base_penalty( void );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_intra_chroma_mode_cost_function(
     uchar chroma_mode_penalty,
     intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_ac_only_haar(
    intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_mce_payload_t payload );

#ifdef __opencl_c_images
intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_single_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_dual_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    read_only image2d_t bwd_ref_image,
    intel_sub_group_avc_mce_payload_t payload );
#endif //__opencl_c_images

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_ref_id_raw(
    uint packed_ref_ids,
    intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_set_ref_id_polarities_raw(
    uchar packed_ref_id_polarities,
    intel_sub_group_avc_mce_payload_t payload );

ulong __attribute__((overloadable))
intel_sub_group_avc_mce_get_motion_vectors(
    intel_sub_group_avc_mce_result_t  result );

ushort __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_distortions(
    intel_sub_group_avc_mce_result_t  result );

ushort __attribute__((overloadable))
intel_sub_group_avc_mce_get_best_inter_distortion(
    intel_sub_group_avc_mce_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_major_shape(
    intel_sub_group_avc_mce_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_minor_shapes(
    intel_sub_group_avc_mce_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_directions(
    intel_sub_group_avc_mce_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_motion_vector_count(
    intel_sub_group_avc_mce_result_t  result );

uint __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_reference_ids(
    intel_sub_group_avc_mce_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_mce_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint  packed_reference_parameter_field_polarities,
    intel_sub_group_avc_mce_result_t  result );

// ... IME operations ...

ushort2 __attribute__((overloadable))
intel_sub_group_ime_ref_window_size(
  uchar search_window_config,
  char  dual_ref );

short2 __attribute__((overloadable))
intel_sub_group_avc_ime_adjust_ref_offset(
   short2  ref_offset,
   ushort2 src_coord,
   ushort2 ref_window_size,
   ushort2 frame_size );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_initialize(
    ushort2 src_coord,
    uchar partition_mask,
    uchar sad_adjustment );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_single_reference(
    short2 ref_offset,
    uchar  search_window_config,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_dual_reference(
   short2 fwd_ref_offset,
   short2 bwd_ref_offset,
   uchar search_window_config,
   intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_max_motion_vector_count(
    uchar  max_motion_vector_count,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_convert_to_mce_payload(
      intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_ime_payload(
      intel_sub_group_avc_mce_payload_t payload );

#ifdef __opencl_c_images
intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_result_single_reference_streamout_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_single_reference_streamout(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_result_dual_reference_streamout_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_dual_reference_streamout(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_single_reference_streamin(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_single_reference_streamin_t streamin );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_dual_reference_streamin(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_dual_reference_streamin_t streamin );

intel_sub_group_avc_ime_result_single_reference_streamout_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_single_reference_streaminout(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_single_reference_streamin_t streamin );

intel_sub_group_avc_ime_result_dual_reference_streamout_t __attribute__((overloadable))
intel_sub_group_avc_ime_evaluate_with_dual_reference_streaminout(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ime_payload_t payload,
      intel_sub_group_avc_ime_dual_reference_streamin_t streamin );
#endif //__opencl_c_images

intel_sub_group_avc_ime_single_reference_streamin_t __attribute__((overloadable))
intel_sub_group_avc_ime_get_single_reference_streamin(
   intel_sub_group_avc_ime_result_single_reference_streamout_t result );

intel_sub_group_avc_ime_dual_reference_streamin_t __attribute__((overloadable))
intel_sub_group_avc_ime_get_dual_reference_streamin(
   intel_sub_group_avc_ime_result_dual_reference_streamout_t result );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_strip_single_reference_streamout(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_strip_dual_reference_streamout(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result );

uint __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_motion_vectors(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape );

ushort __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_distortions(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_reference_ids(
    intel_sub_group_avc_ime_result_single_reference_streamout_t result,
    uchar major_shape );

uint __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_motion_vectors(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar major_shape,
    uchar direction );

ushort __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_distortions(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar major_shape,
    uchar direction );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_streamout_major_shape_reference_ids(
    intel_sub_group_avc_ime_result_dual_reference_streamout_t result,
    uchar major_shape,
    uchar direction );

intel_sub_group_avc_mce_result_t __attribute__((overloadable))
intel_sub_group_avc_ime_convert_to_mce_result(
      intel_sub_group_avc_ime_result_t  result );

intel_sub_group_avc_ime_result_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_ime_result(
      intel_sub_group_avc_mce_result_t  result );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_unidirectional_mix_disable(
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_early_search_termination_threshold(
    uchar threshold,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_early_unidirectional_search_termination_threshold(
    uchar threshold,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_weighted_sad(
    uint packed_sad_weights,
    intel_sub_group_avc_ime_payload_t payload );

uint __attribute__((overloadable))
intel_sub_group_avc_ime_get_weighting_pattern_minimum_motion_vector(
    intel_sub_group_avc_ime_result_t  result );

ushort __attribute__((overloadable))
intel_sub_group_avc_ime_get_weighting_pattern_minimum_distortion(
    intel_sub_group_avc_ime_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_unidirectional_early_search_termination(
    intel_sub_group_avc_ime_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_border_reached(
    uchar frame_select,
    intel_sub_group_avc_ime_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_truncated_search_indication(
    intel_sub_group_avc_ime_result_t payload );

// ... Common VME operation wrappers ...

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_motion_vector_cost_function(
    ulong packed_cost_center_delta,
    uint2 packed_cost_table,
    uchar cost_precision,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_motion_vector_cost_function(
    ulong packed_cost_center_delta,
    uint2 packed_cost_table,
    uchar cost_precision,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_motion_vector_cost_function(
    ulong packed_cost_center_delta,
    uint2 packed_cost_table,
    uchar cost_precision,
    intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_inter_base_multi_reference_penalty(
    uchar reference_penalty,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_inter_base_multi_reference_penalty(
    uchar reference_penalty,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_inter_base_multi_reference_penalty(
    uchar reference_penalty,
    intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_inter_shape_penalty(
    ulong packed_shape_cost,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_inter_shape_penalty(
     ulong packed_shape_cost,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_intra_luma_shape_penalty(
     uint packed_shape_cost,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_inter_direction_penalty(
    uchar direction_cost,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_inter_direction_penalty(
     uchar direction_cost,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_intra_luma_mode_cost_function(
     uchar luma_mode_penalty,
     uint  luma_packed_neighbor_modes,
     uint  luma_packed_non_dc_penalty,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_intra_chroma_mode_cost_function(
     uchar chroma_mode_penalty,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_source_interlaced_field_polarity(
     uchar  src_field_polarity,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_single_reference_interlaced_field_polarity(
     uchar  ref_field_polarity,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_dual_reference_interlaced_field_polarities(
     uchar  fwd_ref_field_polarity,
     uchar  bwd_ref_field_polarity,
     intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_ac_only_haar(
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_ac_only_haar(
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_ac_only_haar(
    intel_sub_group_avc_sic_payload_t payload );

#ifdef __opencl_c_images
intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_dual_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    read_only image2d_t bwd_ref_image,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_single_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_dual_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    read_only image2d_t bwd_ref_image,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_single_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_dual_ref_id(
    read_only image2d_t src_image,
    read_only image2d_t fwd_ref_image,
    read_only image2d_t bwd_ref_image,
    intel_sub_group_avc_sic_payload_t payload );
#endif //__opencl_c_images

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_ref_id_raw(
    uint packed_ref_ids,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_ref_id_raw(
    uint packed_ref_ids,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_ref_id_raw(
    uint packed_ref_ids,
    intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_ime_payload_t __attribute__((overloadable))
intel_sub_group_avc_ime_set_ref_id_polarities_raw(
    uchar packed_ref_id_polarities,
    intel_sub_group_avc_ime_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_ref_id_polarities_raw(
    uchar packed_ref_id_polarities,
    intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_ref_id_polarities_raw(
    uchar packed_ref_id_polarities,
    intel_sub_group_avc_sic_payload_t payload );

ulong __attribute__((overloadable))
intel_sub_group_avc_ime_get_motion_vectors(
    intel_sub_group_avc_ime_result_t result );

ulong __attribute__((overloadable))
intel_sub_group_avc_ref_get_motion_vectors(
    intel_sub_group_avc_ref_result_t result );

ushort __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_distortions(
    intel_sub_group_avc_ime_result_t result );

ushort __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_distortions(
    intel_sub_group_avc_ref_result_t result );

ushort __attribute__((overloadable))
intel_sub_group_avc_sic_get_inter_distortions(
    intel_sub_group_avc_sic_result_t result );

ushort __attribute__((overloadable))
intel_sub_group_avc_ime_get_best_inter_distortion(
    intel_sub_group_avc_ime_result_t result );

ushort __attribute__((overloadable))
intel_sub_group_avc_ref_get_best_inter_distortion(
    intel_sub_group_avc_ref_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_major_shape(
    intel_sub_group_avc_ime_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_major_shape(
    intel_sub_group_avc_ref_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_minor_shapes(
    intel_sub_group_avc_ime_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_minor_shapes(
    intel_sub_group_avc_ref_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_directions(
    intel_sub_group_avc_ime_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_motion_vector_count(
    intel_sub_group_avc_ime_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_motion_vector_count(
    intel_sub_group_avc_ref_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_directions(
    intel_sub_group_avc_ref_result_t result );

uint __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_reference_ids(
    intel_sub_group_avc_ime_result_t result );

uint __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_reference_ids(
    intel_sub_group_avc_ref_result_t result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ime_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint packed_reference_parameter_field_polarities,
    intel_sub_group_avc_ime_result_t  result );

uchar __attribute__((overloadable))
intel_sub_group_avc_ref_get_inter_reference_interlaced_field_polarities(
    uint packed_reference_ids,
    uint packed_reference_parameter_field_polarities,
    intel_sub_group_avc_ref_result_t  result );

// ... REF operations ...

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_fme_initialize(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shapes,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar sad_adjustment );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_bme_initialize(
    ushort2 src_coord,
    ulong motion_vectors,
    uchar major_shapes,
    uchar minor_shapes,
    uchar directions,
    uchar pixel_resolution,
    uchar bidirectional_weight,
    uchar sad_adjustment );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_convert_to_mce_payload(
      intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_ref_payload(
      intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_bidirectional_mix_disable(
      intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_payload_t __attribute__((overloadable))
intel_sub_group_avc_ref_set_bilinear_filter_enable(
      intel_sub_group_avc_ref_payload_t payload );

#ifdef __opencl_c_images
intel_sub_group_avc_ref_result_t __attribute__((overloadable))
intel_sub_group_avc_ref_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_result_t __attribute__((overloadable))
intel_sub_group_avc_ref_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_result_t __attribute__((overloadable))
intel_sub_group_avc_ref_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload );

intel_sub_group_avc_ref_result_t __attribute__((overloadable))
intel_sub_group_avc_ref_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      uchar packed_reference_field_polarities,
      sampler_t vme_accelerator,
      intel_sub_group_avc_ref_payload_t payload );
#endif //__opencl_c_images

intel_sub_group_avc_mce_result_t __attribute__((overloadable))
intel_sub_group_avc_ref_convert_to_mce_result(
      intel_sub_group_avc_ref_result_t result );

intel_sub_group_avc_ref_result_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_ref_result(
      intel_sub_group_avc_mce_result_t result );

// ... SIC operations ...

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_initialize(
    ushort2 src_coord );

uint __attribute__((overloadable))
intel_sub_group_avc_sic_get_motion_vector_mask(
      uint skip_block_partition_type,
      uchar direction );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_configure_skc(
      uint skip_block_partition_type,
      uint skip_motion_vector_mask,
      ulong motion_vectors,
      uchar bidirectional_weight,
      uchar skip_sad_adjustment,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_configure_ipe(
      uchar luma_intra_partition_mask,
      uchar intra_neighbour_availabilty,
      uchar left_edge_luma_pixels,
      uchar left_upper_edge_luma_pixel,
      uchar upper_edge_luma_pixels,
      uchar upper_right_edge_luma_pixels,
      uchar intra_sad_adjustment,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_configure_ipe(
      uchar luma_intra_partition_mask,
      uchar intra_neighbour_availabilty,
      uchar left_edge_luma_pixels,
      uchar left_upper_edge_luma_pixel,
      uchar upper_edge_luma_pixels,
      uchar upper_right_edge_luma_pixels,
      ushort left_edge_chroma_pixels,
      ushort upper_left_corner_chroma_pixel,
      ushort upper_edge_chroma_pixels,
      uchar intra_sad_adjustment,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_mce_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_convert_to_mce_payload(
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_sic_payload(
      intel_sub_group_avc_mce_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_skc_bilinear_filter_enable(
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_skc_forward_transform_enable(
      ulong  packed_sad_coefficients,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_payload_t __attribute__((overloadable))
intel_sub_group_avc_sic_set_block_based_raw_skip_sad(
      uchar block_based_skip_block_type,
      intel_sub_group_avc_sic_payload_t payload );

#ifdef __opencl_c_images
intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_evaluate_ipe(
      read_only image2d_t src_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_evaluate_with_single_reference(
      read_only image2d_t src_image,
      read_only image2d_t ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_evaluate_with_dual_reference(
      read_only image2d_t src_image,
      read_only image2d_t fwd_ref_image,
      read_only image2d_t bwd_ref_image,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload );

intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_evaluate_with_multi_reference(
      read_only image2d_t src_image,
      uint packed_reference_ids,
      uchar packed_reference_field_polarities,
      sampler_t vme_accelerator,
      intel_sub_group_avc_sic_payload_t payload );
#endif //__opencl_c_images

intel_sub_group_avc_mce_result_t __attribute__((overloadable))
intel_sub_group_avc_sic_convert_to_mce_result(
      intel_sub_group_avc_sic_result_t result );

intel_sub_group_avc_sic_result_t __attribute__((overloadable))
intel_sub_group_avc_mce_convert_to_sic_result(
      intel_sub_group_avc_mce_result_t result);

uchar __attribute__((overloadable))
intel_sub_group_avc_sic_get_ipe_luma_shape(
      intel_sub_group_avc_sic_result_t result);

ushort __attribute__((overloadable))
intel_sub_group_avc_sic_get_best_ipe_luma_distortion(
      intel_sub_group_avc_sic_result_t result);

ushort __attribute__((overloadable))
intel_sub_group_avc_sic_get_best_ipe_chroma_distortion(
      intel_sub_group_avc_sic_result_t result);

ulong __attribute__((overloadable))
intel_sub_group_avc_sic_get_packed_ipe_luma_modes(
      intel_sub_group_avc_sic_result_t result);

uchar __attribute__((overloadable))
intel_sub_group_avc_sic_get_ipe_chroma_mode(
      intel_sub_group_avc_sic_result_t result);

uint __attribute__((overloadable))
intel_sub_group_avc_sic_get_packed_skc_luma_count_threshold(
      intel_sub_group_avc_sic_result_t result);

ulong __attribute__((overloadable))
intel_sub_group_avc_sic_get_packed_skc_luma_sum_threshold(
      intel_sub_group_avc_sic_result_t result);

ushort __attribute__((overloadable))
intel_sub_group_avc_sic_get_inter_raw_sads(
      intel_sub_group_avc_sic_result_t result);

#endif // cl_intel_device_side_avc_motion_estimation

#ifdef __opencl_c_images
#ifdef cl_intel_device_side_va_enable
// VA
//
// All of the comments above about VME also apply here.
// This is good enough to improve the runtime's toolchain, but it is
// not sufficient to expose a general-purpose device-side VA built-in
// function extension.

// These functions accept int2 un-normalized coordinates.
void __attribute__((overloadable)) intel_work_group_va_boolcentroid(
    __local void* dst,
    int2 coord,
    int2 size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_boolsum(
    __local void* dst,
    int2 coord,
    int2 size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_centroid(
    __local void* dst,
    int2 coord,
    int size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_convolve_16x4(
    __local void* dst,
    int2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_dilate_64x4(
    __local void* dst,
    int2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_erode_64x4(
    __local void* dst,
    int2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_minmax(
    __local void* dst,
    int2 coord,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_minmaxfilter_16x4(
    __local void* dst,
    int2 coord,
    image2d_t image,
    sampler_t a );

// These functions accept float2 normalized coordinates.

void __attribute__((overloadable)) intel_work_group_va_boolcentroid(
    __local void* dst,
    float2 coord,
    int2 size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_boolsum(
    __local void* dst,
    float2 coord,
    int2 size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_centroid(
    __local void* dst,
    float2 coord,
    int size,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_convolve_16x4(
    __local void* dst,
    float2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_dilate_64x4(
    __local void* dst,
    float2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_erode_64x4(
    __local void* dst,
    float2 coord,
    image2d_t image,
    sampler_t a );
void __attribute__((overloadable)) intel_work_group_va_minmax(
    __local void* dst,
    float2 coord,
    image2d_t image,
    sampler_t a );  // TODO: Should this be "accelerator-less"?
void __attribute__((overloadable)) intel_work_group_va_minmaxfilter_16x4(
    __local void* dst,
    float2 coord,
    image2d_t image,
    sampler_t a );
short __attribute__( ( overloadable ) ) intel_work_group_va_convolve_16x1(
    float2 coord,
    image2d_t image,
    sampler_t a );
short4 __attribute__( ( overloadable ) ) intel_work_group_va_convolve_16x4(
    float2 coord,
    image2d_t image,
    sampler_t a );
uchar __attribute__( ( overloadable ) ) intel_work_group_va_minfilter_16x1(
    float2 coord,
    image2d_t image,
    sampler_t a );
uchar4 __attribute__( ( overloadable ) ) intel_work_group_va_minfilter_16x4(
    float2 coord,
    image2d_t image,
    sampler_t a );
uchar __attribute__( ( overloadable ) ) intel_work_group_va_maxfilter_16x1(
    float2 coord,
    image2d_t image,
    sampler_t a );
uchar4 __attribute__( ( overloadable ) ) intel_work_group_va_maxfilter_16x4(
    float2 coord,
    image2d_t image,
    sampler_t a );
#endif

#ifdef cl_intel_image_atomics
  int __attribute__((overloadable)) intel_atomic_and    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image1d_buffer_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image1d_buffer_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image1d_buffer_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image1d_buffer_t image, int coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image1d_buffer_t image, int coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image1d_buffer_t image, int coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image1d_buffer_t image, int coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image1d_buffer_t image, int coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image1d_buffer_t image, int coord);
  int __attribute__((overloadable)) intel_atomic_add    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image1d_buffer_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image1d_buffer_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_buffer_t image, int coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_buffer_t image, int coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_buffer_t image, int coord, float cmp, float val);    // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image1d_buffer_t image, int coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image1d_buffer_t image, int coord, float val);               // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image1d_buffer_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image1d_buffer_t image, int coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image1d_buffer_t image, int coord, float val);               // NOT supporting - float atomic

  int __attribute__((overloadable)) intel_atomic_and    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image1d_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image1d_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image1d_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image1d_t image, int coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image1d_t image, int coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image1d_t image, int coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image1d_t image, int coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image1d_t image, int coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image1d_t image, int coord);
  int __attribute__((overloadable)) intel_atomic_add    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image1d_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image1d_t image, int coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_t image, int coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_t image, int coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_t image, int coord, float cmp, float val);   // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image1d_t image, int coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image1d_t image, int coord, float val);              // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image1d_t image, int coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image1d_t image, int coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image1d_t image, int coord, float val);              // NOT supporting - float atomic

  int __attribute__((overloadable)) intel_atomic_and    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image1d_array_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image1d_array_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image1d_array_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image1d_array_t image, int2 coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image1d_array_t image, int2 coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image1d_array_t image, int2 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image1d_array_t image, int2 coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image1d_array_t image, int2 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image1d_array_t image, int2 coord);
  int __attribute__((overloadable)) intel_atomic_add    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image1d_array_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image1d_array_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_array_t image, int2 coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_array_t image, int2 coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image1d_array_t image, int2 coord, float cmp, float val);    // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image1d_array_t image, int2 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image1d_array_t image, int2 coord, float val);               // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image1d_array_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image1d_array_t image, int2 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image1d_array_t image, int2 coord, float val);               // NOT supporting - float atomic

  int __attribute__((overloadable)) intel_atomic_and    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image2d_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image2d_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image2d_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image2d_t image, int2 coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image2d_t image, int2 coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image2d_t image, int2 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image2d_t image, int2 coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image2d_t image, int2 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image2d_t image, int2 coord);
  int __attribute__((overloadable)) intel_atomic_add    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image2d_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image2d_t image, int2 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_t image, int2 coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_t image, int2 coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_t image, int2 coord, float cmp, float val);  // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image2d_t image, int2 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image2d_t image, int2 coord, float val);             // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image2d_t image, int2 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image2d_t image, int2 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image2d_t image, int2 coord, float val);             // NOT supporting - float atomic

  int __attribute__((overloadable)) intel_atomic_and    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image2d_array_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image2d_array_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image2d_array_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image2d_array_t image, int4 coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image2d_array_t image, int4 coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image2d_array_t image, int4 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image2d_array_t image, int4 coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image2d_array_t image, int4 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image2d_array_t image, int4 coord);
  int __attribute__((overloadable)) intel_atomic_add    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image2d_array_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image2d_array_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_array_t image, int4 coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_array_t image, int4 coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image2d_array_t image, int4 coord, float cmp, float val);    // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image2d_array_t image, int4 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image2d_array_t image, int4 coord, float val);               // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image2d_array_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image2d_array_t image, int4 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image2d_array_t image, int4 coord, float val);               // NOT supporting - float atomic

  int __attribute__((overloadable)) intel_atomic_and    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_and    (image3d_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_or     (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_or     (image3d_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xor    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xor    (image3d_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_xchg   (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_xchg   (image3d_t image, int4 coord,  uint val);
float __attribute__((overloadable)) intel_atomic_xchg   (image3d_t image, int4 coord, float val);
  int __attribute__((overloadable)) intel_atomic_inc    (image3d_t image, int4 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_inc    (image3d_t image, int4 coord);
  int __attribute__((overloadable)) intel_atomic_dec    (image3d_t image, int4 coord);
// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//uint __attribute__((overloadable)) intel_atomic_dec    (image3d_t image, int4 coord);
  int __attribute__((overloadable)) intel_atomic_add    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_add    (image3d_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_sub    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_sub    (image3d_t image, int4 coord,  uint val);
  int __attribute__((overloadable)) intel_atomic_cmpxchg(image3d_t image, int4 coord,   int cmp,   int val);
 uint __attribute__((overloadable)) intel_atomic_cmpxchg(image3d_t image, int4 coord,  uint cmp,  uint val);
//float __attribute__((overloadable)) intel_atomic_cmpxchg(image3d_t image, int4 coord, float cmp, float val);  // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_min    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_min    (image3d_t image, int4 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_min    (image3d_t image, int4 coord, float val);             // NOT supporting - float atomic
  int __attribute__((overloadable)) intel_atomic_max    (image3d_t image, int4 coord,   int val);
 uint __attribute__((overloadable)) intel_atomic_max    (image3d_t image, int4 coord,  uint val);
//float __attribute__((overloadable)) intel_atomic_max    (image3d_t image, int4 coord, float val);             // NOT supporting - float atomic

float __attribute__((overloadable)) intel_atomic_xchg   (image2d_depth_t image, int2 coord, float val);

float __attribute__((overloadable)) intel_atomic_xchg   (image2d_array_depth_t image, int4 coord, float val);
#endif
#endif //__opencl_c_images


#if defined(cl_khr_extended_bit_ops)
// bfi i8
uchar   __attribute__((overloadable)) bitfield_insert(uchar   base, uchar   insert, uint offset, uint count);
uchar2  __attribute__((overloadable)) bitfield_insert(uchar2  base, uchar2  insert, uint offset, uint count);
uchar3  __attribute__((overloadable)) bitfield_insert(uchar3  base, uchar3  insert, uint offset, uint count);
uchar4  __attribute__((overloadable)) bitfield_insert(uchar4  base, uchar4  insert, uint offset, uint count);
uchar8  __attribute__((overloadable)) bitfield_insert(uchar8  base, uchar8  insert, uint offset, uint count);
uchar16 __attribute__((overloadable)) bitfield_insert(uchar16 base, uchar16 insert, uint offset, uint count);
char    __attribute__((overloadable)) bitfield_insert(char    base, char    insert, uint offset, uint count);
char2   __attribute__((overloadable)) bitfield_insert(char2   base, char2   insert, uint offset, uint count);
char3   __attribute__((overloadable)) bitfield_insert(char3   base, char3   insert, uint offset, uint count);
char4   __attribute__((overloadable)) bitfield_insert(char4   base, char4   insert, uint offset, uint count);
char8   __attribute__((overloadable)) bitfield_insert(char8   base, char8   insert, uint offset, uint count);
char16  __attribute__((overloadable)) bitfield_insert(char16  base, char16  insert, uint offset, uint count);

// bfi i16
ushort   __attribute__((overloadable)) bitfield_insert(ushort   base, ushort   insert, uint offset, uint count);
ushort2  __attribute__((overloadable)) bitfield_insert(ushort2  base, ushort2  insert, uint offset, uint count);
ushort3  __attribute__((overloadable)) bitfield_insert(ushort3  base, ushort3  insert, uint offset, uint count);
ushort4  __attribute__((overloadable)) bitfield_insert(ushort4  base, ushort4  insert, uint offset, uint count);
ushort8  __attribute__((overloadable)) bitfield_insert(ushort8  base, ushort8  insert, uint offset, uint count);
ushort16 __attribute__((overloadable)) bitfield_insert(ushort16 base, ushort16 insert, uint offset, uint count);
short    __attribute__((overloadable)) bitfield_insert(short    base, short    insert, uint offset, uint count);
short2   __attribute__((overloadable)) bitfield_insert(short2   base, short2   insert, uint offset, uint count);
short3   __attribute__((overloadable)) bitfield_insert(short3   base, short3   insert, uint offset, uint count);
short4   __attribute__((overloadable)) bitfield_insert(short4   base, short4   insert, uint offset, uint count);
short8   __attribute__((overloadable)) bitfield_insert(short8   base, short8   insert, uint offset, uint count);
short16  __attribute__((overloadable)) bitfield_insert(short16  base, short16  insert, uint offset, uint count);

// bfi i32
uint   __attribute__((overloadable)) bitfield_insert(uint   base, uint   insert, uint offset, uint count);
uint2  __attribute__((overloadable)) bitfield_insert(uint2  base, uint2  insert, uint offset, uint count);
uint3  __attribute__((overloadable)) bitfield_insert(uint3  base, uint3  insert, uint offset, uint count);
uint4  __attribute__((overloadable)) bitfield_insert(uint4  base, uint4  insert, uint offset, uint count);
uint8  __attribute__((overloadable)) bitfield_insert(uint8  base, uint8  insert, uint offset, uint count);
uint16 __attribute__((overloadable)) bitfield_insert(uint16 base, uint16 insert, uint offset, uint count);
int    __attribute__((overloadable)) bitfield_insert(int    base, int    insert, uint offset, uint count);
int2   __attribute__((overloadable)) bitfield_insert(int2   base, int2   insert, uint offset, uint count);
int3   __attribute__((overloadable)) bitfield_insert(int3   base, int3   insert, uint offset, uint count);
int4   __attribute__((overloadable)) bitfield_insert(int4   base, int4   insert, uint offset, uint count);
int8   __attribute__((overloadable)) bitfield_insert(int8   base, int8   insert, uint offset, uint count);
int16  __attribute__((overloadable)) bitfield_insert(int16  base, int16  insert, uint offset, uint count);

// bfi i64
ulong   __attribute__((overloadable)) bitfield_insert(ulong   base, ulong   insert, uint offset, uint count);
ulong2  __attribute__((overloadable)) bitfield_insert(ulong2  base, ulong2  insert, uint offset, uint count);
ulong3  __attribute__((overloadable)) bitfield_insert(ulong3  base, ulong3  insert, uint offset, uint count);
ulong4  __attribute__((overloadable)) bitfield_insert(ulong4  base, ulong4  insert, uint offset, uint count);
ulong8  __attribute__((overloadable)) bitfield_insert(ulong8  base, ulong8  insert, uint offset, uint count);
ulong16 __attribute__((overloadable)) bitfield_insert(ulong16 base, ulong16 insert, uint offset, uint count);
long    __attribute__((overloadable)) bitfield_insert(long    base, long    insert, uint offset, uint count);
long2   __attribute__((overloadable)) bitfield_insert(long2   base, long2   insert, uint offset, uint count);
long3   __attribute__((overloadable)) bitfield_insert(long3   base, long3   insert, uint offset, uint count);
long4   __attribute__((overloadable)) bitfield_insert(long4   base, long4   insert, uint offset, uint count);
long8   __attribute__((overloadable)) bitfield_insert(long8   base, long8   insert, uint offset, uint count);
long16  __attribute__((overloadable)) bitfield_insert(long16  base, long16  insert, uint offset, uint count);

// sbfe i8
char   __attribute__((overloadable)) bitfield_extract_signed(uchar   base, uint offset, uint count);
char2  __attribute__((overloadable)) bitfield_extract_signed(uchar2  base, uint offset, uint count);
char3  __attribute__((overloadable)) bitfield_extract_signed(uchar3  base, uint offset, uint count);
char4  __attribute__((overloadable)) bitfield_extract_signed(uchar4  base, uint offset, uint count);
char8  __attribute__((overloadable)) bitfield_extract_signed(uchar8  base, uint offset, uint count);
char16 __attribute__((overloadable)) bitfield_extract_signed(uchar16 base, uint offset, uint count);
char   __attribute__((overloadable)) bitfield_extract_signed(char    base, uint offset, uint count);
char2  __attribute__((overloadable)) bitfield_extract_signed(char2   base, uint offset, uint count);
char3  __attribute__((overloadable)) bitfield_extract_signed(char3   base, uint offset, uint count);
char4  __attribute__((overloadable)) bitfield_extract_signed(char4   base, uint offset, uint count);
char8  __attribute__((overloadable)) bitfield_extract_signed(char8   base, uint offset, uint count);
char16 __attribute__((overloadable)) bitfield_extract_signed(char16  base, uint offset, uint count);

// sbfe i16
short   __attribute__((overloadable)) bitfield_extract_signed(ushort   base, uint offset, uint count);
short2  __attribute__((overloadable)) bitfield_extract_signed(ushort2  base, uint offset, uint count);
short3  __attribute__((overloadable)) bitfield_extract_signed(ushort3  base, uint offset, uint count);
short4  __attribute__((overloadable)) bitfield_extract_signed(ushort4  base, uint offset, uint count);
short8  __attribute__((overloadable)) bitfield_extract_signed(ushort8  base, uint offset, uint count);
short16 __attribute__((overloadable)) bitfield_extract_signed(ushort16 base, uint offset, uint count);
short   __attribute__((overloadable)) bitfield_extract_signed(short    base, uint offset, uint count);
short2  __attribute__((overloadable)) bitfield_extract_signed(short2   base, uint offset, uint count);
short3  __attribute__((overloadable)) bitfield_extract_signed(short3   base, uint offset, uint count);
short4  __attribute__((overloadable)) bitfield_extract_signed(short4   base, uint offset, uint count);
short8  __attribute__((overloadable)) bitfield_extract_signed(short8   base, uint offset, uint count);
short16 __attribute__((overloadable)) bitfield_extract_signed(short16  base, uint offset, uint count);

// sbfe i32
int   __attribute__((overloadable)) bitfield_extract_signed(uint   base, uint offset, uint count);
int2  __attribute__((overloadable)) bitfield_extract_signed(uint2  base, uint offset, uint count);
int3  __attribute__((overloadable)) bitfield_extract_signed(uint3  base, uint offset, uint count);
int4  __attribute__((overloadable)) bitfield_extract_signed(uint4  base, uint offset, uint count);
int8  __attribute__((overloadable)) bitfield_extract_signed(uint8  base, uint offset, uint count);
int16 __attribute__((overloadable)) bitfield_extract_signed(uint16 base, uint offset, uint count);
int   __attribute__((overloadable)) bitfield_extract_signed(int    base, uint offset, uint count);
int2  __attribute__((overloadable)) bitfield_extract_signed(int2   base, uint offset, uint count);
int3  __attribute__((overloadable)) bitfield_extract_signed(int3   base, uint offset, uint count);
int4  __attribute__((overloadable)) bitfield_extract_signed(int4   base, uint offset, uint count);
int8  __attribute__((overloadable)) bitfield_extract_signed(int8   base, uint offset, uint count);
int16 __attribute__((overloadable)) bitfield_extract_signed(int16  base, uint offset, uint count);

// sbfe i64
long   __attribute__((overloadable)) bitfield_extract_signed(ulong   base, uint offset, uint count);
long2  __attribute__((overloadable)) bitfield_extract_signed(ulong2  base, uint offset, uint count);
long3  __attribute__((overloadable)) bitfield_extract_signed(ulong3  base, uint offset, uint count);
long4  __attribute__((overloadable)) bitfield_extract_signed(ulong4  base, uint offset, uint count);
long8  __attribute__((overloadable)) bitfield_extract_signed(ulong8  base, uint offset, uint count);
long16 __attribute__((overloadable)) bitfield_extract_signed(ulong16 base, uint offset, uint count);
long   __attribute__((overloadable)) bitfield_extract_signed(long    base, uint offset, uint count);
long2  __attribute__((overloadable)) bitfield_extract_signed(long2   base, uint offset, uint count);
long3  __attribute__((overloadable)) bitfield_extract_signed(long3   base, uint offset, uint count);
long4  __attribute__((overloadable)) bitfield_extract_signed(long4   base, uint offset, uint count);
long8  __attribute__((overloadable)) bitfield_extract_signed(long8   base, uint offset, uint count);
long16 __attribute__((overloadable)) bitfield_extract_signed(long16  base, uint offset, uint count);


// ubfe i8
uchar   __attribute__((overloadable)) bitfield_extract_unsigned(uchar   base, uint offset, uint count);
uchar2  __attribute__((overloadable)) bitfield_extract_unsigned(uchar2  base, uint offset, uint count);
uchar3  __attribute__((overloadable)) bitfield_extract_unsigned(uchar3  base, uint offset, uint count);
uchar4  __attribute__((overloadable)) bitfield_extract_unsigned(uchar4  base, uint offset, uint count);
uchar8  __attribute__((overloadable)) bitfield_extract_unsigned(uchar8  base, uint offset, uint count);
uchar16 __attribute__((overloadable)) bitfield_extract_unsigned(uchar16 base, uint offset, uint count);
uchar   __attribute__((overloadable)) bitfield_extract_unsigned(char    base, uint offset, uint count);
uchar2  __attribute__((overloadable)) bitfield_extract_unsigned(char2   base, uint offset, uint count);
uchar3  __attribute__((overloadable)) bitfield_extract_unsigned(char3   base, uint offset, uint count);
uchar4  __attribute__((overloadable)) bitfield_extract_unsigned(char4   base, uint offset, uint count);
uchar8  __attribute__((overloadable)) bitfield_extract_unsigned(char8   base, uint offset, uint count);
uchar16 __attribute__((overloadable)) bitfield_extract_unsigned(char16  base, uint offset, uint count);

// ubfe i16
ushort   __attribute__((overloadable)) bitfield_extract_unsigned(ushort   base, uint offset, uint count);
ushort2  __attribute__((overloadable)) bitfield_extract_unsigned(ushort2  base, uint offset, uint count);
ushort3  __attribute__((overloadable)) bitfield_extract_unsigned(ushort3  base, uint offset, uint count);
ushort4  __attribute__((overloadable)) bitfield_extract_unsigned(ushort4  base, uint offset, uint count);
ushort8  __attribute__((overloadable)) bitfield_extract_unsigned(ushort8  base, uint offset, uint count);
ushort16 __attribute__((overloadable)) bitfield_extract_unsigned(ushort16 base, uint offset, uint count);
ushort   __attribute__((overloadable)) bitfield_extract_unsigned(short    base, uint offset, uint count);
ushort2  __attribute__((overloadable)) bitfield_extract_unsigned(short2   base, uint offset, uint count);
ushort3  __attribute__((overloadable)) bitfield_extract_unsigned(short3   base, uint offset, uint count);
ushort4  __attribute__((overloadable)) bitfield_extract_unsigned(short4   base, uint offset, uint count);
ushort8  __attribute__((overloadable)) bitfield_extract_unsigned(short8   base, uint offset, uint count);
ushort16 __attribute__((overloadable)) bitfield_extract_unsigned(short16  base, uint offset, uint count);

// ubfe i32
uint   __attribute__((overloadable)) bitfield_extract_unsigned(uint   base, uint offset, uint count);
uint2  __attribute__((overloadable)) bitfield_extract_unsigned(uint2  base, uint offset, uint count);
uint3  __attribute__((overloadable)) bitfield_extract_unsigned(uint3  base, uint offset, uint count);
uint4  __attribute__((overloadable)) bitfield_extract_unsigned(uint4  base, uint offset, uint count);
uint8  __attribute__((overloadable)) bitfield_extract_unsigned(uint8  base, uint offset, uint count);
uint16 __attribute__((overloadable)) bitfield_extract_unsigned(uint16 base, uint offset, uint count);
uint   __attribute__((overloadable)) bitfield_extract_unsigned(int    base, uint offset, uint count);
uint2  __attribute__((overloadable)) bitfield_extract_unsigned(int2   base, uint offset, uint count);
uint3  __attribute__((overloadable)) bitfield_extract_unsigned(int3   base, uint offset, uint count);
uint4  __attribute__((overloadable)) bitfield_extract_unsigned(int4   base, uint offset, uint count);
uint8  __attribute__((overloadable)) bitfield_extract_unsigned(int8   base, uint offset, uint count);
uint16 __attribute__((overloadable)) bitfield_extract_unsigned(int16  base, uint offset, uint count);

// ubfe i64
ulong   __attribute__((overloadable)) bitfield_extract_unsigned(ulong   base, uint offset, uint count);
ulong2  __attribute__((overloadable)) bitfield_extract_unsigned(ulong2  base, uint offset, uint count);
ulong3  __attribute__((overloadable)) bitfield_extract_unsigned(ulong3  base, uint offset, uint count);
ulong4  __attribute__((overloadable)) bitfield_extract_unsigned(ulong4  base, uint offset, uint count);
ulong8  __attribute__((overloadable)) bitfield_extract_unsigned(ulong8  base, uint offset, uint count);
ulong16 __attribute__((overloadable)) bitfield_extract_unsigned(ulong16 base, uint offset, uint count);
ulong   __attribute__((overloadable)) bitfield_extract_unsigned(long    base, uint offset, uint count);
ulong2  __attribute__((overloadable)) bitfield_extract_unsigned(long2   base, uint offset, uint count);
ulong3  __attribute__((overloadable)) bitfield_extract_unsigned(long3   base, uint offset, uint count);
ulong4  __attribute__((overloadable)) bitfield_extract_unsigned(long4   base, uint offset, uint count);
ulong8  __attribute__((overloadable)) bitfield_extract_unsigned(long8   base, uint offset, uint count);
ulong16 __attribute__((overloadable)) bitfield_extract_unsigned(long16  base, uint offset, uint count);

// bfrev i8
uchar   __attribute__((overloadable)) bit_reverse(uchar   base);
uchar2  __attribute__((overloadable)) bit_reverse(uchar2  base);
uchar3  __attribute__((overloadable)) bit_reverse(uchar3  base);
uchar4  __attribute__((overloadable)) bit_reverse(uchar4  base);
uchar8  __attribute__((overloadable)) bit_reverse(uchar8  base);
uchar16 __attribute__((overloadable)) bit_reverse(uchar16 base);
char    __attribute__((overloadable)) bit_reverse(char    base);
char2   __attribute__((overloadable)) bit_reverse(char2   base);
char3   __attribute__((overloadable)) bit_reverse(char3   base);
char4   __attribute__((overloadable)) bit_reverse(char4   base);
char8   __attribute__((overloadable)) bit_reverse(char8   base);
char16  __attribute__((overloadable)) bit_reverse(char16  base);

// bfrev i16
ushort   __attribute__((overloadable)) bit_reverse(ushort   base);
ushort2  __attribute__((overloadable)) bit_reverse(ushort2  base);
ushort3  __attribute__((overloadable)) bit_reverse(ushort3  base);
ushort4  __attribute__((overloadable)) bit_reverse(ushort4  base);
ushort8  __attribute__((overloadable)) bit_reverse(ushort8  base);
ushort16 __attribute__((overloadable)) bit_reverse(ushort16 base);
short    __attribute__((overloadable)) bit_reverse(short    base);
short2   __attribute__((overloadable)) bit_reverse(short2   base);
short3   __attribute__((overloadable)) bit_reverse(short3   base);
short4   __attribute__((overloadable)) bit_reverse(short4   base);
short8   __attribute__((overloadable)) bit_reverse(short8   base);
short16  __attribute__((overloadable)) bit_reverse(short16  base);

// bfrev i32
uint   __attribute__((overloadable)) bit_reverse(uint   base);
uint2  __attribute__((overloadable)) bit_reverse(uint2  base);
uint3  __attribute__((overloadable)) bit_reverse(uint3  base);
uint4  __attribute__((overloadable)) bit_reverse(uint4  base);
uint8  __attribute__((overloadable)) bit_reverse(uint8  base);
uint16 __attribute__((overloadable)) bit_reverse(uint16 base);
int    __attribute__((overloadable)) bit_reverse(int    base);
int2   __attribute__((overloadable)) bit_reverse(int2   base);
int3   __attribute__((overloadable)) bit_reverse(int3   base);
int4   __attribute__((overloadable)) bit_reverse(int4   base);
int8   __attribute__((overloadable)) bit_reverse(int8   base);
int16  __attribute__((overloadable)) bit_reverse(int16  base);

// bfrev i64
ulong   __attribute__((overloadable)) bit_reverse(ulong   base);
ulong2  __attribute__((overloadable)) bit_reverse(ulong2  base);
ulong3  __attribute__((overloadable)) bit_reverse(ulong3  base);
ulong4  __attribute__((overloadable)) bit_reverse(ulong4  base);
ulong8  __attribute__((overloadable)) bit_reverse(ulong8  base);
ulong16 __attribute__((overloadable)) bit_reverse(ulong16 base);
long    __attribute__((overloadable)) bit_reverse(long    base);
long2   __attribute__((overloadable)) bit_reverse(long2   base);
long3   __attribute__((overloadable)) bit_reverse(long3   base);
long4   __attribute__((overloadable)) bit_reverse(long4   base);
long8   __attribute__((overloadable)) bit_reverse(long8   base);
long16  __attribute__((overloadable)) bit_reverse(long16  base);
#endif // defined(cl_khr_extended_bit_ops)

#if defined(cl_intel_bit_instructions)
// bfi i8
uchar   __attribute__((overloadable)) intel_bfi(uchar   base, uchar   insert, uint offset, uint count);
uchar2  __attribute__((overloadable)) intel_bfi(uchar2  base, uchar2  insert, uint offset, uint count);
uchar3  __attribute__((overloadable)) intel_bfi(uchar3  base, uchar3  insert, uint offset, uint count);
uchar4  __attribute__((overloadable)) intel_bfi(uchar4  base, uchar4  insert, uint offset, uint count);
uchar8  __attribute__((overloadable)) intel_bfi(uchar8  base, uchar8  insert, uint offset, uint count);
uchar16 __attribute__((overloadable)) intel_bfi(uchar16 base, uchar16 insert, uint offset, uint count);
char    __attribute__((overloadable)) intel_bfi(char    base, char    insert, uint offset, uint count);
char2   __attribute__((overloadable)) intel_bfi(char2   base, char2   insert, uint offset, uint count);
char3   __attribute__((overloadable)) intel_bfi(char3   base, char3   insert, uint offset, uint count);
char4   __attribute__((overloadable)) intel_bfi(char4   base, char4   insert, uint offset, uint count);
char8   __attribute__((overloadable)) intel_bfi(char8   base, char8   insert, uint offset, uint count);
char16  __attribute__((overloadable)) intel_bfi(char16  base, char16  insert, uint offset, uint count);

// bfi i16
ushort   __attribute__((overloadable)) intel_bfi(ushort   base, ushort   insert, uint offset, uint count);
ushort2  __attribute__((overloadable)) intel_bfi(ushort2  base, ushort2  insert, uint offset, uint count);
ushort3  __attribute__((overloadable)) intel_bfi(ushort3  base, ushort3  insert, uint offset, uint count);
ushort4  __attribute__((overloadable)) intel_bfi(ushort4  base, ushort4  insert, uint offset, uint count);
ushort8  __attribute__((overloadable)) intel_bfi(ushort8  base, ushort8  insert, uint offset, uint count);
ushort16 __attribute__((overloadable)) intel_bfi(ushort16 base, ushort16 insert, uint offset, uint count);
short    __attribute__((overloadable)) intel_bfi(short    base, short    insert, uint offset, uint count);
short2   __attribute__((overloadable)) intel_bfi(short2   base, short2   insert, uint offset, uint count);
short3   __attribute__((overloadable)) intel_bfi(short3   base, short3   insert, uint offset, uint count);
short4   __attribute__((overloadable)) intel_bfi(short4   base, short4   insert, uint offset, uint count);
short8   __attribute__((overloadable)) intel_bfi(short8   base, short8   insert, uint offset, uint count);
short16  __attribute__((overloadable)) intel_bfi(short16  base, short16  insert, uint offset, uint count);

// bfi i32
uint   __attribute__((overloadable)) intel_bfi(uint   base, uint   insert, uint offset, uint count);
uint2  __attribute__((overloadable)) intel_bfi(uint2  base, uint2  insert, uint offset, uint count);
uint3  __attribute__((overloadable)) intel_bfi(uint3  base, uint3  insert, uint offset, uint count);
uint4  __attribute__((overloadable)) intel_bfi(uint4  base, uint4  insert, uint offset, uint count);
uint8  __attribute__((overloadable)) intel_bfi(uint8  base, uint8  insert, uint offset, uint count);
uint16 __attribute__((overloadable)) intel_bfi(uint16 base, uint16 insert, uint offset, uint count);
int    __attribute__((overloadable)) intel_bfi(int    base, int    insert, uint offset, uint count);
int2   __attribute__((overloadable)) intel_bfi(int2   base, int2   insert, uint offset, uint count);
int3   __attribute__((overloadable)) intel_bfi(int3   base, int3   insert, uint offset, uint count);
int4   __attribute__((overloadable)) intel_bfi(int4   base, int4   insert, uint offset, uint count);
int8   __attribute__((overloadable)) intel_bfi(int8   base, int8   insert, uint offset, uint count);
int16  __attribute__((overloadable)) intel_bfi(int16  base, int16  insert, uint offset, uint count);

// bfi i64
ulong   __attribute__((overloadable)) intel_bfi(ulong   base, ulong   insert, uint offset, uint count);
ulong2  __attribute__((overloadable)) intel_bfi(ulong2  base, ulong2  insert, uint offset, uint count);
ulong3  __attribute__((overloadable)) intel_bfi(ulong3  base, ulong3  insert, uint offset, uint count);
ulong4  __attribute__((overloadable)) intel_bfi(ulong4  base, ulong4  insert, uint offset, uint count);
ulong8  __attribute__((overloadable)) intel_bfi(ulong8  base, ulong8  insert, uint offset, uint count);
ulong16 __attribute__((overloadable)) intel_bfi(ulong16 base, ulong16 insert, uint offset, uint count);
long    __attribute__((overloadable)) intel_bfi(long    base, long    insert, uint offset, uint count);
long2   __attribute__((overloadable)) intel_bfi(long2   base, long2   insert, uint offset, uint count);
long3   __attribute__((overloadable)) intel_bfi(long3   base, long3   insert, uint offset, uint count);
long4   __attribute__((overloadable)) intel_bfi(long4   base, long4   insert, uint offset, uint count);
long8   __attribute__((overloadable)) intel_bfi(long8   base, long8   insert, uint offset, uint count);
long16  __attribute__((overloadable)) intel_bfi(long16  base, long16  insert, uint offset, uint count);

// sbfe i8
char   __attribute__((overloadable)) intel_sbfe(uchar   base, uint offset, uint count);
char2  __attribute__((overloadable)) intel_sbfe(uchar2  base, uint offset, uint count);
char3  __attribute__((overloadable)) intel_sbfe(uchar3  base, uint offset, uint count);
char4  __attribute__((overloadable)) intel_sbfe(uchar4  base, uint offset, uint count);
char8  __attribute__((overloadable)) intel_sbfe(uchar8  base, uint offset, uint count);
char16 __attribute__((overloadable)) intel_sbfe(uchar16 base, uint offset, uint count);
char   __attribute__((overloadable)) intel_sbfe(char    base, uint offset, uint count);
char2  __attribute__((overloadable)) intel_sbfe(char2   base, uint offset, uint count);
char3  __attribute__((overloadable)) intel_sbfe(char3   base, uint offset, uint count);
char4  __attribute__((overloadable)) intel_sbfe(char4   base, uint offset, uint count);
char8  __attribute__((overloadable)) intel_sbfe(char8   base, uint offset, uint count);
char16 __attribute__((overloadable)) intel_sbfe(char16  base, uint offset, uint count);

// sbfe i16
short   __attribute__((overloadable)) intel_sbfe(ushort   base, uint offset, uint count);
short2  __attribute__((overloadable)) intel_sbfe(ushort2  base, uint offset, uint count);
short3  __attribute__((overloadable)) intel_sbfe(ushort3  base, uint offset, uint count);
short4  __attribute__((overloadable)) intel_sbfe(ushort4  base, uint offset, uint count);
short8  __attribute__((overloadable)) intel_sbfe(ushort8  base, uint offset, uint count);
short16 __attribute__((overloadable)) intel_sbfe(ushort16 base, uint offset, uint count);
short   __attribute__((overloadable)) intel_sbfe(short    base, uint offset, uint count);
short2  __attribute__((overloadable)) intel_sbfe(short2   base, uint offset, uint count);
short3  __attribute__((overloadable)) intel_sbfe(short3   base, uint offset, uint count);
short4  __attribute__((overloadable)) intel_sbfe(short4   base, uint offset, uint count);
short8  __attribute__((overloadable)) intel_sbfe(short8   base, uint offset, uint count);
short16 __attribute__((overloadable)) intel_sbfe(short16  base, uint offset, uint count);

// sbfe i32
int   __attribute__((overloadable)) intel_sbfe(uint   base, uint offset, uint count);
int2  __attribute__((overloadable)) intel_sbfe(uint2  base, uint offset, uint count);
int3  __attribute__((overloadable)) intel_sbfe(uint3  base, uint offset, uint count);
int4  __attribute__((overloadable)) intel_sbfe(uint4  base, uint offset, uint count);
int8  __attribute__((overloadable)) intel_sbfe(uint8  base, uint offset, uint count);
int16 __attribute__((overloadable)) intel_sbfe(uint16 base, uint offset, uint count);
int   __attribute__((overloadable)) intel_sbfe(int    base, uint offset, uint count);
int2  __attribute__((overloadable)) intel_sbfe(int2   base, uint offset, uint count);
int3  __attribute__((overloadable)) intel_sbfe(int3   base, uint offset, uint count);
int4  __attribute__((overloadable)) intel_sbfe(int4   base, uint offset, uint count);
int8  __attribute__((overloadable)) intel_sbfe(int8   base, uint offset, uint count);
int16 __attribute__((overloadable)) intel_sbfe(int16  base, uint offset, uint count);

// sbfe i64
long   __attribute__((overloadable)) intel_sbfe(ulong   base, uint offset, uint count);
long2  __attribute__((overloadable)) intel_sbfe(ulong2  base, uint offset, uint count);
long3  __attribute__((overloadable)) intel_sbfe(ulong3  base, uint offset, uint count);
long4  __attribute__((overloadable)) intel_sbfe(ulong4  base, uint offset, uint count);
long8  __attribute__((overloadable)) intel_sbfe(ulong8  base, uint offset, uint count);
long16 __attribute__((overloadable)) intel_sbfe(ulong16 base, uint offset, uint count);
long   __attribute__((overloadable)) intel_sbfe(long    base, uint offset, uint count);
long2  __attribute__((overloadable)) intel_sbfe(long2   base, uint offset, uint count);
long3  __attribute__((overloadable)) intel_sbfe(long3   base, uint offset, uint count);
long4  __attribute__((overloadable)) intel_sbfe(long4   base, uint offset, uint count);
long8  __attribute__((overloadable)) intel_sbfe(long8   base, uint offset, uint count);
long16 __attribute__((overloadable)) intel_sbfe(long16  base, uint offset, uint count);


// ubfe i8
uchar   __attribute__((overloadable)) intel_ubfe(uchar   base, uint offset, uint count);
uchar2  __attribute__((overloadable)) intel_ubfe(uchar2  base, uint offset, uint count);
uchar3  __attribute__((overloadable)) intel_ubfe(uchar3  base, uint offset, uint count);
uchar4  __attribute__((overloadable)) intel_ubfe(uchar4  base, uint offset, uint count);
uchar8  __attribute__((overloadable)) intel_ubfe(uchar8  base, uint offset, uint count);
uchar16 __attribute__((overloadable)) intel_ubfe(uchar16 base, uint offset, uint count);
uchar   __attribute__((overloadable)) intel_ubfe(char    base, uint offset, uint count);
uchar2  __attribute__((overloadable)) intel_ubfe(char2   base, uint offset, uint count);
uchar3  __attribute__((overloadable)) intel_ubfe(char3   base, uint offset, uint count);
uchar4  __attribute__((overloadable)) intel_ubfe(char4   base, uint offset, uint count);
uchar8  __attribute__((overloadable)) intel_ubfe(char8   base, uint offset, uint count);
uchar16 __attribute__((overloadable)) intel_ubfe(char16  base, uint offset, uint count);

// ubfe i16
ushort   __attribute__((overloadable)) intel_ubfe(ushort   base, uint offset, uint count);
ushort2  __attribute__((overloadable)) intel_ubfe(ushort2  base, uint offset, uint count);
ushort3  __attribute__((overloadable)) intel_ubfe(ushort3  base, uint offset, uint count);
ushort4  __attribute__((overloadable)) intel_ubfe(ushort4  base, uint offset, uint count);
ushort8  __attribute__((overloadable)) intel_ubfe(ushort8  base, uint offset, uint count);
ushort16 __attribute__((overloadable)) intel_ubfe(ushort16 base, uint offset, uint count);
ushort   __attribute__((overloadable)) intel_ubfe(short    base, uint offset, uint count);
ushort2  __attribute__((overloadable)) intel_ubfe(short2   base, uint offset, uint count);
ushort3  __attribute__((overloadable)) intel_ubfe(short3   base, uint offset, uint count);
ushort4  __attribute__((overloadable)) intel_ubfe(short4   base, uint offset, uint count);
ushort8  __attribute__((overloadable)) intel_ubfe(short8   base, uint offset, uint count);
ushort16 __attribute__((overloadable)) intel_ubfe(short16  base, uint offset, uint count);

// ubfe i32
uint   __attribute__((overloadable)) intel_ubfe(uint   base, uint offset, uint count);
uint2  __attribute__((overloadable)) intel_ubfe(uint2  base, uint offset, uint count);
uint3  __attribute__((overloadable)) intel_ubfe(uint3  base, uint offset, uint count);
uint4  __attribute__((overloadable)) intel_ubfe(uint4  base, uint offset, uint count);
uint8  __attribute__((overloadable)) intel_ubfe(uint8  base, uint offset, uint count);
uint16 __attribute__((overloadable)) intel_ubfe(uint16 base, uint offset, uint count);
uint   __attribute__((overloadable)) intel_ubfe(int    base, uint offset, uint count);
uint2  __attribute__((overloadable)) intel_ubfe(int2   base, uint offset, uint count);
uint3  __attribute__((overloadable)) intel_ubfe(int3   base, uint offset, uint count);
uint4  __attribute__((overloadable)) intel_ubfe(int4   base, uint offset, uint count);
uint8  __attribute__((overloadable)) intel_ubfe(int8   base, uint offset, uint count);
uint16 __attribute__((overloadable)) intel_ubfe(int16  base, uint offset, uint count);

// ubfe i64
ulong   __attribute__((overloadable)) intel_ubfe(ulong   base, uint offset, uint count);
ulong2  __attribute__((overloadable)) intel_ubfe(ulong2  base, uint offset, uint count);
ulong3  __attribute__((overloadable)) intel_ubfe(ulong3  base, uint offset, uint count);
ulong4  __attribute__((overloadable)) intel_ubfe(ulong4  base, uint offset, uint count);
ulong8  __attribute__((overloadable)) intel_ubfe(ulong8  base, uint offset, uint count);
ulong16 __attribute__((overloadable)) intel_ubfe(ulong16 base, uint offset, uint count);
ulong   __attribute__((overloadable)) intel_ubfe(long    base, uint offset, uint count);
ulong2  __attribute__((overloadable)) intel_ubfe(long2   base, uint offset, uint count);
ulong3  __attribute__((overloadable)) intel_ubfe(long3   base, uint offset, uint count);
ulong4  __attribute__((overloadable)) intel_ubfe(long4   base, uint offset, uint count);
ulong8  __attribute__((overloadable)) intel_ubfe(long8   base, uint offset, uint count);
ulong16 __attribute__((overloadable)) intel_ubfe(long16  base, uint offset, uint count);

// bfrev i8
uchar   __attribute__((overloadable)) intel_bfrev(uchar   base);
uchar2  __attribute__((overloadable)) intel_bfrev(uchar2  base);
uchar3  __attribute__((overloadable)) intel_bfrev(uchar3  base);
uchar4  __attribute__((overloadable)) intel_bfrev(uchar4  base);
uchar8  __attribute__((overloadable)) intel_bfrev(uchar8  base);
uchar16 __attribute__((overloadable)) intel_bfrev(uchar16 base);
char    __attribute__((overloadable)) intel_bfrev(char    base);
char2   __attribute__((overloadable)) intel_bfrev(char2   base);
char3   __attribute__((overloadable)) intel_bfrev(char3   base);
char4   __attribute__((overloadable)) intel_bfrev(char4   base);
char8   __attribute__((overloadable)) intel_bfrev(char8   base);
char16  __attribute__((overloadable)) intel_bfrev(char16  base);

// bfrev i16
ushort   __attribute__((overloadable)) intel_bfrev(ushort   base);
ushort2  __attribute__((overloadable)) intel_bfrev(ushort2  base);
ushort3  __attribute__((overloadable)) intel_bfrev(ushort3  base);
ushort4  __attribute__((overloadable)) intel_bfrev(ushort4  base);
ushort8  __attribute__((overloadable)) intel_bfrev(ushort8  base);
ushort16 __attribute__((overloadable)) intel_bfrev(ushort16 base);
short    __attribute__((overloadable)) intel_bfrev(short    base);
short2   __attribute__((overloadable)) intel_bfrev(short2   base);
short3   __attribute__((overloadable)) intel_bfrev(short3   base);
short4   __attribute__((overloadable)) intel_bfrev(short4   base);
short8   __attribute__((overloadable)) intel_bfrev(short8   base);
short16  __attribute__((overloadable)) intel_bfrev(short16  base);

// bfrev i32
uint   __attribute__((overloadable)) intel_bfrev(uint   base);
uint2  __attribute__((overloadable)) intel_bfrev(uint2  base);
uint3  __attribute__((overloadable)) intel_bfrev(uint3  base);
uint4  __attribute__((overloadable)) intel_bfrev(uint4  base);
uint8  __attribute__((overloadable)) intel_bfrev(uint8  base);
uint16 __attribute__((overloadable)) intel_bfrev(uint16 base);
int    __attribute__((overloadable)) intel_bfrev(int    base);
int2   __attribute__((overloadable)) intel_bfrev(int2   base);
int3   __attribute__((overloadable)) intel_bfrev(int3   base);
int4   __attribute__((overloadable)) intel_bfrev(int4   base);
int8   __attribute__((overloadable)) intel_bfrev(int8   base);
int16  __attribute__((overloadable)) intel_bfrev(int16  base);

// bfrev i64
ulong   __attribute__((overloadable)) intel_bfrev(ulong   base);
ulong2  __attribute__((overloadable)) intel_bfrev(ulong2  base);
ulong3  __attribute__((overloadable)) intel_bfrev(ulong3  base);
ulong4  __attribute__((overloadable)) intel_bfrev(ulong4  base);
ulong8  __attribute__((overloadable)) intel_bfrev(ulong8  base);
ulong16 __attribute__((overloadable)) intel_bfrev(ulong16 base);
long    __attribute__((overloadable)) intel_bfrev(long    base);
long2   __attribute__((overloadable)) intel_bfrev(long2   base);
long3   __attribute__((overloadable)) intel_bfrev(long3   base);
long4   __attribute__((overloadable)) intel_bfrev(long4   base);
long8   __attribute__((overloadable)) intel_bfrev(long8   base);
long16  __attribute__((overloadable)) intel_bfrev(long16  base);
#endif // defined(cl_intel_bit_instructions)

#ifdef cl_intel_subgroups_ballot
uint intel_sub_group_ballot(bool p);
#endif


#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#ifdef cl_intel_subgroup_matrix_multiply_accumulate

/**** SIMD8 ****/

// 8-bit and 8-bit matrices:
int   __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(int   a, int8  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(int2  a, int8  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(int4  a, int8  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(int8  a, int8  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(int   a, uint8 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(int2  a, uint8 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(int4  a, uint8 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(int8  a, uint8 b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(uint  a, int8  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(uint2 a, int8  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(uint4 a, int8  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(uint8 a, int8  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(uint  a, uint8 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(uint2 a, uint8 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(uint4 a, uint8 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(uint8 a, uint8 b, int8 acc);

// 8-bit and 4-bit matrices:
int   __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(int   a, int4  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(int2  a, int4  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(int4  a, int4  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(int8  a, int4  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(int   a, uint4 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(int2  a, uint4 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(int4  a, uint4 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(int8  a, uint4 b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(uint  a, int4  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(uint2 a, int4  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(uint4 a, int4  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(uint8 a, int4  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(uint  a, uint4 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(uint2 a, uint4 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(uint4 a, uint4 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(uint8 a, uint4 b, int8 acc);

// 8-bit and 2-bit matrices:
int   __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(int   a, int2  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(int2  a, int2  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(int4  a, int2  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(int8  a, int2  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(int   a, uint2 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(int2  a, uint2 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(int4  a, uint2 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(int8  a, uint2 b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(uint  a, int2  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(uint2 a, int2  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(uint4 a, int2  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(uint8 a, int2  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(uint  a, uint2 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(uint2 a, uint2 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(uint4 a, uint2 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(uint8 a, uint2 b, int8 acc);

// 4-bit and 8-bit matrices:
int   __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(short   a, int8  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(short2  a, int8  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(short4  a, int8  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(short8  a, int8  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(short   a, uint8 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(short2  a, uint8 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(short4  a, uint8 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(short8  a, uint8 b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(ushort  a, int8  b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(ushort2 a, int8  b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(ushort4 a, int8  b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(ushort8 a, int8  b, int8 acc);

int   __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(ushort  a, uint8 b, int  acc);
int2  __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(ushort2 a, uint8 b, int2 acc);
int4  __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(ushort4 a, uint8 b, int4 acc);
int8  __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(ushort8 a, uint8 b, int8 acc);

// 2-bit and 8-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i2_i8_matrix_mad_k32(char   a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_i8_matrix_mad_k32(char2  a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_i8_matrix_mad_k32(char4  a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_i8_matrix_mad_k32(char8  a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i2_u8_matrix_mad_k32(char   a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_u8_matrix_mad_k32(char2  a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_u8_matrix_mad_k32(char4  a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_u8_matrix_mad_k32(char8  a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_i8_matrix_mad_k32(uchar  a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_i8_matrix_mad_k32(uchar2 a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_i8_matrix_mad_k32(uchar4 a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_i8_matrix_mad_k32(uchar8 a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_u8_matrix_mad_k32(uchar  a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_u8_matrix_mad_k32(uchar2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_u8_matrix_mad_k32(uchar4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_u8_matrix_mad_k32(uchar8 a, uint8 b, int8 acc);

/// double throughput versions (k64)
///
// 4-bit and 4-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(int   a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(int2  a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(int4  a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(int8  a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(int   a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(int2  a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(int4  a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(int8  a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(uint  a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(uint2 a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(uint4 a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(uint8 a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(uint  a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(uint2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(uint4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(uint8 a, uint8 b, int8 acc);

// 4-bit and 2-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(int   a, int4  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(int2  a, int4  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(int4  a, int4  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(int8  a, int4  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(int   a, uint4 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(int2  a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(int4  a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(int8  a, uint4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(uint  a, int4  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(uint2 a, int4  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(uint4 a, int4  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(uint8 a, int4  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(uint  a, uint4 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(uint2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(uint4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(uint8 a, uint4 b, int8 acc);

// 2-bit and 4-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(short   a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(short2  a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(short4  a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(short8  a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(short   a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(short2  a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(short4  a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(short8  a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(ushort  a, int8  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(ushort2 a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(ushort4 a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(ushort8 a, int8  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(ushort  a, uint8 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(ushort2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(ushort4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(ushort8 a, uint8 b, int8 acc);

// 2-bit and 2-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(short   a, int4  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(short2  a, int4  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(short4  a, int4  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(short8  a, int4  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(short   a, uint4 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(short2  a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(short4  a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(short8  a, uint4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(ushort  a, int4  b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(ushort2 a, int4  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(ushort4 a, int4  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(ushort8 a, int4  b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(ushort  a, uint4 b, int  acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(ushort2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(ushort4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(ushort8 a, uint4 b, int8 acc);

// bfloat16 and bfloat16 matrices:
float   __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(int  a, int8 b, float  acc);
float2  __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(int2 a, int8 b, float2 acc);
float4  __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(int4 a, int8 b, float4 acc);
float8  __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(int8 a, int8 b, float8 acc);

// fp16 and fp16 matrices:
float   __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(int  a, int8 b, float  acc);
float2  __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(int2 a, int8 b, float2 acc);
float4  __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(int4 a, int8 b, float4 acc);
float8  __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(int8 a, int8 b, float8 acc);


/**** SIMD16 ****/

// 8-bit and 8-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(short a, int8 b, int acc);   // M = 1
int2 __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(short2 a, int8 b, int2 acc); // M = 2
int4 __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(short4 a, int8 b, int4 acc); // M = 4
int8 __attribute__((overloadable)) intel_sub_group_i8_i8_matrix_mad_k32(short8 a, int8 b, int8 acc); // M = 8

int  __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(short a, uint8 b, int acc); // ...
int2 __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(short2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(short4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_u8_matrix_mad_k32(short8 a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(ushort a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(ushort2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(ushort4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_i8_matrix_mad_k32(ushort8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(ushort a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(ushort2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(ushort4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_u8_matrix_mad_k32(ushort8 a, uint8 b, int8 acc);

// 8-bit and 4-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(short a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(short2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(short4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_i4_matrix_mad_k32(short8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(short a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(short2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(short4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_u4_matrix_mad_k32(short8 a, uint4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(ushort a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(ushort2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(ushort4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_i4_matrix_mad_k32(ushort8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(ushort a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(ushort2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(ushort4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_u4_matrix_mad_k32(ushort8 a, uint4 b, int8 acc);

// 8-bit and 2-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(short a, int2 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(short2 a, int2 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(short4 a, int2 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_i2_matrix_mad_k32(short8 a, int2 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(short a, uint2 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(short2 a, uint2 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(short4 a, uint2 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_u2_matrix_mad_k32(short8 a, uint2 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(ushort a, int2 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(ushort2 a, int2 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(ushort4 a, int2 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_i2_matrix_mad_k32(ushort8 a, int2 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(ushort a, uint2 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(ushort2 a, uint2 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(ushort4 a, uint2 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_u2_matrix_mad_k32(ushort8 a, uint2 b, int8 acc);

// 4-bit and 8-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(char a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(char2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(char4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_i8_matrix_mad_k32(char8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(char a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(char2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(char4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_u8_matrix_mad_k32(char8 a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(uchar a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(uchar2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(uchar4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_i8_matrix_mad_k32(uchar8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(uchar a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(uchar2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(uchar4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_u8_matrix_mad_k32(uchar8 a, uint8 b, int8 acc);

// 4-bit and 4-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(short a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(short2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(short4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_i4_matrix_mad_k64(short8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(short a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(short2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(short4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_u4_matrix_mad_k64(short8 a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(ushort a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(ushort2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(ushort4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_i4_matrix_mad_k64(ushort8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(ushort a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(ushort2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(ushort4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_u4_matrix_mad_k64(ushort8 a, uint8 b, int8 acc);

// 4-bit and 2-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(short a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(short2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(short4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_i2_matrix_mad_k64(short8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(short a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(short2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(short4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i4_u2_matrix_mad_k64(short8 a, uint4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(ushort a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(ushort2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(ushort4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_i2_matrix_mad_k64(ushort8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(ushort a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(ushort2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(ushort4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u4_u2_matrix_mad_k64(ushort8 a, uint4 b, int8 acc);

// 2-bit and 4-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(char a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(char2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(char4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_i4_matrix_mad_k64(char8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(char a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(char2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(char4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_u4_matrix_mad_k64(char8 a, uint8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(uchar a, int8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(uchar2 a, int8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(uchar4 a, int8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_i4_matrix_mad_k64(uchar8 a, int8 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(uchar a, uint8 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(uchar2 a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(uchar4 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_u4_matrix_mad_k64(uchar8 a, uint8 b, int8 acc);

// 2-bit and 2-bit matrices:
int  __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(char a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(char2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(char4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_i2_matrix_mad_k64(char8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(char a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(char2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(char4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i2_u2_matrix_mad_k64(char8 a, uint4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(uchar a, int4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(uchar2 a, int4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(uchar4 a, int4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_i2_matrix_mad_k64(uchar8 a, int4 b, int8 acc);

int  __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(uchar a, uint4 b, int acc);
int2 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(uchar2 a, uint4 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(uchar4 a, uint4 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u2_u2_matrix_mad_k64(uchar8 a, uint4 b, int8 acc);

// bfloat16 and bfloat16 matrices,
// float acc, float return type:
float  __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short a, int8 b, float acc);
float2 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short2 a, int8 b, float2 acc);
float4 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short4 a, int8 b, float4 acc);
float8 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short8 a, int8 b, float8 acc);

// bfloat16 and bfloat16 matrices,
// bfloat16 acc, bfloat16 return type:
short  __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short a, int8 b, short acc);
short2 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short2 a, int8 b, short2 acc);
short4 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short4 a, int8 b, short4 acc);
short8 __attribute__((overloadable)) intel_sub_group_bf16_bf16_matrix_mad_k16(short8 a, int8 b, short8 acc);

#ifdef cl_khr_fp16

// fp16 and fp16 matrices,
// fp16 acc, fp16 return type:
half  __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short a, int8 b, half acc);
half2 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short2 a, int8 b, half2 acc);
half4 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short4 a, int8 b, half4 acc);
half8 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short8 a, int8 b, half8 acc);

#endif // cl_khr_fp16

// fp16 and fp16 matrices,
// float acc, float return type:
float  __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short a, int8 b, float acc);
float2 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short2 a, int8 b, float2 acc);
float4 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short4 a, int8 b, float4 acc);
float8 __attribute__((overloadable)) intel_sub_group_f16_f16_matrix_mad_k16(short8 a, int8 b, float8 acc);

//  bf <-> float conversion
short   __attribute__((overloadable)) intel_convert_f32_to_bf16(float   source);
short2  __attribute__((overloadable)) intel_convert_f32_to_bf16(float2  source);
short3  __attribute__((overloadable)) intel_convert_f32_to_bf16(float3  source);
short4  __attribute__((overloadable)) intel_convert_f32_to_bf16(float4  source);
short8  __attribute__((overloadable)) intel_convert_f32_to_bf16(float8  source);
short16 __attribute__((overloadable)) intel_convert_f32_to_bf16(float16 source);

float   __attribute__((overloadable)) intel_convert_bf16_to_f32(short   source);
float2  __attribute__((overloadable)) intel_convert_bf16_to_f32(short2  source);
float3  __attribute__((overloadable)) intel_convert_bf16_to_f32(short3  source);
float4  __attribute__((overloadable)) intel_convert_bf16_to_f32(short4  source);
float8  __attribute__((overloadable)) intel_convert_bf16_to_f32(short8  source);
float16 __attribute__((overloadable)) intel_convert_bf16_to_f32(short16 source);

int   __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float   a, float   b);
int2  __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float2  a, float2  b);
int3  __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float3  a, float3  b);
int4  __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float4  a, float4  b);
int8  __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float8  a, float8  b);
int16 __attribute__((overloadable)) intel_convert_f32_to_bf16_packed(float16 a, float16 b);

#ifdef cl_intel_subgroup_matrix_multiply_accumulate_tf32

// A: tf32, even rows in lower 8 SIMD channels, odd rows in upper 8 SIMD channels
// B: tf32
// ACC: float
// DST: float

// M = 1, K = 8, N = 16, upper 8 channels of a ignored
float  __attribute__((overloadable)) intel_sub_group_tf32_tf32_matrix_mad_k8(float  a, float8 b, float  acc);

// M = 2, K = 8, N = 16, all channels of a are used
float2 __attribute__((overloadable)) intel_sub_group_tf32_tf32_matrix_mad_k8(float  a, float8 b, float2 acc);

// M = 4, K = 8, N = 16
float4 __attribute__((overloadable)) intel_sub_group_tf32_tf32_matrix_mad_k8(float2 a, float8 b, float4 acc);

// M = 8, K = 8, N = 16
float8 __attribute__((overloadable)) intel_sub_group_tf32_tf32_matrix_mad_k8(float4 a, float8 b, float8 acc);

// Conversions
float   intel_convert_tfloat32_as_float(    float   source);
float2  intel_convert_tfloat322_as_float2(  float2  source);
float3  intel_convert_tfloat323_as_float3(  float3  source);
float4  intel_convert_tfloat324_as_float4(  float4  source);
float8  intel_convert_tfloat328_as_float8(  float8  source);
float16 intel_convert_tfloat3216_as_float16(float16 source);

#endif // cl_intel_subgroup_matrix_multiply_accumulate_tf32

#ifdef cl_intel_subgroup_matrix_multiply_accumulate_bf8

// A: bfloat8 B: bfloat8 ACC: float DST: float
float  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short  a, int8 b, float  acc);
float2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short2 a, int8 b, float2 acc);
float4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short4 a, int8 b, float4 acc);
float8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short8 a, int8 b, float8 acc);

// A: bfloat8 B: bfloat8 ACC: bfloat16 DST: float
float  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short  a, int8 b, short  acc);
float2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short2 a, int8 b, short2 acc);
float4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short4 a, int8 b, short4 acc);
float8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short8 a, int8 b, short8 acc);

// A: bfloat8 B: bfloat8 ACC: float DST: bfloat16
short  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short  a, int8 b, float  acc);
short2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short2 a, int8 b, float2 acc);
short4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short4 a, int8 b, float4 acc);
short8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short8 a, int8 b, float8 acc);

// A: bfloat8 B: bfloat8 ACC: bfloat16 DST: bfloat16
short  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short  a, int8 b, short  acc);
short2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short2 a, int8 b, short2 acc);
short4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short4 a, int8 b, short4 acc);
short8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short8 a, int8 b, short8 acc);

#ifdef cl_khr_fp16

// A: bfloat8 B: bfloat8 ACC: half DST: float
float  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short  a, int8 b, half  acc);
float2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short2 a, int8 b, half2 acc);
float4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short4 a, int8 b, half4 acc);
float8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f32(short8 a, int8 b, half8 acc);

// A: bfloat8 B: bfloat8 ACC: half DST: bfloat16
short  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short  a, int8 b, half  acc);
short2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short2 a, int8 b, half2 acc);
short4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short4 a, int8 b, half4 acc);
short8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_bf16(short8 a, int8 b, half8 acc);

// A: bfloat8 B: bfloat8 ACC: float DST: half
half  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short  a, int8 b, float  acc);
half2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short2 a, int8 b, float2 acc);
half4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short4 a, int8 b, float4 acc);
half8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short8 a, int8 b, float8 acc);

// A: bfloat8 B: bfloat8 ACC: half DST: half
half  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short  a, int8 b, half  acc);
half2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short2 a, int8 b, half2 acc);
half4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short4 a, int8 b, half4 acc);
half8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short8 a, int8 b, half8 acc);

// A: bfloat8 B: bfloat8 ACC: bfloat16 DST: half
half  __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short  a, int8 b, short  acc);
half2 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short2 a, int8 b, short2 acc);
half4 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short4 a, int8 b, short4 acc);
half8 __attribute__((overloadable)) intel_sub_group_bf8_bf8_matrix_mad_k32_f16(short8 a, int8 b, short8 acc);

// Conversions
char   __attribute__((overloadable)) intel_convert_f16_to_bf8(half source);
char2  __attribute__((overloadable)) intel_convert_f16_to_bf8(half2 source);
char3  __attribute__((overloadable)) intel_convert_f16_to_bf8(half3 source);
char4  __attribute__((overloadable)) intel_convert_f16_to_bf8(half4 source);
char8  __attribute__((overloadable)) intel_convert_f16_to_bf8(half8 source);
char16 __attribute__((overloadable)) intel_convert_f16_to_bf8(half16 source);

half   __attribute__((overloadable)) intel_convert_bf8_to_f16(char source);
half2  __attribute__((overloadable)) intel_convert_bf8_to_f16(char2 source);
half3  __attribute__((overloadable)) intel_convert_bf8_to_f16(char3 source);
half4  __attribute__((overloadable)) intel_convert_bf8_to_f16(char4 source);
half8  __attribute__((overloadable)) intel_convert_bf8_to_f16(char8 source);
half16 __attribute__((overloadable)) intel_convert_bf8_to_f16(char16 source);

#ifdef cl_intel_stochastic_rounding

// stochastic rounding
uchar   __attribute__((overloadable)) intel_convert_bfloat8_as_uchar_srnd(half       source, uchar   random);
uchar2  __attribute__((overloadable)) intel_convert_bfloat82_as_uchar2_srnd(half2    source, uchar2  random);
uchar3  __attribute__((overloadable)) intel_convert_bfloat83_as_uchar3_srnd(half3    source, uchar3  random);
uchar4  __attribute__((overloadable)) intel_convert_bfloat84_as_uchar4_srnd(half4    source, uchar4  random);
uchar8  __attribute__((overloadable)) intel_convert_bfloat88_as_uchar8_srnd(half8    source, uchar8  random);
uchar16 __attribute__((overloadable)) intel_convert_bfloat816_as_uchar16_srnd(half16 source, uchar16 random);

half   __attribute__((overloadable)) intel_convert_half_srnd(float     source, ushort   random);
half2  __attribute__((overloadable)) intel_convert_half2_srnd(float2   source, ushort2  random);
half3  __attribute__((overloadable)) intel_convert_half3_srnd(float3   source, ushort3  random);
half4  __attribute__((overloadable)) intel_convert_half4_srnd(float4   source, ushort4  random);
half8  __attribute__((overloadable)) intel_convert_half8_srnd(float8   source, ushort8  random);
half16 __attribute__((overloadable)) intel_convert_half16_srnd(float16 source, ushort16 random);
#endif // cl_intel_stochastic_rounding

#endif // cl_khr_fp16

#endif // cl_intel_subgroup_matrix_multiply_accumulate_bf8

#endif // cl_intel_subgroup_matrix_multiply_accumulate

#ifdef cl_intel_subgroup_split_matrix_multiply_accumulate

/// split matrix (dpasw, simd8 only)
int2 __attribute__((overloadable)) intel_sub_group_i8_i8_split_matrix_mad_k32(int   a, int8  b, int2 acc);  // M = 2
int4 __attribute__((overloadable)) intel_sub_group_i8_i8_split_matrix_mad_k32(int2  a, int8  b, int4 acc);  // M = 4
int8 __attribute__((overloadable)) intel_sub_group_i8_i8_split_matrix_mad_k32(int4  a, int8  b, int8 acc);  // M = 8

int2 __attribute__((overloadable)) intel_sub_group_i8_u8_split_matrix_mad_k32(int   a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_i8_u8_split_matrix_mad_k32(int2  a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_i8_u8_split_matrix_mad_k32(int4  a, uint8 b, int8 acc);

int2 __attribute__((overloadable)) intel_sub_group_u8_i8_split_matrix_mad_k32(uint  a, int8  b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_i8_split_matrix_mad_k32(uint2 a, int8  b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_i8_split_matrix_mad_k32(uint4 a, int8  b, int8 acc);

int2 __attribute__((overloadable)) intel_sub_group_u8_u8_split_matrix_mad_k32(uint  a, uint8 b, int2 acc);
int4 __attribute__((overloadable)) intel_sub_group_u8_u8_split_matrix_mad_k32(uint2 a, uint8 b, int4 acc);
int8 __attribute__((overloadable)) intel_sub_group_u8_u8_split_matrix_mad_k32(uint4 a, uint8 b, int8 acc);

// bfloat16 and bfloat16 matrices:
float2 __attribute__((overloadable)) intel_sub_group_bf16_bf16_split_matrix_mad_k16(int  a, int8 b, float2 acc);
float4 __attribute__((overloadable)) intel_sub_group_bf16_bf16_split_matrix_mad_k16(int2 a, int8 b, float4 acc);
float8 __attribute__((overloadable)) intel_sub_group_bf16_bf16_split_matrix_mad_k16(int4 a, int8 b, float8 acc);

// fp16 and fp16 matrices:
float2 __attribute__((overloadable)) intel_sub_group_f16_f16_split_matrix_mad_k16(int  a, int8 b, float2 acc);
float4 __attribute__((overloadable)) intel_sub_group_f16_f16_split_matrix_mad_k16(int2 a, int8 b, float4 acc);
float8 __attribute__((overloadable)) intel_sub_group_f16_f16_split_matrix_mad_k16(int4 a, int8 b, float8 acc);

#endif // cl_intel_subgroup_split_matrix_multiply_accumulate

//atomic fadd/fsub
#if defined(cl_intel_global_float_atomics)
float __attribute__((overloadable)) atom_add(volatile __global float *p, float val);
float __attribute__((overloadable)) atomic_add(volatile __global float *p, float val);
float __attribute__((overloadable)) atom_sub(volatile __global float *p, float val);
float __attribute__((overloadable)) atomic_sub(volatile __global float *p, float val);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) atomic_fetch_add(volatile __global atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_add_explicit(volatile __global atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_add_explicit(volatile __global atomic_float *object, float operand, memory_order order, memory_scope scope);
float __attribute__((overloadable)) atomic_fetch_sub(volatile __global atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_sub_explicit(volatile __global atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_sub_explicit(volatile __global atomic_float *object, float operand, memory_order order, memory_scope scope);
#endif // CL_VERSION_2_0
#endif //defined(cl_intel_global_float_atomics)


#ifdef cl_intel_subgroup_extended_block_read
ushort2  intel_subgroup_block_read_u8_m1k32v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort4  intel_subgroup_block_read_u8_m2k32v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort8  intel_subgroup_block_read_u8_m4k32v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort16 intel_subgroup_block_read_u8_m8k32v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort2  intel_subgroup_block_read_u16_m1k16v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort4  intel_subgroup_block_read_u16_m2k16v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort8  intel_subgroup_block_read_u16_m4k16v2(__global void *base_address, int width, int height, int pitch, int2 coord);
ushort16 intel_subgroup_block_read_u16_m8k16v2(__global void *base_address, int width, int height, int pitch, int2 coord);
uint8    intel_subgroup_block_read_transform_u8_k32(__global void *base_address, int width, int height, int pitch, int2 coord);
uint8    intel_subgroup_block_read_transform_u16_k16(__global void *base_address, int width, int height, int pitch, int2 coord);
uint8    intel_subgroup_block_read_transpose_u32_k8(__global void *base_address, int width, int height, int pitch, int2 coord);
ulong4   intel_subgroup_block_read_transpose_u64_k4(__global void *base_address, int width, int height, int pitch, int2 coord);

#endif //defined(cl_intel_subgroup_extended_block_read)

#ifdef cl_intel_subgroup_extended_block_read_cacheopts
#ifndef READ_CACHE_CONTROL_TYPE
#define READ_CACHE_CONTROL_TYPE
typedef enum
{
    read_cache_control_default_intel,
    read_cache_control_l1_uncached_l3_uncached_intel,
    read_cache_control_l1_uncached_l3_cached_intel,
    read_cache_control_l1_cached_l3_uncached_intel,
    read_cache_control_l1_cached_l3_cached_intel,
    read_cache_control_l1_streaming_l3_uncached_intel,
    read_cache_control_l1_streaming_l3_cached_intel,
    read_cache_control_l1_iar_l3_cached_intel // iar - invalidate after read
} intel_read_cache_control;
#endif // READ_CACHE_CONTROL_TYPE

ushort2 intel_subgroup_block_read_cacheopts_u8_m1k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort4 intel_subgroup_block_read_cacheopts_u8_m2k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort8 intel_subgroup_block_read_cacheopts_u8_m4k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort16 intel_subgroup_block_read_cacheopts_u8_m8k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort2 intel_subgroup_block_read_cacheopts_u16_m1k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort4 intel_subgroup_block_read_cacheopts_u16_m2k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort8 intel_subgroup_block_read_cacheopts_u16_m4k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ushort16 intel_subgroup_block_read_cacheopts_u16_m8k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
uint8 intel_subgroup_block_read_cacheopts_transform_u8_k32(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
uint8 intel_subgroup_block_read_cacheopts_transform_u16_k16(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
uint8 intel_subgroup_block_read_cacheopts_transpose_u32_k8(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
ulong4 intel_subgroup_block_read_cacheopts_transpose_u64_k4(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);

void intel_subgroup_block_prefetch_u8_m1k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u8_m2k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u8_m4k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u8_m8k32v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u16_m1k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u16_m2k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u16_m4k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_u16_m8k16v2(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_transform_u8_k32(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_transform_u16_k16(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_transpose_u32_k8(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
void intel_subgroup_block_prefetch_transpose_u64_k4(__global void *base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control);
#endif //defined(cl_intel_subgroup_extended_block_read_cacheopts)

#ifdef cl_intel_subgroup_extended_block_write_cacheopts
typedef enum
{
    write_cache_control_default_intel,
    write_cache_control_l1_uncached_l3_uncached_intel,
    write_cache_control_l1_uncached_l3_writeback_intel,
    write_cache_control_l1_writethrough_l3_uncached_intel,
    write_cache_control_l1_writethrough_l3_writeback_intel,
    write_cache_control_l1_streaming_l3_uncached_intel,
    write_cache_control_l1_streaming_l3_writeback_intel,
    write_cache_control_l1_writeback_l3_writeback_intel,
} intel_write_cache_control;

void intel_subgroup_block_write_cacheopts_u8_m1k32v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort  val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u8_m2k32v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort2 val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u8_m4k32v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort4 val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u8_m8k32v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort8 val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u16_m1k16v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort  val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u16_m2k16v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort2 val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u16_m4k16v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort4 val, intel_write_cache_control cache_control);
void intel_subgroup_block_write_cacheopts_u16_m8k16v1(__global void* base_address, int width, int height, int pitch, int2 coord, ushort8 val, intel_write_cache_control cache_control);
#endif //defined(cl_intel_subgroup_extended_block_write_cacheopts)

#ifdef cl_intel_subgroup_2d_block_io

// Configurations marked with:
//  `*` - are not implemented yet
//  `^` - are implemented in opencl_cth_released.h

////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 8-bit data, Rows in [1, 2, 4, 8, 16, 32], Columns in [32]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_1r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_2r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_4r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_8r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_16r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_32r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_1r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_2r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_4r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_8r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_16r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_32r32x1c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 8-bit data, Rows in [1^, 2^, 4^, 8^, 16, 32], Columns in [32x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_16r32x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_32r32x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_16r32x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_32r32x2c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 8-bit data, Rows in [1*, 2*, 4*, 8, 16*, 32*], Columns in [16x4]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_8r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uchar* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_16r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uchar* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_32r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uchar* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_8r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_16r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 16-bit data, Rows in [1, 2, 4, 8, 16, 32], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_16r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_32r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_16r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_32r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 16-bit data, Rows in [1^, 2^, 4^, 8^, 16, 32], Columns in [16x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_16r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_32r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private ushort* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_16r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_32r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 32-bit data, Rows in [1, 2, 4, 8, 16, 32], Columns in [8]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_1r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_2r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_4r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_8r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_16r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_32r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_1r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_2r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_4r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_8r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_16r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_32r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load, 32-bit data, Rows in [1, 2, 4, 8, 16, 32], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_16r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_32r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_16r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_32r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load / Prefetch, 32-bit data, Rows in [1, 2, 4, 8, 16, 32], Columns in [8x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_1r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_2r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_4r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_8r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_16r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_32b_32r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_1r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_2r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_4r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_8r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_16r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_32r8x2c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 8-bit data, Rows in [32^], Columns in [16]:

// Implemented in opencl_cth_released.h

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 8-bit data, Rows in [32], Columns in [16x2, 16x4]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_8b_32r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_8b_32r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_32r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_32r16x4c(__global void* base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 16-bit data, Rows in [16^, 32], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_16b_32r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

// prefetches implemented above in the section with block read

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 16-bit data, Rows in [16, 32], Columns in [16x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_16b_16r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_16b_32r16x2c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

// prefetches implemented above in the section with block read

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with Transpose, 32-bit data, Rows in [16], Columns in [1*, 2*, 4*, 8^]:

// 8 columns version implemented in opencl_cth_released.h

// 1, 2 and 4 columns version TBD

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with Transpose, 32-bit data, Rows in [32], Columns in [1*, 2*, 4*, 8]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transpose_32b_32r8x1c(__global void* base_address, int width, int height, int pitch, int2 coord, __private uint* destination);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Load with Transpose, 64-bit data, Rows in [8], Columns in [1*, 2*, 4*]:

// TBD

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Store, 8-bit data, Rows in [1, 2, 4, 8], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uchar* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uchar* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uchar* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uchar* val);

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Store, 16-bit data, Rows in [1*, 2*, 4*, 8*], Columns in [16]:

// Implemented in opencl_cth_released.h

////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
// 2D Block Store, 32-bit data, Rows in [1, 2, 4, 8], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_write_32b_1r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uint* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_32b_2r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uint* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_32b_4r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uint* val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_32b_8r16x1c(__global void* base_address, int width, int height, int pitch, int2 coord, private uint* val);

////////////////////////////////////////////////////////////////

#endif //defined(cl_intel_subgroup_2d_block_io)

void global_barrier();

//
// Named Barriers definitions
//
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#ifndef NAMED_BARRIER_STRUCT_TYPE
#define NAMED_BARRIER_STRUCT_TYPE
typedef struct
{
    int count;
    int orig_count;
    int inc;
} __namedBarrier;
#endif // NAMED_BARRIER_STRUCT_TYPE
typedef __namedBarrier NamedBarrier_t;

local NamedBarrier_t* __attribute__((overloadable)) named_barrier_init(int count);

void __attribute__((overloadable)) work_group_named_barrier(local NamedBarrier_t *barrier, cl_mem_fence_flags flags);

void __attribute__((overloadable)) work_group_named_barrier(local NamedBarrier_t *barrier, cl_mem_fence_flags flags, memory_scope scope);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_intel_pvc_rt_validation)

struct rtglobals_t;
typedef __global struct rtglobals_t *rtglobals_t;
struct rtfence_t;
typedef __private struct rtfence_t *rtfence_t;

void *intel_get_rt_stack(rtglobals_t rt_dispatch_globals);

void *intel_get_thread_btd_stack(rtglobals_t rt_dispatch_globals);

void *intel_get_global_btd_stack(rtglobals_t rt_dispatch_globals);

rtfence_t intel_dispatch_trace_ray_query(
    rtglobals_t rt_dispatch_globals, uint bvh_level, uint traceRayCtrl);

void intel_rt_sync(rtfence_t fence);

global void *intel_get_implicit_dispatch_globals();

#endif // defined(cl_intel_pvc_rt_validation)

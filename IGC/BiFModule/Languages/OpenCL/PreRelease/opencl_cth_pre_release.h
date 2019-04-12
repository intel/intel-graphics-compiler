//reflect_101
#if defined (cl_intel_mirrored_repeat_101)
#define CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL 0xA
#endif
//
// Float Atomics (SKL feature)
//
#if defined (float_atomics_enable)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) atomic_fetch_min(volatile generic atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_float *object, float operand, memory_order order, memory_scope scope);

float __attribute__((overloadable)) atomic_fetch_max(volatile generic atomic_float *object, float operand);
float __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_float *object, float operand, memory_order order);
float __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_float *object, float operand, memory_order order, memory_scope scope);
#endif // CL_VERSION_2_0

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

#ifdef cl_intel_subgroups_ballot
uint intel_sub_group_ballot(bool p);
#endif

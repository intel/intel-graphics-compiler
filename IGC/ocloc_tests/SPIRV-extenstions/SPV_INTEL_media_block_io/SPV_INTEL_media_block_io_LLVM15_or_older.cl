// Tests targets spirv-extension: SPV_INTEL_media_block_io
// REQUIRES: dg2-supported, regkeys, llvm-15-or-older
// RUN: ocloc compile -file %s -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintBefore=EmitPass'" -internal_options "-cl-intel-use-bindless-mode -cl-intel-use-bindless-advanced-mode" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM

uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(int2 src_offset, int width, int height, read_only image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(int2 src_offset, int width, int height, read_only image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(int2 src_offset, int width, int height, read_only image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(int2 src_offset, int width, int height, read_only image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(int2 src_offset, int width, int height, read_only image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(int2 src_offset, int width, int height, read_only image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(int2 src_offset, int width, int height, read_only image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(int2 src_offset, int width, int height, read_only image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(int2 src_offset, int width, int height, read_only image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(int2 src_offset, int width, int height, read_only image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(int2 src_offset, int width, int height, read_only image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(int2 src_offset, int width, int height, read_only image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(int2 src_offset, int width, int height, read_only image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(int2 src_offset, int width, int height, read_only image2d_t image);

uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(int2 src_offset, int width, int height, read_write image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(int2 src_offset, int width, int height, read_write image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(int2 src_offset, int width, int height, read_write image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(int2 src_offset, int width, int height, read_write image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(int2 src_offset, int width, int height, read_write image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(int2 src_offset, int width, int height, read_write image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(int2 src_offset, int width, int height, read_write image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(int2 src_offset, int width, int height, read_write image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(int2 src_offset, int width, int height, read_write image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(int2 src_offset, int width, int height, read_write image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(int2 src_offset, int width, int height, read_write image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(int2 src_offset, int width, int height, read_write image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(int2 src_offset, int width, int height, read_write image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(int2 src_offset, int width, int height, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(int2 src_offset, int width, int height, uchar pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(int2 src_offset, int width, int height, uchar2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(int2 src_offset, int width, int height, uchar4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(int2 src_offset, int width, int height, uchar8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(int2 src_offset, int width, int height, uchar16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(int2 src_offset, int width, int height, ushort pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(int2 src_offset, int width, int height, ushort2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(int2 src_offset, int width, int height, ushort4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(int2 src_offset, int width, int height, ushort8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(int2 src_offset, int width, int height, ushort16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(int2 src_offset, int width, int height, uint pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(int2 src_offset, int width, int height, uint2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(int2 src_offset, int width, int height, uint4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(int2 src_offset, int width, int height, uint8 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(int2 src_offset, int width, int height, uchar pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(int2 src_offset, int width, int height, uchar2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(int2 src_offset, int width, int height, uchar4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(int2 src_offset, int width, int height, uchar8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(int2 src_offset, int width, int height, uchar16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(int2 src_offset, int width, int height, ushort pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(int2 src_offset, int width, int height, ushort2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(int2 src_offset, int width, int height, ushort4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(int2 src_offset, int width, int height, ushort8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(int2 src_offset, int width, int height, ushort16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(int2 src_offset, int width, int height, uint pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(int2 src_offset, int width, int height, uint2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(int2 src_offset, int width, int height, uint4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(int2 src_offset, int width, int height, uint8 pixels, read_write image2d_t image);

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
__kernel void intel_media_block_test(int2 edgeCoord, __read_only image2d_t src_luma_image,
                         __write_only image2d_t dst_luma_image) {
  // Byte sized read operations
  uchar uc =
      intel_sub_group_media_block_read_uc(edgeCoord, 4, 16, src_luma_image);
  uchar2 uc2 =
      intel_sub_group_media_block_read_uc2(edgeCoord, 4, 16, src_luma_image);
  uchar4 uc4 =
      intel_sub_group_media_block_read_uc4(edgeCoord, 4, 16, src_luma_image);
  uchar8 uc8 =
      intel_sub_group_media_block_read_uc8(edgeCoord, 4, 16, src_luma_image);
  uchar16 uc16 =
      intel_sub_group_media_block_read_uc16(edgeCoord, 4, 16, src_luma_image);

  // Word sized read operations
  ushort us =
      intel_sub_group_media_block_read_us(edgeCoord, 4, 16, src_luma_image);
  ushort2 us2 =
      intel_sub_group_media_block_read_us2(edgeCoord, 4, 16, src_luma_image);
  ushort4 us4 =
      intel_sub_group_media_block_read_us4(edgeCoord, 4, 16, src_luma_image);
  ushort8 us8 =
      intel_sub_group_media_block_read_us8(edgeCoord, 4, 16, src_luma_image);
  ushort16 us16 =
      intel_sub_group_media_block_read_us16(edgeCoord, 4, 16, src_luma_image);

  // Double Word (DWORD) sized read operations
  uint ui =
      intel_sub_group_media_block_read_ui(edgeCoord, 4, 16, src_luma_image);
  uint2 ui2 =
      intel_sub_group_media_block_read_ui2(edgeCoord, 4, 16, src_luma_image);
  uint4 ui4 =
      intel_sub_group_media_block_read_ui4(edgeCoord, 4, 16, src_luma_image);
  uint8 ui8 =
      intel_sub_group_media_block_read_ui8(edgeCoord, 4, 16, src_luma_image);

  // Byte sized write operations
  intel_sub_group_media_block_write_uc(edgeCoord, 4, 16, uc, dst_luma_image);
  intel_sub_group_media_block_write_uc2(edgeCoord, 4, 16, uc2, dst_luma_image);
  intel_sub_group_media_block_write_uc4(edgeCoord, 4, 16, uc4, dst_luma_image);
  intel_sub_group_media_block_write_uc8(edgeCoord, 4, 16, uc8, dst_luma_image);
  intel_sub_group_media_block_write_uc16(edgeCoord, 4, 16, uc16, dst_luma_image);

  // Word sized write operations
  intel_sub_group_media_block_write_us(edgeCoord, 4, 16, us, dst_luma_image);
  intel_sub_group_media_block_write_us2(edgeCoord, 4, 16, us2, dst_luma_image);
  intel_sub_group_media_block_write_us4(edgeCoord, 4, 16, us4, dst_luma_image);
  intel_sub_group_media_block_write_us8(edgeCoord, 4, 16, us8, dst_luma_image);
  intel_sub_group_media_block_write_us16(edgeCoord, 4, 16, us16, dst_luma_image);

  // Double word (DWORD) sized write operations
  intel_sub_group_media_block_write_ui(edgeCoord, 4, 16, ui, dst_luma_image);
  intel_sub_group_media_block_write_ui2(edgeCoord, 4, 16, ui2, dst_luma_image);
  intel_sub_group_media_block_write_ui4(edgeCoord, 4, 16, ui4, dst_luma_image);
  intel_sub_group_media_block_write_ui8(edgeCoord, 4, 16, ui8, dst_luma_image);
}

// CHECK-LLVM: [[EL0:%[A-Za-z0-9_.]+]] = extractelement <2 x i32> %edgeCoord, i32 0
// CHECK-LLVM: [[EL1:%[A-Za-z0-9_.]+]] = extractelement <2 x i32> %edgeCoord, i32 1
// CHECK-LLVM: [[SRC2:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL2:%[A-Za-z0-9_.]+]] = call i8 @llvm.genx.GenISA.MediaBlockRead.i8.i64(i64 [[SRC2]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC3:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL3:%[A-Za-z0-9_.]+]] = call <2 x i8> @llvm.genx.GenISA.MediaBlockRead.v2i8.i64(i64 [[SRC3]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC4:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL4:%[A-Za-z0-9_.]+]] = call <4 x i8> @llvm.genx.GenISA.MediaBlockRead.v4i8.i64(i64 [[SRC4]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC5:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL5:%[A-Za-z0-9_.]+]] = call <8 x i8> @llvm.genx.GenISA.MediaBlockRead.v8i8.i64(i64 [[SRC5]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC6:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL6:%[A-Za-z0-9_.]+]] = call <16 x i8> @llvm.genx.GenISA.MediaBlockRead.v16i8.i64(i64 [[SRC6]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC7:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL7:%[A-Za-z0-9_.]+]] = call i16 @llvm.genx.GenISA.MediaBlockRead.i16.i64(i64 [[SRC7]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC8:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL8:%[A-Za-z0-9_.]+]] = call <2 x i16> @llvm.genx.GenISA.MediaBlockRead.v2i16.i64(i64 [[SRC8]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC9:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL9:%[A-Za-z0-9_.]+]] = call <4 x i16> @llvm.genx.GenISA.MediaBlockRead.v4i16.i64(i64 [[SRC9]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC10:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL10:%[A-Za-z0-9_.]+]] = call <8 x i16> @llvm.genx.GenISA.MediaBlockRead.v8i16.i64(i64 [[SRC10]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC11:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL11:%[A-Za-z0-9_.]+]] = call <16 x i16> @llvm.genx.GenISA.MediaBlockRead.v16i16.i64(i64 [[SRC11]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC12:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL12:%[A-Za-z0-9_.]+]] = call i32 @llvm.genx.GenISA.MediaBlockRead.i32.i64(i64 [[SRC12]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC13:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL13:%[A-Za-z0-9_.]+]] = call <2 x i32> @llvm.genx.GenISA.MediaBlockRead.v2i32.i64(i64 [[SRC13]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC14:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL14:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.GenISA.MediaBlockRead.v4i32.i64(i64 [[SRC14]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[SRC15:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace\(1\)\* %src_luma_image to i64)}}
// CHECK-LLVM: [[CALL15:%[A-Za-z0-9_.]+]] = call <8 x i32> @llvm.genx.GenISA.MediaBlockRead.v8i32.i64(i64 [[SRC15]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16)
// CHECK-LLVM: [[DST2:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.i8(i64 [[DST2]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, i8 [[CALL2]])
// CHECK-LLVM: [[DST3:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v2i8(i64 [[DST3]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <2 x i8> [[CALL3]])
// CHECK-LLVM: [[DST4:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v4i8(i64 [[DST4]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <4 x i8> [[CALL4]])
// CHECK-LLVM: [[DST5:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v8i8(i64 [[DST5]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <8 x i8> [[CALL5]])
// CHECK-LLVM: [[DST6:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v16i8(i64 [[DST6]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <16 x i8> [[CALL6]])
// CHECK-LLVM: [[DST7:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.i16(i64 [[DST7]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, i16 [[CALL7]])
// CHECK-LLVM: [[DST8:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v2i16(i64 [[DST8]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <2 x i16> [[CALL8]])
// CHECK-LLVM: [[DST9:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v4i16(i64 [[DST9]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <4 x i16> [[CALL9]])
// CHECK-LLVM: [[DST10:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v8i16(i64 [[DST10]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <8 x i16> [[CALL10]])
// CHECK-LLVM: [[DST11:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v16i16(i64 [[DST11]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <16 x i16> [[CALL11]])
// CHECK-LLVM: [[DST12:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.i32(i64 [[DST12]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, i32 [[CALL12]])
// CHECK-LLVM: [[DST13:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v2i32(i64 [[DST13]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <2 x i32> [[CALL13]])
// CHECK-LLVM: [[DST14:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v4i32(i64 [[DST14]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <4 x i32> [[CALL14]])
// CHECK-LLVM: [[DST15:%[0-9]+]] = {{(bitcast <2 x i32> .* to i64|ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace\(1\)\* %dst_luma_image to i64)}}
// CHECK-LLVM: call void @llvm.genx.GenISA.MediaBlockWrite.i64.v8i32(i64 [[DST15]], i32 [[EL0]], i32 [[EL1]], i32 0, i32 4, i32 16, <8 x i32> [[CALL15]])

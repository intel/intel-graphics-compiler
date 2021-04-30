/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Device-Side Enqueue Instructions dependent on pointer size
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_IB_copyNDRangeTondrange(ndrange_t* out, Ndrange_t* in)
{
    out->workDimension       = in->dim;
    out->globalWorkOffset[0] = in->globalWorkOffset[0];
    out->globalWorkOffset[1] = in->globalWorkOffset[1];
    out->globalWorkOffset[2] = in->globalWorkOffset[2];
    out->globalWorkSize[0]   = in->globalWorkSize[0];
    out->globalWorkSize[1]   = in->globalWorkSize[1];
    out->globalWorkSize[2]   = in->globalWorkSize[2];
    out->localWorkSize[0]    = in->localWorkSize[0];
    out->localWorkSize[1]    = in->localWorkSize[1];
    out->localWorkSize[2]    = in->localWorkSize[2];
}

Ndrange_t __builtin_spirv_OpBuildNDRange_i64_i64_i64(ulong GlobalWorkSize, ulong LocalWorkSize, ulong GlobalWorkOffset)
{
  Ndrange_t range = { 1, {GlobalWorkOffset, 0, 0}, {GlobalWorkSize, 1, 1}, {LocalWorkSize, 0, 0} };
  return range;
}

Ndrange_t __builtin_spirv_OpBuildNDRange_i32_i32_i32(uint GlobalWorkSize, uint LocalWorkSize, uint GlobalWorkOffset)
{
  return __builtin_spirv_OpBuildNDRange_i64_i64_i64(GlobalWorkSize, LocalWorkSize, GlobalWorkOffset);
}

Ndrange_t __builtin_spirv_OpBuildNDRange_a2i64_a2i64_a2i64(ulong GlobalWorkSize[2], ulong LocalWorkSize[2], ulong GlobalWorkOffset[2])
{
  Ndrange_t range = {
    2,
    {GlobalWorkOffset[0], GlobalWorkOffset[1], 0},
    {GlobalWorkSize[0], GlobalWorkSize[1], 1},
    {LocalWorkSize[0], LocalWorkSize[1], 0}
  };
  return range;
}

Ndrange_t __builtin_spirv_OpBuildNDRange_a2i32_a2i32_a2i32(uint GlobalWorkSize[2], uint LocalWorkSize[2], uint GlobalWorkOffset[2])
{
  Ndrange_t range = {
    2,
    {GlobalWorkOffset[0], GlobalWorkOffset[1], 0},
    {GlobalWorkSize[0], GlobalWorkSize[1], 1},
    {LocalWorkSize[0], LocalWorkSize[1], 0}
  };
  return range;
}

Ndrange_t __builtin_spirv_OpBuildNDRange_a3i64_a3i64_a3i64(ulong GlobalWorkSize[3], ulong LocalWorkSize[3], ulong GlobalWorkOffset[3])
{
  Ndrange_t range = {
    3,
    {GlobalWorkOffset[0], GlobalWorkOffset[1], GlobalWorkOffset[2]},
    {GlobalWorkSize[0], GlobalWorkSize[1], GlobalWorkSize[2]},
    {LocalWorkSize[0], LocalWorkSize[1], LocalWorkSize[2]}
  };
  return range;
}

Ndrange_t __builtin_spirv_OpBuildNDRange_a3i32_a3i32_a3i32(uint GlobalWorkSize[3], uint LocalWorkSize[3], uint GlobalWorkOffset[3])
{
  Ndrange_t range = {
    3,
    {GlobalWorkOffset[0], GlobalWorkOffset[1], GlobalWorkOffset[2]},
    {GlobalWorkSize[0], GlobalWorkSize[1], GlobalWorkSize[2]},
    {LocalWorkSize[0], LocalWorkSize[1], LocalWorkSize[2]}
  };
  return range;
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0


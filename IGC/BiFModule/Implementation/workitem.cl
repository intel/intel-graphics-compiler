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


#define MAX_DIM 2

#define BuiltinVector(BuiltinName) \
    (size_t3)(BuiltinName(0),      \
              BuiltinName(1),      \
              BuiltinName(2))

#ifdef NO_ASSUME_SUPPORT
#    define BuiltinVectorAssumeGE0(v)
#else
#    define BuiltinVectorAssumeGE0(v) \
    __builtin_assume( (v.x) >= 0 ); \
    __builtin_assume( (v.y) >= 0 ); \
    __builtin_assume( (v.z) >= 0 );
#endif

size_t3 __builtin_spirv_BuiltInNumWorkgroups()
{
    size_t3 v = BuiltinVector(__builtin_IB_get_num_groups);

    BuiltinVectorAssumeGE0(v);
    return v;
}

size_t3 __builtin_spirv_BuiltInWorkgroupSize()
{
   size_t3 v = BuiltinVector(__builtin_IB_get_local_size);

   BuiltinVectorAssumeGE0(v);

   return v;
}

size_t3 __builtin_spirv_BuiltInWorkgroupId()
{
     size_t3 v = BuiltinVector(__builtin_IB_get_group_id);

     BuiltinVectorAssumeGE0(v);

     return v;
}

static size_t __intel_LocalInvocationId(uint dim)
{
    size_t v = 0;

    if (dim > MAX_DIM) {
        v = 0;
    } else if (dim == 0) {
        v =  __builtin_IB_get_local_id_x();
    } else if (dim == 1) {
        v =  __builtin_IB_get_local_id_y();
    } else if (dim == 2) {
        v = __builtin_IB_get_local_id_z();
    }

    // local id is a 16 bit number in curbe.
#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 0);
    __builtin_assume(v <= 0xffff);
#endif
    return v;
}

size_t3 __builtin_spirv_BuiltInLocalInvocationId()
{
    size_t3 v = (size_t3)(__builtin_IB_get_local_id_x(),
                     __builtin_IB_get_local_id_y(),
                     __builtin_IB_get_local_id_z());


    BuiltinVectorAssumeGE0(v);
#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v.x <= 0xffff);
    __builtin_assume(v.y <= 0xffff);
    __builtin_assume(v.z <= 0xffff);
#endif

    return v;
}

static size_t __intel_GlobalInvocationId(uint dim)
{
    if (dim > MAX_DIM) {
        return 0;
    }

    size_t v =
         __builtin_IB_get_group_id(dim) * __builtin_IB_get_enqueued_local_size(dim) +
         __intel_LocalInvocationId(dim) + __builtin_IB_get_global_offset(dim);

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 0);
#endif
    return v;

}

size_t3 __builtin_spirv_BuiltInGlobalInvocationId()
{
    size_t3 v = BuiltinVector(__intel_GlobalInvocationId);

    return v;
}

uint __builtin_spirv_BuiltInWorkDim()
{
    uint dim = __builtin_IB_get_work_dim();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(dim >= 0);
    __builtin_assume(dim <= 3);
#endif

    return dim;
}

size_t3 __builtin_spirv_BuiltInGlobalSize()
{
    size_t3 v = BuiltinVector(__builtin_IB_get_global_size);

    BuiltinVectorAssumeGE0(v);

    return v;
}

size_t3 __builtin_spirv_BuiltInEnqueuedWorkgroupSize()
{
    size_t3 v = BuiltinVector(__builtin_IB_get_enqueued_local_size);

    BuiltinVectorAssumeGE0(v);

    return v;
}

size_t3 __builtin_spirv_BuiltInGlobalOffset()
{
    return BuiltinVector(__builtin_IB_get_global_offset);
}

size_t __builtin_spirv_BuiltInGlobalLinearId()
{
  uint dim = __builtin_spirv_BuiltInWorkDim();
  size_t result = 0;

  switch (dim) {
    default:
    case 1:
      result = __intel_GlobalInvocationId(0) - __builtin_IB_get_global_offset(0);
      break;
    case 2:
      result = (__intel_GlobalInvocationId(1) - __builtin_IB_get_global_offset(1))*
                __builtin_IB_get_global_size(0) + (__intel_GlobalInvocationId(0) - __builtin_IB_get_global_offset(0));
      break;
    case 3:
      result = ((__intel_GlobalInvocationId(2) - __builtin_IB_get_global_offset(2)) *
                __builtin_IB_get_global_size(1) * __builtin_IB_get_global_size(0)) +
               ((__intel_GlobalInvocationId(1) - __builtin_IB_get_global_offset(1)) * __builtin_IB_get_global_size(0)) +
               (__intel_GlobalInvocationId(0) - __builtin_IB_get_global_offset(0));
      break;
  }

#ifndef NO_ASSUME_SUPPORT
  __builtin_assume(result >= 0);
#endif

  return result;
}

size_t __builtin_spirv_BuiltInLocalInvocationIndex()
{
#if 0
    // This doesn't work right now due to a bug in the runtime.
    // If/when they fix their bug we can experiment if spending the
    // register(s) for get_local_linear_id() is better than spending
    // the math to compute the linear local ID.
    return __builtin_IB_get_local_linear_id();
#else
    uint    llid;

    llid  = (uint)__intel_LocalInvocationId(2);
    llid *= (uint)__builtin_IB_get_local_size(1);
    llid += (uint)__intel_LocalInvocationId(1);
    llid *= (uint)__builtin_IB_get_local_size(0);
    llid += (uint)__intel_LocalInvocationId(0);

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(llid >= 0);
#endif

    return llid;
#endif
}

// Helper funcs ////////

uint __spirv_LocalID(uint dim)
{
    return __intel_LocalInvocationId(dim);
}

uint __spirv_WorkgroupSize()
{
    uint totalWorkGroupSize =
        (uint)__builtin_IB_get_local_size(0) *
        (uint)__builtin_IB_get_local_size(1) *
        (uint)__builtin_IB_get_local_size(2);

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(totalWorkGroupSize >= 0);
#endif
    return totalWorkGroupSize;
}

uint __spirv_EnqueuedWorkgroupSize( void )
{
    uint totalWorkGroupSize =
        (uint)__builtin_IB_get_enqueued_local_size(0) *
        (uint)__builtin_IB_get_enqueued_local_size(1) *
        (uint)__builtin_IB_get_enqueued_local_size(2);

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(totalWorkGroupSize >= 0);
#endif
    return totalWorkGroupSize;
}

uint __spirv_BuiltInLocalInvocationIndex()
{
    uint id = (uint)__builtin_spirv_BuiltInLocalInvocationIndex();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(id >= 0);
#endif

    return id;
}

////////////////////////

uint __builtin_spirv_BuiltInSubgroupMaxSize()
{
    uint v = __builtin_IB_get_simd_size();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 8);
    __builtin_assume(v <= 32);
#endif

    return v;
}

uint __builtin_spirv_BuiltInSubgroupId()
{
    uint v = (uint)__builtin_spirv_BuiltInLocalInvocationIndex() / __builtin_spirv_BuiltInSubgroupMaxSize();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 0);
    __builtin_assume(v < 32);
#endif

    return v;
}

uint __builtin_spirv_BuiltInNumSubgroups()
{
    uint totalWorkGroupSize = __spirv_WorkgroupSize() + __builtin_spirv_BuiltInSubgroupMaxSize() - 1;
    return totalWorkGroupSize / __builtin_spirv_BuiltInSubgroupMaxSize();
}

uint __builtin_spirv_BuiltInSubgroupSize()
{
    uint    remainder =
                __spirv_WorkgroupSize() & ( __builtin_spirv_BuiltInSubgroupMaxSize() - 1 );
    bool    fullSubGroup =
                ( remainder == 0 ) ||
                ( __builtin_spirv_BuiltInSubgroupId() < __builtin_spirv_BuiltInNumSubgroups() - 1 );

    return fullSubGroup ? __builtin_spirv_BuiltInSubgroupMaxSize() : remainder;
}

uint __builtin_spirv_BuiltInNumEnqueuedSubgroups()
{
    uint totalEnqueuedWorkGroupSize = __spirv_EnqueuedWorkgroupSize() + __builtin_spirv_BuiltInSubgroupMaxSize() - 1;
    return totalEnqueuedWorkGroupSize / __builtin_spirv_BuiltInSubgroupMaxSize();
}

uint __builtin_spirv_BuiltInSubgroupLocalInvocationId()
{
    return __builtin_IB_get_simd_id();
}


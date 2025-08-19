/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_intel_rt_struct_defs.h"
#include "IBiF_intel_rt_utils.h"

#if defined(cl_intel_rt_production)

void __basic_rtstack_init(
    global void* rtStack,
    global HWAccel* hwaccel,
    intel_float3 origin,
    intel_float3 direction,
    float tmin,
    float tmax,
    uint mask,
    intel_ray_flags_t flags)
{
    unsigned int    bvh_level = 0;
    /* init ray */
    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    memRay->org[0] = origin.x;
    memRay->org[1] = origin.y;
    memRay->org[2] = origin.z;
    memRay->dir[0] = direction.x;
    memRay->dir[1] = direction.y;
    memRay->dir[2] = direction.z;
    memRay->tnear  = tmin;
    memRay->tfar   = tmax;

    memRay->data[1] = 0;
    memRay->data[2] = 0;
    memRay->data[3] = 0;

    MemRay_setRootNodePtr(memRay, (ulong)hwaccel + 128);
    MemRay_setRayFlags(memRay,    flags);
    MemRay_setRayMask(memRay,     mask);

    MemHit* commitedHit = get_rt_stack_hit(rtStack, intel_hit_type_committed_hit);
    MemHit_clearUV(commitedHit);
    commitedHit->t = INFINITY;
    commitedHit->data0 = 0;
    MemHit_setValid(commitedHit, 0);
    MemHit_setDone(commitedHit, 0);

    MemHit* potentialHit = get_rt_stack_hit(rtStack, intel_hit_type_potential_hit);
    MemHit_clearUV(potentialHit);
    potentialHit->t = INFINITY;
    potentialHit->data0 = 0;
    MemHit_setValid(potentialHit, 1);
    MemHit_setDone(potentialHit, 1);
}

void __basic_ray_forward(
    global void* rtStack,
    HWAccel* hwaccel,
    uint bvhLevel,
    intel_float3 origin,
    intel_float3 direction,
    float tmin,
    float tmax,
    uint mask,
    intel_ray_flags_t flags)
{
    MemRay* memRay = get_rt_stack_ray(rtStack, bvhLevel);
    memRay->org[0] = origin.x;
    memRay->org[1] = origin.y;
    memRay->org[2] = origin.z;
    memRay->dir[0] = direction.x;
    memRay->dir[1] = direction.y;
    memRay->dir[2] = direction.z;
    memRay->tnear  = tmin;
    memRay->tfar   = tmax;

    memRay->data[1] = 0;
    memRay->data[2] = 0;
    memRay->data[3] = 0;

    MemRay_setRootNodePtr(memRay, (ulong)hwaccel + 128);
    MemRay_setRayFlags(memRay,    flags);
    MemRay_setRayMask(memRay,     mask);
}

typedef enum
{
    intel_raytracing_ext_flag_ray_query = 1 << 0,   // true if ray queries are supported
} intel_raytracing_ext_flag_t;

intel_raytracing_ext_flag_t intel_get_raytracing_ext_flag()
{
    return intel_raytracing_ext_flag_ray_query;
};

intel_ray_query_t intel_ray_query_init(
    intel_ray_desc_t ray, intel_raytracing_acceleration_structure_t accel)
{
    global HWAccel* hwaccel   = to_global((HWAccel*)accel);
    rtglobals_t     dispatchGlobalsPtr = (rtglobals_t) __getImplicitDispatchGlobals();
    global void* rtStack = to_global(__builtin_IB_intel_get_rt_stack(dispatchGlobalsPtr));

    __basic_rtstack_init(rtStack, hwaccel, ray.origin, ray.direction, ray.tmin, ray.tmax, ray.mask, ray.flags);

    intel_ray_query_t rayquery = __builtin_IB_intel_init_ray_query(
        NULL,
        dispatchGlobalsPtr,
        rtStack,
        TRACE_RAY_INITIAL,
        0
    );

    return rayquery;
}

void intel_ray_query_forward_ray(
    intel_ray_query_t                         rayquery,
    intel_ray_desc_t                          ray,
    intel_raytracing_acceleration_structure_t accel_i)
{
    HWAccel* hwaccel = (HWAccel*)accel_i;
    global void* rtStack = __builtin_IB_intel_query_rt_stack(rayquery);

    /* init ray */
    uint bvh_level = __builtin_IB_intel_query_bvh_level(rayquery) + 1;

    __basic_ray_forward(
        rtStack, hwaccel, bvh_level, ray.origin, ray.direction, ray.tmin, ray.tmax, ray.mask, ray.flags);

    __builtin_IB_intel_update_ray_query(
        rayquery,
        NULL,
        __builtin_IB_intel_query_rt_globals(rayquery),
        rtStack,
        TRACE_RAY_INSTANCE,
        bvh_level
    );
}

void intel_ray_query_commit_potential_hit(intel_ray_query_t rayquery)
{
    global void* rtStack = __builtin_IB_intel_query_rt_stack(rayquery);

    uint bvh_level = __builtin_IB_intel_query_bvh_level(rayquery);
    MemRay* memRay    = get_rt_stack_ray(rtStack, bvh_level);
    uint rflags    = MemRay_getRayFlags(memRay);

    MemHit* commitedHit = get_rt_stack_hit(rtStack, intel_hit_type_committed_hit);
    MemHit* potentialHit = get_rt_stack_hit(rtStack, intel_hit_type_potential_hit);
    if (rflags & intel_ray_flags_accept_first_hit_and_end_search)
    {
        *commitedHit = *potentialHit;
        MemHit_setValid(commitedHit, 1);

        __builtin_IB_intel_update_ray_query(
            rayquery,
            NULL,
            __builtin_IB_intel_query_rt_globals(rayquery),
            rtStack,
            TRACE_RAY_DONE,
            bvh_level
        );
    }
    else
    {
        MemHit_setValid(potentialHit, 1); // FIXME: is this required?

        __builtin_IB_intel_update_ray_query(
            rayquery,
            NULL,
            __builtin_IB_intel_query_rt_globals(rayquery),
            rtStack,
            TRACE_RAY_COMMIT,
            bvh_level
        );
    }
}

void intel_ray_query_commit_potential_hit_override(
    intel_ray_query_t rayquery, float override_hit_distance, intel_float2 override_uv)
{
    global void*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);
    MemHit* potentialHit = get_rt_stack_hit(rtStack, intel_hit_type_potential_hit);
    potentialHit->t = override_hit_distance;
    MemHit_setUV(potentialHit, override_uv.x, override_uv.y);
    intel_ray_query_commit_potential_hit(rayquery);
}

void intel_ray_query_start_traversal(intel_ray_query_t rayquery)
{
    rtglobals_t             dispatchGlobalsPtr = __builtin_IB_intel_query_rt_globals(rayquery);
    global void*            rtStack            = __builtin_IB_intel_query_rt_stack(rayquery);
    MemHit* potentialHit = get_rt_stack_hit(rtStack, intel_hit_type_potential_hit);
    MemHit_setDone(potentialHit, 1);
    MemHit_setValid(potentialHit, 1);

    TraceRayCtrl ctrl = __builtin_IB_intel_query_ctrl(rayquery);

    if (ctrl == TRACE_RAY_DONE) return;

    uint bvh_level = __builtin_IB_intel_query_bvh_level(rayquery);

    rtfence_t fence = __builtin_IB_intel_dispatch_trace_ray_query(
        dispatchGlobalsPtr, bvh_level, ctrl);

    __builtin_IB_intel_update_ray_query(
        rayquery,
        fence,
        dispatchGlobalsPtr,
        rtStack,
        ctrl,
        bvh_level
    );
}

void intel_ray_query_sync(intel_ray_query_t rayquery)
{
    rtfence_t fence = __builtin_IB_intel_query_rt_fence(rayquery);
    __builtin_IB_intel_rt_sync(fence);

    global void* rtStack = __builtin_IB_intel_query_rt_stack(rayquery);
    MemHit* potentialHit = get_rt_stack_hit(rtStack, intel_hit_type_potential_hit);
    uint bvh_level = MemHit_getBvhLevel(potentialHit);

     __builtin_IB_intel_update_ray_query(
        rayquery,
        fence,
        __builtin_IB_intel_query_rt_globals(rayquery),
        rtStack,
        TRACE_RAY_CONTINUE,
        bvh_level
    );
}

void intel_ray_query_abandon(intel_ray_query_t rayquery)
{
    intel_ray_query_sync(rayquery);

    __builtin_IB_intel_update_ray_query(
        rayquery,
        NULL,
        NULL,
        NULL,
        TRACE_RAY_INITIAL,
        0
    );
}

uint intel_get_hit_bvh_level(intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    return MemHit_getBvhLevel(get_query_hit(rayquery, hit_type));
}

float intel_get_hit_distance(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    return get_query_hit(rayquery, hit_type)->t;
}

intel_float2 intel_get_hit_barycentrics(intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit* hit = get_query_hit(rayquery, hit_type);
    return MemHit_getUV(hit);
}

bool intel_get_hit_front_face(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    return MemHit_getFrontFace(get_query_hit(rayquery, hit_type));
}

uint intel_get_hit_geometry_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit* hit = get_query_hit(rayquery, hit_type);

    PrimLeafDesc* leaf = (PrimLeafDesc*)MemHit_getPrimLeafPtr(hit);
    return PrimLeafDesc_getGeomIndex(leaf);
}

uint intel_get_hit_primitive_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*       hit  = get_query_hit(rayquery, hit_type);
    PrimLeafDesc* leaf = (PrimLeafDesc*)MemHit_getPrimLeafPtr(hit);

    if (MemHit_getLeafType(hit) == NODE_TYPE_QUAD)
        return ((QuadLeaf*)leaf)->primIndex0 + MemHit_getPrimIndexDelta(hit);
    else
        return ((ProceduralLeaf*)leaf)->_primIndex[MemHit_getPrimLeafIndex(hit)];
}

uint intel_get_hit_triangle_primitive_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*   hit  = get_query_hit(rayquery, hit_type);
    QuadLeaf* leaf = (QuadLeaf*)MemHit_getPrimLeafPtr(hit);

    return leaf->primIndex0 + MemHit_getPrimIndexDelta(hit);
}

uint intel_get_hit_procedural_primitive_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*         hit  = get_query_hit(rayquery, hit_type);
    ProceduralLeaf* leaf = (ProceduralLeaf*)MemHit_getPrimLeafPtr(hit);
    return leaf->_primIndex[MemHit_getPrimLeafIndex(hit)];
}

uint intel_get_hit_instance_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*       hit  = get_query_hit(rayquery, hit_type);
    InstanceLeaf* leaf = (InstanceLeaf*)MemHit_getInstanceLeafPtr(hit);
    if (leaf == NULL) return -1;
    return leaf->part1.instanceIndex;
}

uint intel_get_hit_instance_user_id(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*       hit  = get_query_hit(rayquery, hit_type);
    InstanceLeaf* leaf = (InstanceLeaf*)MemHit_getInstanceLeafPtr(hit);
    if (leaf == NULL) return -1;
    return leaf->part1.instanceID;
}

intel_float4x3 intel_get_hit_world_to_object(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*       hit  = get_query_hit(rayquery, hit_type);
    InstanceLeaf* leaf = (InstanceLeaf*)MemHit_getInstanceLeafPtr(hit);

    if (leaf == NULL) return (intel_float4x3){{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};

    return (intel_float4x3) {
        {leaf->part0.world2obj_vx[0],
         leaf->part0.world2obj_vx[1],
         leaf->part0.world2obj_vx[2]},
        {leaf->part0.world2obj_vy[0],
         leaf->part0.world2obj_vy[1],
         leaf->part0.world2obj_vy[2]},
        {leaf->part0.world2obj_vz[0],
         leaf->part0.world2obj_vz[1],
         leaf->part0.world2obj_vz[2]},
        {leaf->part1.world2obj_p[0],
         leaf->part1.world2obj_p[1],
         leaf->part1.world2obj_p[2]}};
}

intel_float4x3 intel_get_hit_object_to_world(
    intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    MemHit*       hit  = get_query_hit(rayquery, hit_type);
    InstanceLeaf* leaf = (InstanceLeaf*)MemHit_getInstanceLeafPtr(hit);
    if (leaf == NULL) return (intel_float4x3){{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
    return (intel_float4x3) {
        {leaf->part1.obj2world_vx[0],
         leaf->part1.obj2world_vx[1],
         leaf->part1.obj2world_vx[2]},
        {leaf->part1.obj2world_vy[0],
         leaf->part1.obj2world_vy[1],
         leaf->part1.obj2world_vy[2]},
        {leaf->part1.obj2world_vz[0],
         leaf->part1.obj2world_vz[1],
         leaf->part1.obj2world_vz[2]},
        {leaf->part0.obj2world_p[0],
         leaf->part0.obj2world_p[1],
         leaf->part0.obj2world_p[2]}};
}

intel_candidate_type_t
intel_get_hit_candidate(intel_ray_query_t rayquery, intel_hit_type_t hit_type)
{
    if (BIF_FLAG_CTRL_GET(IsRayQueryReturnOptimizationEnabled))
    {
        rtfence_t fence = __builtin_IB_intel_query_rt_fence(rayquery);
        uintptr_t fenceInt = (uintptr_t)fence;

        intel_candidate_type_t committedStatus = (fenceInt >> 1) & 0x3;
        intel_candidate_type_t candidateType = (fenceInt>> 3) & 0x1;

        if (hit_type == intel_hit_type_committed_hit)
        {
            return committedStatus - 1;
        }
        else if (hit_type == intel_hit_type_potential_hit)
        {
            return candidateType;
        }
    }
    return MemHit_getLeafType(get_query_hit(rayquery, hit_type)) == NODE_TYPE_QUAD
               ? intel_candidate_type_triangle
               : intel_candidate_type_procedural;
}

// fetch triangle vertices for a hit
void intel_get_hit_triangle_vertices(
    intel_ray_query_t rayquery, intel_float3 vertices_out[3], intel_hit_type_t hit_type)
{
    MemHit*         hit  = get_query_hit(rayquery, hit_type);
    const QuadLeaf* leaf = (QuadLeaf*)MemHit_getPrimLeafPtr(hit);

    unsigned int j0 = 0, j1 = 1, j2 = 2;
    if (MemHit_getPrimLeafIndex(hit) != 0)
    {
        j0 = QuadLeaf_getJ0(leaf);
        j1 = QuadLeaf_getJ1(leaf);
        j2 = QuadLeaf_getJ2(leaf);
    }

    vertices_out[0] = (intel_float3){leaf->v[j0][0], leaf->v[j0][1], leaf->v[j0][2]};
    vertices_out[1] = (intel_float3){leaf->v[j1][0], leaf->v[j1][1], leaf->v[j1][2]};
    vertices_out[2] = (intel_float3){leaf->v[j2][0], leaf->v[j2][1], leaf->v[j2][2]};
}

// Read ray-data. This is used to read transformed rays produced by HW instancing pipeline
// during any-hit or intersection shader execution.
intel_float3 intel_get_ray_origin(intel_ray_query_t rayquery, uint bvh_level)
{
    global void*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);

    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    return (intel_float3){memRay->org[0], memRay->org[1], memRay->org[2]};
}

intel_float3 intel_get_ray_direction(intel_ray_query_t rayquery, uint bvh_level)
{
    global RTStack*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);

    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    return (intel_float3){memRay->dir[0], memRay->dir[1], memRay->dir[2]};
}

float intel_get_ray_tmin(intel_ray_query_t rayquery, uint bvh_level)
{
    global void*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);

    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    return memRay->tnear;
}

intel_ray_flags_t intel_get_ray_flags(intel_ray_query_t rayquery, uint bvh_level)
{
    global void*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);

    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    return (intel_ray_flags_t)MemRay_getRayFlags(memRay);
}

int intel_get_ray_mask(intel_ray_query_t rayquery, uint bvh_level)
{
    global void*         rtStack  = __builtin_IB_intel_query_rt_stack(rayquery);

    MemRay* memRay = get_rt_stack_ray(rtStack, bvh_level);
    return MemRay_getRayMask(memRay);
}

// Test whether traversal has terminated.  If false, the ray has reached
// a procedural leaf or a non-opaque triangle leaf, and requires shader processing.
bool intel_is_traversal_done(intel_ray_query_t rayquery)
{
    if (BIF_FLAG_CTRL_GET(IsRayQueryReturnOptimizationEnabled))
    {
        rtfence_t fence = __builtin_IB_intel_query_rt_fence(rayquery);
        uintptr_t fenceInt = (uintptr_t)fence;

        bool proceedFurther = (fenceInt & 0x1) != 0;


        return !proceedFurther;
    }
    bool isTraversalDone = MemHit_getDone(get_query_hit(rayquery, intel_hit_type_potential_hit));
    return isTraversalDone;
}

// if traversal is done one can test for the presence of a committed hit to either invoke miss or closest hit shader
bool intel_has_committed_hit(intel_ray_query_t rayquery)
{
    return MemHit_getValid(get_query_hit(rayquery, intel_hit_type_committed_hit));
}

#endif // defined(cl_intel_rt_production)

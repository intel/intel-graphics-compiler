/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Ray tracing built-in declarations.

// ===== cl_intel_rt_production =====
#ifdef cl_intel_rt_production
// ----------- Raytracing production API code -----------

// --- Opaque types ---
typedef private struct intel_ray_query_opaque_t *intel_ray_query_t;
typedef global struct intel_raytracing_acceleration_structure_opaque_t
    *intel_raytracing_acceleration_structure_t;

// --- Enum and struct definitions ---

typedef enum // intel_ray_flags_t
{
    intel_ray_flags_none = 0x00,
    intel_ray_flags_force_opaque =
        0x01, // forces geometry to be opaque (no anyhit shader invokation)
    intel_ray_flags_force_non_opaque =
        0x02, // forces geometry to be non-opqaue (invoke anyhit shader)
    intel_ray_flags_accept_first_hit_and_end_search =
        0x04, // terminates traversal on the first hit found (shadow rays)
    intel_ray_flags_skip_closest_hit_shader =
        0x08, // skip execution of the closest hit shader
    intel_ray_flags_cull_back_facing_triangles =
        0x10, // back facing triangles to not produce a hit
    intel_ray_flags_cull_front_facing_triangles =
        0x20,                               // front facing triangles do not produce a hit
    intel_ray_flags_cull_opaque     = 0x40, // opaque geometry does not produce a hit
    intel_ray_flags_cull_non_opaque = 0x80, // non-opaque geometry does not produce a hit
    intel_ray_flags_skip_triangles = 0x100, // treat all triangle intersections as misses.
    intel_ray_flags_skip_procedural_primitives =
        0x200, // skip execution of intersection shaders
} intel_ray_flags_t;

typedef enum intel_hit_type_t
{
    intel_hit_type_committed_hit = 0,
    intel_hit_type_potential_hit = 1,
} intel_hit_type_t;

typedef struct // intel_float2
{
    float x, y;
} intel_float2;

typedef struct // intel_float3
{
    float x, y, z;
} intel_float3;

typedef struct // intel_float4x3
{
    intel_float3 vx, vy, vz, p;
} intel_float4x3;

typedef struct // intel_ray_desc_t
{
    intel_float3      origin;
    intel_float3      direction;
    float             tmin;
    float             tmax;
    uint              mask;
    intel_ray_flags_t flags;
} intel_ray_desc_t;

// if traversal returns one can test if a triangle or procedural is hit
typedef enum // intel_candidate_type_t
{
    intel_candidate_type_triangle,
    intel_candidate_type_procedural
} intel_candidate_type_t;

// --- API functions ---

// initialize a ray query
intel_ray_query_t intel_ray_query_init(
    intel_ray_desc_t ray, intel_raytracing_acceleration_structure_t accel);

// setup for instance traversal using a transformed ray and bottom-level AS
void intel_ray_query_forward_ray(
    intel_ray_query_t                         query,
    intel_ray_desc_t                          ray,
    intel_raytracing_acceleration_structure_t accel);

// commit the potential hit
void intel_ray_query_commit_potential_hit(intel_ray_query_t query);

// commit the potential hit and override hit distance and UVs
void intel_ray_query_commit_potential_hit_override(
    intel_ray_query_t query, float override_hit_distance, intel_float2 override_uv);

// start traversal of a ray query
void intel_ray_query_start_traversal(intel_ray_query_t query);

// Synchronize ray_query execution. If a ray was traversed,
// this must be called prior to accessing the ray query.
void intel_ray_query_sync(intel_ray_query_t query);

// Signal that a ray query will not be used further. This is the moral
// equivalent of a delete. This function does an implicit sync.
void intel_ray_query_abandon(intel_ray_query_t query);

// read hit information during shader execution
uint  intel_get_hit_bvh_level(intel_ray_query_t query, intel_hit_type_t hit_type);
float intel_get_hit_distance(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float2
     intel_get_hit_barycentrics(intel_ray_query_t query, intel_hit_type_t hit_type);
bool intel_get_hit_front_face(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_geometry_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_primitive_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_triangle_primitive_id(
    intel_ray_query_t query,
    intel_hit_type_t  hit_type); // fast path for triangles
uint intel_get_hit_procedural_primitive_id(
    intel_ray_query_t query,
    intel_hit_type_t  hit_type); // fast path for procedurals
uint intel_get_hit_instance_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_instance_user_id(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float4x3
intel_get_hit_world_to_object(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float4x3
intel_get_hit_object_to_world(intel_ray_query_t query, intel_hit_type_t hit_type);

intel_candidate_type_t
intel_get_hit_candidate(intel_ray_query_t query, intel_hit_type_t hit_type);

// fetch triangle vertices for a hit
void intel_get_hit_triangle_vertices(
    intel_ray_query_t query, intel_float3 vertices_out[3], intel_hit_type_t hit_type);

// Read ray-data. This is used to read transformed rays produced by HW
// instancing pipeline during any-hit or intersection shader execution.
intel_float3      intel_get_ray_origin(intel_ray_query_t query, uint bvh_level);
intel_float3      intel_get_ray_direction(intel_ray_query_t query, uint bvh_level);
float             intel_get_ray_tmin(intel_ray_query_t query, uint bvh_level);
intel_ray_flags_t intel_get_ray_flags(intel_ray_query_t query, uint bvh_level);
int               intel_get_ray_mask(intel_ray_query_t query, uint bvh_level);

// Test whether traversal has terminated.  If false, the ray has reached
// a procedural leaf or a non-opaque triangle leaf, and requires shader
// processing.
bool intel_is_traversal_done(intel_ray_query_t query);

// if traversal is done one can test for the presence of a committed hit to
// either invoke miss or closest hit shader
bool intel_has_committed_hit(intel_ray_query_t query);
#endif // cl_intel_rt_production

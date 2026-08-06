/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

half __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf(float a, int seed)
{
    uint new_seed = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    return __builtin_IB_srnd_ftohf_1(a, as_short2(new_seed).x);
}

half2 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf2(float2 a, int seed)
{
    uint new_seed = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    return __builtin_IB_srnd_ftohf_2(a, as_short2(new_seed));
}

half3 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf3(float3 a, int seed)
{
    uint2 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x2(seed);
    return __builtin_IB_srnd_ftohf_3(a, as_short4(new_seed).xyz);
}

half4 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf4(float4 a, int seed)
{
    uint2 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x2(seed);
    return __builtin_IB_srnd_ftohf_4(a, as_short4(new_seed));
}

half8 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf8(float8 a, int seed)
{
    uint4 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x4(seed);
    return __builtin_IB_srnd_ftohf_8(a, as_short8(new_seed));
}

half16 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf16(float16 a, int seed)
{
    uint8 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x8(seed);
    return __builtin_IB_srnd_ftohf_16(a, as_short16(new_seed));
}

half __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf(float a, int seed, private int *output_seed)
{
    uint new_seed = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    *output_seed  = new_seed;
    return __builtin_IB_srnd_ftohf_1(a, as_short2(new_seed).x);
}

half2 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf2(float2 a, int seed, private int *output_seed)
{
    uint new_seed = __builtin_IB_lfsr_b16v2(as_uint(seed), LfsrPolynomial_b16v2);
    *output_seed  = new_seed;
    return __builtin_IB_srnd_ftohf_2(a, as_short2(new_seed));
}

half3 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf3(float3 a, int seed, private int *output_seed)
{
    uint2 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x2(seed);
    *output_seed   = new_seed.y;
    return __builtin_IB_srnd_ftohf_3(a, as_short4(new_seed).xyz);
}

half4 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf4(float4 a, int seed, private int *output_seed)
{
    uint2 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x2(seed);
    *output_seed   = new_seed.y;
    return __builtin_IB_srnd_ftohf_4(a, as_short4(new_seed));
}

half8 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf8(float8 a, int seed, private int *output_seed)
{
    uint4 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x4(seed);
    *output_seed   = new_seed.w;
    return __builtin_IB_srnd_ftohf_8(a, as_short8(new_seed));
}

half16 __attribute__((overloadable))
__spirv_StochasticRoundFToFINTEL_Rhalf16(float16 a, int seed, private int *output_seed)
{
    uint8 new_seed = __builtin_IB_lfsr_helper_b16v2_to_b16v2x8(seed);
    *output_seed   = new_seed.s7;
    return __builtin_IB_srnd_ftohf_16(a, as_short16(new_seed));
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ConvertE5M2ToFP16INTEL
half __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char a) { return __builtin_IB_bf8tohf_1(a); }
half2 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char2 a) { return __builtin_IB_bf8tohf_2(a); }
half3 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char3 a) { return __builtin_IB_bf8tohf_3(a); }
half4 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char4 a) { return __builtin_IB_bf8tohf_4(a); }
half8 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char8 a) { return __builtin_IB_bf8tohf_8(a); }
half16 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16INTEL(char16 a) { return __builtin_IB_bf8tohf_16(a); }

// ConvertE4M3ToFP16
half __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char a) { return __builtin_IB_hf8tohf_1(a); }
half2 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char2 a) { return __builtin_IB_hf8tohf_2(a); }
half3 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char3 a) { return __builtin_IB_hf8tohf_3(a); }
half4 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char4 a) { return __builtin_IB_hf8tohf_4(a); }
half8 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char8 a) { return __builtin_IB_hf8tohf_8(a); }
half16 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16INTEL(char16 a) { return __builtin_IB_hf8tohf_16(a); }

// ConvertFP16ToE5M2INTEL
char __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half a) { return __builtin_IB_hftobf8_1(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half2 a) { return __builtin_IB_hftobf8_2(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half3 a) { return __builtin_IB_hftobf8_3(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half4 a) { return __builtin_IB_hftobf8_4(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half8 a) { return __builtin_IB_hftobf8_8(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2INTEL(half16 a) { return __builtin_IB_hftobf8_16(a); }

// ConvertFP16ToE4M3INTEL
char __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half a) { return __builtin_IB_hftohf8_1(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half2 a) { return __builtin_IB_hftohf8_2(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half3 a) { return __builtin_IB_hftohf8_3(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half4 a) { return __builtin_IB_hftohf8_4(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half8 a) { return __builtin_IB_hftohf8_8(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3INTEL(half16 a) { return __builtin_IB_hftohf8_16(a); }

// ClampConvertFP16ToE5M2INTEL
char __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half a) { return __builtin_IB_hftobf8_1_sat(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half2 a) { return __builtin_IB_hftobf8_2_sat(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half3 a) { return __builtin_IB_hftobf8_3_sat(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half4 a) { return __builtin_IB_hftobf8_4_sat(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half8 a) { return __builtin_IB_hftobf8_8_sat(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE5M2INTEL(half16 a) { return __builtin_IB_hftobf8_16_sat(a); }

// ClampConvertFP16ToE4M3INTEL
char __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half a) { return __builtin_IB_hftohf8_1_sat(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half2 a) { return __builtin_IB_hftohf8_2_sat(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half3 a) { return __builtin_IB_hftohf8_3_sat(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half4 a) { return __builtin_IB_hftohf8_4_sat(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half8 a) { return __builtin_IB_hftohf8_8_sat(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ClampConvertFP16ToE4M3INTEL(half16 a) { return __builtin_IB_hftohf8_16_sat(a); }

// ConvertE4M3ToBF16INTEL (emulation)
short __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}
short2 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char2 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat2(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}
short3 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char3 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat3(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}
short4 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char4 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat4(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}
short8 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char8 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat8(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}
short16 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16INTEL(char16 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat16(__builtin_spirv_ConvertE4M3ToFP16INTEL(a)));
}

// ConvertE5M2ToBF16INTEL (emulation)
short __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}
short2 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char2 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat2(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}
short3 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char3 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat3(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}
short4 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char4 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat4(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}
short8 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char8 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat8(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}
short16 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16INTEL(char16 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat16(__builtin_spirv_ConvertE5M2ToFP16INTEL(a)));
}

// ConvertBF16ToE5M2INTEL (emulation)
char __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short2 a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short3 a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short4 a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short8 a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2INTEL(short16 a) {
    return __builtin_spirv_ConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
}

// ConvertBF16ToE4M3INTEL (emulation)
char __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short2 a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short3 a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short4 a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short8 a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3INTEL(short16 a) {
    return __builtin_spirv_ConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
}

// ClampConvertBF16ToE5M2INTEL (emulation)
char __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short a) {
    return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short2 a) {
    return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short3 a) {
    return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short4 a) {
    return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short8 a) {
   return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE5M2INTEL(short16 a) {
    return __builtin_spirv_ClampConvertFP16ToE5M2INTEL(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
}

// ClampConvertBF16ToE4M3INTEL (emulation)
char __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short2 a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short3 a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short4 a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short8 a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ClampConvertBF16ToE4M3INTEL(short16 a) {
    return __builtin_spirv_ClampConvertFP16ToE4M3INTEL(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
}

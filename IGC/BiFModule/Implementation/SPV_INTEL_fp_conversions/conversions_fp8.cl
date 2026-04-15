/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ConvertE5M2ToFP16EXT
half __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char a) { return __builtin_IB_bf8tohf_1(a); }
half2 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char2 a) { return __builtin_IB_bf8tohf_2(a); }
half3 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char3 a) { return __builtin_IB_bf8tohf_3(a); }
half4 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char4 a) { return __builtin_IB_bf8tohf_4(a); }
half8 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char8 a) { return __builtin_IB_bf8tohf_8(a); }
half16 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToFP16EXT(char16 a) { return __builtin_IB_bf8tohf_16(a); }

// ConvertE4M3ToFP16EXT
half __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char a) { return __builtin_IB_hf8tohf_1(a); }
half2 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char2 a) { return __builtin_IB_hf8tohf_2(a); }
half3 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char3 a) { return __builtin_IB_hf8tohf_3(a); }
half4 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char4 a) { return __builtin_IB_hf8tohf_4(a); }
half8 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char8 a) { return __builtin_IB_hf8tohf_8(a); }
half16 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToFP16EXT(char16 a) { return __builtin_IB_hf8tohf_16(a); }

// ConvertFP16ToE5M2EXT
char __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half a) { return __builtin_IB_hftobf8_1(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half2 a) { return __builtin_IB_hftobf8_2(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half3 a) { return __builtin_IB_hftobf8_3(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half4 a) { return __builtin_IB_hftobf8_4(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half8 a) { return __builtin_IB_hftobf8_8(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE5M2EXT(half16 a) { return __builtin_IB_hftobf8_16(a); }

// ConvertFP16ToE4M3EXT
char __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half a) { return __builtin_IB_hftohf8_1(a); }
char2 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half2 a) { return __builtin_IB_hftohf8_2(a); }
char3 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half3 a) { return __builtin_IB_hftohf8_3(a); }
char4 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half4 a) { return __builtin_IB_hftohf8_4(a); }
char8 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half8 a) { return __builtin_IB_hftohf8_8(a); }
char16 __attribute__((overloadable)) __builtin_spirv_ConvertFP16ToE4M3EXT(half16 a) { return __builtin_IB_hftohf8_16(a); }

// ClampConvertFP16ToE5M2INTEL (emulation)
// If _sat converts ±inf to E5M2 inf encoding (0x7C/0xFC),
// subtract 1 to get ±max normal (0x7B/0xFB).
// Check the input for inf to preserve NaN pass-through.
char __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half a) {
  char result = __builtin_IB_hftobf8_1_sat(a);
  char isInf = convert_char(__spirv_IsInf(a));
  return select(result, (char)(result - 1), isInf);
}

char2 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half2 a) {
  char2 result = __builtin_IB_hftobf8_2_sat(a);
  char2 isInf = convert_char2(__spirv_IsInf(a));
  return select(result, result - (char2)(1), isInf);
}

char3 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half3 a) {
  char3 result = __builtin_IB_hftobf8_3_sat(a);
  char3 isInf = convert_char3(__spirv_IsInf(a));
  return select(result, result - (char3)(1), isInf);
}

char4 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half4 a) {
  char4 result = __builtin_IB_hftobf8_4_sat(a);
  char4 isInf = convert_char4(__spirv_IsInf(a));
  return select(result, result - (char4)(1), isInf);
}

char8 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half8 a) {
  char8 result = __builtin_IB_hftobf8_8_sat(a);
  char8 isInf = convert_char8(__spirv_IsInf(a));
  return select(result, result - (char8)(1), isInf);
}

char16 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE5M2INTEL(half16 a) {
  char16 result = __builtin_IB_hftobf8_16_sat(a);
  char16 isInf = convert_char16(__spirv_IsInf(a));
  return select(result, result - (char16)(1), isInf);
}


// ClampConvertFP16ToE4M3INTEL (emulation)
// If _sat converts ±inf to E4M3 NaN encoding (0x7F/0xFF),
// subtract 1 to get ±max normal (0x7E/0xFE).
// Check the input for inf (not the output) to preserve NaN pass-through.
char __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half a) {
  char result = __builtin_IB_hftohf8_1_sat(a);
  char isInf = convert_char(__spirv_IsInf(a));
  return select(result, (char)(result - 1), isInf);
}

char2 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half2 a) {
  char2 result = __builtin_IB_hftohf8_2_sat(a);
  char2 isInf = convert_char2(__spirv_IsInf(a));
  return select(result, result - (char2)(1), isInf);
}

char3 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half3 a) {
  char3 result = __builtin_IB_hftohf8_3_sat(a);
  char3 isInf = convert_char3(__spirv_IsInf(a));
  return select(result, result - (char3)(1), isInf);
}

char4 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half4 a) {
  char4 result = __builtin_IB_hftohf8_4_sat(a);
  char4 isInf = convert_char4(__spirv_IsInf(a));
  return select(result, result - (char4)(1), isInf);
}

char8 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half8 a) {
  char8 result = __builtin_IB_hftohf8_8_sat(a);
  char8 isInf = convert_char8(__spirv_IsInf(a));
  return select(result, result - (char8)(1), isInf);
}

char16 __attribute__((overloadable))
__builtin_spirv_ClampConvertFP16ToE4M3INTEL(half16 a) {
  char16 result = __builtin_IB_hftohf8_16_sat(a);
  char16 isInf = convert_char16(__spirv_IsInf(a));
  return select(result, result - (char16)(1), isInf);
}

// ConvertE4M3ToBF16EXT (emulation)
short __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}
short2 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char2 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat2(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}
short3 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char3 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat3(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}
short4 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char4 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat4(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}
short8 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char8 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat8(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}
short16 __attribute__((overloadable)) __builtin_spirv_ConvertE4M3ToBF16EXT(char16 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat16(__builtin_spirv_ConvertE4M3ToFP16EXT(a)));
}

// ConvertE5M2ToBF16EXT (emulation)
short __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}
short2 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char2 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat2(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}
short3 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char3 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat3(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}
short4 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char4 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat4(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}
short8 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char8 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat8(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}
short16 __attribute__((overloadable)) __builtin_spirv_ConvertE5M2ToBF16EXT(char16 a) {
    return __spirv_ConvertFToBF16INTEL(__spirv_FConvert_Rfloat16(__builtin_spirv_ConvertE5M2ToFP16EXT(a)));
}

// ConvertBF16ToE5M2EXT (emulation)
char __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short2 a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short3 a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short4 a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short8 a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE5M2EXT(short16 a) {
    return __builtin_spirv_ConvertFP16ToE5M2EXT(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
}

// ConvertBF16ToE4M3EXT (emulation)
char __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf(__spirv_ConvertBF16ToFINTEL(a)));
}
char2 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short2 a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf2(__spirv_ConvertBF16ToFINTEL(a)));
}
char3 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short3 a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf3(__spirv_ConvertBF16ToFINTEL(a)));
}
char4 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short4 a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf4(__spirv_ConvertBF16ToFINTEL(a)));
}
char8 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short8 a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf8(__spirv_ConvertBF16ToFINTEL(a)));
}
char16 __attribute__((overloadable)) __builtin_spirv_ConvertBF16ToE4M3EXT(short16 a) {
    return __builtin_spirv_ConvertFP16ToE4M3EXT(__spirv_FConvert_Rhalf16(__spirv_ConvertBF16ToFINTEL(a)));
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

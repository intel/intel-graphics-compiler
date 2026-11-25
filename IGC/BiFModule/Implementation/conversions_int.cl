/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.h"
#include "../Headers/spirv.h"

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar(uchar UnsignedValue) { return UnsignedValue; }

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uchar UnsignedValue) { return (ushort)UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint(uchar UnsignedValue) { return (uint)UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong(uchar UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar(ushort UnsignedValue) { return (uchar)UnsignedValue; }

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ushort UnsignedValue) { return UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint(ushort UnsignedValue) { return (uint)UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong(ushort UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar(uint UnsignedValue) { return (uchar)UnsignedValue; }

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uint UnsignedValue) { return (ushort)UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint(uint UnsignedValue) { return UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong(uint UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar(ulong UnsignedValue) { return (uchar)UnsignedValue; }

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ulong UnsignedValue) { return (ushort)UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint(ulong UnsignedValue) { return (uint)UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong(ulong UnsignedValue) { return UnsignedValue; }

char __attribute__((overloadable)) __spirv_SConvert_Rchar(char SignedValue) { return SignedValue; }

short __attribute__((overloadable)) __spirv_SConvert_Rshort(char SignedValue) { return (short)SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint(char SignedValue) { return (int)SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong(char SignedValue) { return (long)SignedValue; }

char __attribute__((overloadable)) __spirv_SConvert_Rchar(short SignedValue) { return (char)SignedValue; }

short __attribute__((overloadable)) __spirv_SConvert_Rshort(short SignedValue) { return SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint(short SignedValue) { return (int)SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong(short SignedValue) { return (long)SignedValue; }

char __attribute__((overloadable)) __spirv_SConvert_Rchar(int SignedValue) { return (char)SignedValue; }

short __attribute__((overloadable)) __spirv_SConvert_Rshort(int SignedValue) { return (short)SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint(int SignedValue) { return SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong(int SignedValue) { return (long)SignedValue; }

char __attribute__((overloadable)) __spirv_SConvert_Rchar(long SignedValue) { return (char)SignedValue; }

short __attribute__((overloadable)) __spirv_SConvert_Rshort(long SignedValue) { return (short)SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint(long SignedValue) { return (int)SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong(long SignedValue) { return SignedValue; }

uchar __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(char SignedValue) {
  // return __builtin_IB_ctouc_sat((char)SignedValue);
  if (SignedValue <= 0) {
    return (uchar)0;
  }
  return (uchar)SignedValue;
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(char SignedValue) {
  // return __builtin_IB_ctous_sat((char)SignedValue);
  if (SignedValue <= 0) {
    return 0;
  }
  return (ushort)SignedValue;
}

uint __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(char SignedValue) {
  // return __builtin_IB_ctoui_sat((char)SignedValue);
  return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(char SignedValue) {
  return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(short SignedValue) {
  return (uchar)clamp(SignedValue, (short)0, (short)UCHAR_MAX);
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(short SignedValue) {
  // return __builtin_IB_stous_sat((short)SignedValue);
  if (SignedValue <= 0) {
    return 0;
  }
  return (ushort)SignedValue;
}

uint __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(short SignedValue) {
  // return __builtin_IB_stoui_sat((short)SignedValue);
  return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(short SignedValue) {
  return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(int SignedValue) {
  return (uchar)clamp(SignedValue, 0, (int)UCHAR_MAX);
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(int SignedValue) {
  // return __builtin_IB_itous_sat((int)SignedValue);
  int res = __spirv_ocl_s_clamp(SignedValue, 0, (int)USHRT_MAX);
  return (ushort)res;
}

uint __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(int SignedValue) {
  // return __builtin_IB_itoui_sat((int)SignedValue);
  return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(int SignedValue) {
  return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(long SignedValue) {
  if (SignedValue <= 0) {
    return 0;
  } else if (SignedValue >= UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return (uchar)SignedValue;
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(long SignedValue) {
  if (SignedValue <= 0) {
    return 0;
  } else if (SignedValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)SignedValue;
}

uint __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(long SignedValue) {
  if (SignedValue <= 0) {
    return 0;
  } else if (SignedValue >= UINT_MAX) {
    return UINT_MAX;
  }
  return (uint)SignedValue;
}

ulong __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(long SignedValue) {
  return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

char __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(uchar UnsignedValue) {
  // return __builtin_IB_uctoc_sat((uchar)UnsignedValue);
  return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(uchar UnsignedValue) { return (short)UnsignedValue; }

int __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(uchar UnsignedValue) { return (int)UnsignedValue; }

long __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(uchar UnsignedValue) { return (long)UnsignedValue; }

char __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(ushort UnsignedValue) {
  // return __builtin_IB_ustoc_sat((ushort)UnsignedValue);
  return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(ushort UnsignedValue) {
  // return __builtin_IB_ustos_sat((ushort)UnsignedValue);
  return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (ushort)UnsignedValue;
}

int __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(ushort UnsignedValue) { return (int)UnsignedValue; }

long __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(ushort UnsignedValue) { return (long)UnsignedValue; }

char __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(uint UnsignedValue) {
  // return __builtin_IB_uitoc_sat((uint)UnsignedValue);
  return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(uint UnsignedValue) {
  // return __builtin_IB_uitos_sat((uint)UnsignedValue);
  return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(uint UnsignedValue) {
  // return __builtin_IB_uitoi_sat((uint)UnsignedValue);
  return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(uint UnsignedValue) { return (long)UnsignedValue; }

char __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(ulong UnsignedValue) {
  return (UnsignedValue >= CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(ulong UnsignedValue) {
  return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(ulong UnsignedValue) {
  return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(ulong UnsignedValue) {
  return (UnsignedValue >= LONG_MAX) ? LONG_MAX : __spirv_SConvert_Rlong((long)UnsignedValue);
}

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(uchar UnsignedValue) { return UnsignedValue; }

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uchar UnsignedValue) { return UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(uchar UnsignedValue) { return (uint)UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(uchar UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(ushort UnsignedValue) {
  // return __builtin_IB_ustouc_sat((ushort)UnsignedValue);
  if (UnsignedValue > (uchar)0xff) {
    return (uchar)0xff;
  }
  return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ushort UnsignedValue) { return UnsignedValue; }

uint __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(ushort UnsignedValue) { return (uint)UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(ushort UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(uint UnsignedValue) {
  // return __builtin_IB_uitouc_sat((uint)UnsignedValue);
  if (UnsignedValue > UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uint UnsignedValue) {
  if (UnsignedValue > (ushort)0xffff) {
    return (ushort)0xffff;
  }
  return (ushort)UnsignedValue;
}

uint __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(uint UnsignedValue) { return UnsignedValue; }

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(uint UnsignedValue) { return (ulong)UnsignedValue; }

uchar __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(ulong UnsignedValue) {
  return (UnsignedValue >= UCHAR_MAX) ? UCHAR_MAX : (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ulong UnsignedValue) {
  return (UnsignedValue >= USHRT_MAX) ? USHRT_MAX : (ushort)UnsignedValue;
}

uint __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(ulong UnsignedValue) {
  return (UnsignedValue >= UINT_MAX) ? UINT_MAX : (uint)UnsignedValue;
}

ulong __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(ulong UnsignedValue) { return UnsignedValue; }

char __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(char SignedValue) { return SignedValue; }

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(char SignedValue) { return (short)SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint_sat(char SignedValue) { return (int)SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(char SignedValue) {
  return __spirv_SConvert_Rlong(SignedValue);
}

char __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(short SignedValue) {
  // return __builtin_IB_stoc_sat((short)SignedValue);
  short res = __spirv_ocl_s_clamp(SignedValue, (short)CHAR_MIN, (short)CHAR_MAX);
  return (char)res;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(short SignedValue) { return SignedValue; }

int __attribute__((overloadable)) __spirv_SConvert_Rint_sat(short SignedValue) { return (int)SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(short SignedValue) {
  return __spirv_SConvert_Rlong(SignedValue);
}

char __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(int SignedValue) {
  // return __builtin_IB_itoc_sat((int)SignedValue);
  int res = __spirv_ocl_s_clamp(SignedValue, (int)CHAR_MIN, (int)CHAR_MAX);
  return (char)res;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(int SignedValue) {
  // return __builtin_IB_itos_sat((int)SignedValue);
  int res = __spirv_ocl_s_clamp(SignedValue, (int)SHRT_MIN, (int)SHRT_MAX);
  return (short)res;
}

int __attribute__((overloadable)) __spirv_SConvert_Rint_sat(int SignedValue) { return SignedValue; }

long __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(int SignedValue) {
  return __spirv_SConvert_Rlong(SignedValue);
}

char __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(long SignedValue) {
  if (SignedValue <= CHAR_MIN) {
    return CHAR_MIN;
  } else if (SignedValue >= CHAR_MAX) {
    return CHAR_MAX;
  }
  return (char)SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(long SignedValue) {
  if (SignedValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (SignedValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)SignedValue;
}

int __attribute__((overloadable)) __spirv_SConvert_Rint_sat(long SignedValue) {
  if (SignedValue <= INT_MIN) {
    return INT_MIN;
  } else if (SignedValue >= INT_MAX) {
    return INT_MAX;
  }
  return (int)SignedValue;
}

long __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(long SignedValue) {
  return __spirv_SConvert_Rlong(SignedValue);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed(SConvert, char, i8) GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed(
    SConvert, short,
    i16) GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed(SConvert, int,
                                                         i32) GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed(SConvert,
                                                                                                              long, i64)

    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned(UConvert, uchar,
                                                      i8) GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned(UConvert,
                                                                                                            ushort, i16)
        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned(UConvert, uint, i32)
            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned(UConvert, ulong, i64)

                GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned(SatConvertSToU, uchar, i8)
                    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned(SatConvertSToU, ushort, i16)
                        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned(SatConvertSToU, uint, i32)
                            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned(SatConvertSToU, ulong,
                                                                                                  i64)

                                GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned(SatConvertUToS,
                                                                                                      char, i8)
                                    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned(
                                        SatConvertUToS, short, i16)
                                        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned(
                                            SatConvertUToS, int, i32)
                                            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned(
                                                SatConvertUToS, long, i64)

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

#include <cassert>
#include <cstdint>
#include <unordered_map>

namespace vISA {
class IR_Builder;
class Attributes {
public:
  /// <summary>
  /// Attribute kind, allow an attribute to be of multiple kinds.
  /// </summary>
  enum AttrKind { AK_KERNEL = 0x1, AK_VAR = 0x2, AK_MASK = 0x3 };

  /// <summary>
  ///  Attribute value Types
  /// </summary>
  enum class AttrType : uint16_t { Bool, Int32, Int64, CString };

  struct SAttrVal {
    AttrType m_attrType;
    union {
      uint64_t m_mem; // WA to designated initialization
      int64_t m_i64;
      int32_t m_i32;
      const char *m_cstr;
      bool m_bool;
    } u;
  };

  struct SAttrInfo {
    // Attribute kind : one bit to denote one kind.
    // An attr can be of multiple kinds, for example, both kernel and var.
    uint16_t m_attrKind;
    const char *m_attrName;
    SAttrVal m_defaultVal; // type and default value
    const char *m_description;
  };

  /// <summary>
  /// Attribute ID
  /// </summary>
  enum ID {
#define DEF_ATTR(E, N, K, I, D) E,
#include "VISAAttributes.h"

    ATTR_TOTAL_NUM,
    ATTR_INVALID
  };

  // Used for current Attribute Value
  struct SAttrValue {
    SAttrVal m_val;
    bool m_isSet;
  };

  /// Given an attribute name, return its ID
  static ID getAttributeID(const char *AttrName);

  static bool isValid(ID aID) { return (aID >= 0 && aID < ATTR_TOTAL_NUM); }

  /// Given an attribute ID, return its name
  static const char *getAttributeName(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_attrName;
  }

  /// Return true if the given AttrName's ID == aID
  static bool isAttribute(ID aID, const char *AttrName) {
    return aID == getAttributeID(AttrName);
  }

  static bool isBool(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_defaultVal.m_attrType == AttrType::Bool;
  }
  static bool isInt32(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_defaultVal.m_attrType == AttrType::Int32;
  }
  static bool isInt64(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_defaultVal.m_attrType == AttrType::Int64;
  }
  static bool isCStr(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_defaultVal.m_attrType == AttrType::CString;
  }
  static bool isInt(ID aID) { return isInt32(aID) || isInt64(aID); }
  static bool isKernelAttr(ID aID) {
    vASSERT(isValid(aID));
    return (AttrsInfo[(int)aID].m_attrKind & AK_KERNEL) != 0;
  }
  static bool isVarAttr(ID aID) {
    vASSERT(isValid(aID));
    return (AttrsInfo[(int)aID].m_attrKind & AK_VAR) != 0;
  }
  static bool isIntKernelAttr(ID aID) {
    return isKernelAttr(aID) && isInt(aID);
  }
  static bool isStringKernelAttr(ID aID) {
    return isKernelAttr(aID) && isCStr(aID);
  }
  static bool isIntVarAttr(ID aID) { return isVarAttr(aID) && isInt(aID); }
  static bool isStringVarAttr(ID aID) { return isVarAttr(aID) && isCStr(aID); }

  // default attribute value
  static int32_t getInt32AttrDefault(ID aID) {
    vASSERT(isValid(aID));
    return AttrsInfo[(int)aID].m_defaultVal.u.m_i32;
  }

  Attributes();

  // Set attribute's value
  void setKernelAttr(ID kID, bool v);
  void setKernelAttr(ID kID, int32_t v, const IR_Builder &irb);
  void setKernelAttr(ID kID, int64_t v);
  void setKernelAttr(ID kID, const char *v);

  // Get Attribute's value
  bool getBoolKernelAttr(ID kID) const {
    SAttrValue *pAV = getKernelAttrValue(kID);
    vASSERT(pAV->m_val.m_attrType == AttrType::Bool);
    return pAV->m_val.u.m_bool;
  }
  int32_t getInt32KernelAttr(ID kID) const {
    SAttrValue *pAV = getKernelAttrValue(kID);
    vASSERT(pAV->m_val.m_attrType == AttrType::Int32);
    return pAV->m_val.u.m_i32;
  }
  int64_t getInt64KernelAttr(ID kID) const {
    SAttrValue *pAV = getKernelAttrValue(kID);
    vASSERT(pAV->m_val.m_attrType == AttrType::Int64);
    return pAV->m_val.u.m_i64;
  }
  const char *getCStrKernelAttr(ID kID) const {
    SAttrValue *pAV = getKernelAttrValue(kID);
    vASSERT(pAV->m_val.m_attrType == AttrType::CString);
    return pAV->m_val.u.m_cstr;
  }
  bool isKernelAttrSet(ID kID) const {
    SAttrValue *pAV = getKernelAttrValue(kID);
    return pAV->m_isSet;
  }

private:
  /// <summary>
  /// Attribute meta info
  /// </summary>
  static SAttrInfo AttrsInfo[ATTR_TOTAL_NUM];

  SAttrValue *getKernelAttrValue(ID kID) const {
    auto II = m_kernelAttrs.find(kID);
    vASSERT(II != m_kernelAttrs.end());
    SAttrValue *pAV = II->second;
    return pAV;
  }

  // Storage for holding per-kernel attr.
  SAttrValue m_attrValueStorage[ATTR_TOTAL_NUM];
  std::unordered_map<int, SAttrValue *> m_kernelAttrs;
};
};     // namespace vISA
#endif // _ATTRIBUTES_H_

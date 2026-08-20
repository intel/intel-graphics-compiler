/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "iStdLib/types.h"
#include "common/shaderHash.hpp"
#include "common/SysUtils.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <variant>
#include "CommonMacros.h"

typedef char debugString[1024];

namespace IGC {
inline constexpr size_t MaxUmdRegkeyOptionsSize = 4096;
inline constexpr size_t MaxUmdRegkeyOptionCount = 64;

using UmdRegkeyValue = std::variant<uint32_t, std::string>;
using UmdRegkeyMap = std::map<std::string, UmdRegkeyValue, std::less<>>;

UmdRegkeyMap ParseUmdRegkeys(const char *options, size_t size);

class UmdRegkeyScope {
public:
  UmdRegkeyScope(const char *options, size_t size);
  ~UmdRegkeyScope();

  UmdRegkeyScope(const UmdRegkeyScope &) = delete;
  UmdRegkeyScope &operator=(const UmdRegkeyScope &) = delete;

private:
  UmdRegkeyMap options;
  const UmdRegkeyMap *previous = nullptr;
};

const UmdRegkeyMap *GetCurrentUmdRegkeys();

template <bool AllowPerPsoUmdOverride, typename T> T GetUmdOverrideOrDefault(const char *name, T defaultValue) {
  if constexpr (!AllowPerPsoUmdOverride) {
    return defaultValue;
  } else {
    const UmdRegkeyMap *options = GetCurrentUmdRegkeys();
    if (!options)
      return defaultValue;

    auto value = options->find(name);
    if (value == options->end())
      return defaultValue;

    const uint32_t *numericValue = std::get_if<uint32_t>(&value->second);
    return numericValue ? static_cast<T>(*numericValue) : defaultValue;
  }
}

template <bool AllowPerPsoUmdOverride> bool IsUmdRegkeySet(const char *name) {
  if constexpr (!AllowPerPsoUmdOverride) {
    return false;
  } else {
    const UmdRegkeyMap *options = GetCurrentUmdRegkeys();
    return options && options->find(name) != options->end();
  }
}

namespace UmdRegkeyMetadata {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)                               \
  inline constexpr bool regkeyName = false;
#define DECLARE_IGC_REGKEY_UMD(dataType, regkeyName, defaultValue, description, releaseMode)                           \
  inline constexpr bool regkeyName = true;
#define DECLARE_IGC_REGKEY_ENUM_UMD(regkeyName, defaultValue, description, values, releaseMode)                        \
  inline constexpr bool regkeyName = true;
#define DECLARE_IGC_REGKEY_BITMASK_UMD(regkeyName, defaultValue, description, values, releaseMode)                     \
  inline constexpr bool regkeyName = true;
#include "igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
#undef DECLARE_IGC_REGKEY_UMD
#undef DECLARE_IGC_REGKEY_ENUM_UMD
#undef DECLARE_IGC_REGKEY_BITMASK_UMD
} // namespace UmdRegkeyMetadata
} // namespace IGC

#if defined(_DEBUG) || defined(_INTERNAL)
#define IGC_DEBUG_VARIABLES
#endif

#if defined(__linux__)
#define IGC_DEBUG_VARIABLES
#endif


#include <vector>
#if defined(IGC_DEBUG_VARIABLES)
struct HashRange {
  enum class Type { Asm, Pso, Pipeline };

  unsigned long long start;
  unsigned long long end;
  Type Ty;
  bool implied = false;
  union {
    unsigned m_Value;
    debugString m_string;
  };

  uint64_t getHashVal(const ShaderHash &Hash) const {
    switch (Ty) {
    case Type::Asm:
      return Hash.getAsmHash();
    case Type::Pso:
      return Hash.getPsoHash();
    case Type::Pipeline:
      return Hash.getPipelineHash();
    }
    return {};
  }
};

struct EntryPoint {
  std::string entry_point_name;
  union {
    unsigned m_Value;
    debugString m_string;
  };
};

enum IGCFlagType {
  IGCFlagType_int = 0,
  IGCFlagType_DWORD = IGCFlagType_int,
  IGCFlagType_bool = IGCFlagType_int,
  IGCFlagType_debugString = 1,
};

struct IGCFlag {
  union {
    unsigned m_Value;
    debugString m_string;
  };
  std::vector<HashRange> hashes;
  std::vector<EntryPoint> entry_points;
  bool isSet = false;
  const bool isReleaseMode = false;
  const char *name;
  const unsigned defaultValue;
  IGCFlagType type;

  IGCFlag(unsigned value, const char *name_, bool releaseMode, IGCFlagType type_)
      : m_Value(value), isReleaseMode(releaseMode), name(name_), defaultValue(value), type(type_) {}

  bool IsNumber() { return type == IGCFlagType_int; }

  bool IsString() { return type == IGCFlagType_debugString; }

  bool IsSetToNonDefaultValue() {
    if (IsString()) {
      return m_string[0] != '\0';
    } else {
      return m_Value != defaultValue;
    }
  }
};

#if defined(__linux__) && !defined(_DEBUG) && !defined(_INTERNAL)
#define LINUX_RELEASE_MODE
#endif

// XMACRO defining the regkeys
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) regkeyName,
enum class IGCFlagIndex {
#include "igc_regkeys.h"
  IGCFlagIndexCount
};
#undef DECLARE_IGC_REGKEY

using IGCFlagsArray = std::array<IGCFlag, static_cast<std::size_t>(IGCFlagIndex::IGCFlagIndexCount)>;
extern IGCFlagsArray g_IGCFlagsArray;
#define IGC_GET_REGKEY(name) (g_IGCFlagsArray[static_cast<std::size_t>(IGCFlagIndex::name)])
bool CheckHashRange(IGCFlag &varname);
bool CheckEntryPoint(IGCFlag &varname);
void setImpliedRegkey(IGCFlag &name, const bool set, IGCFlag &subname, const unsigned value);

inline bool doesRegexMatch(const std::string &src, const char *regex) {
  return (regex && *regex != '\0') ? std::regex_search(src, std::regex(regex)) : true;
}

template <bool AllowPerPsoUmdOverride> inline unsigned GetUmdAwareFlagValue(IGCFlag &flag, bool developerValueApplies) {
  if (developerValueApplies && (flag.isSet || flag.m_Value != flag.defaultValue))
    return flag.m_Value;
  return IGC::GetUmdOverrideOrDefault<AllowPerPsoUmdOverride>(flag.name, flag.defaultValue);
}

template <bool AllowPerPsoUmdOverride> inline bool IsUmdAwareFlagSet(IGCFlag &flag, bool developerValueApplies) {
  return (developerValueApplies && flag.isSet) || IGC::IsUmdRegkeySet<AllowPerPsoUmdOverride>(flag.name);
}

#if defined(LINUX_RELEASE_MODE)
#define IGC_GET_FLAG_VALUE(name)                                                                                       \
  (GetUmdAwareFlagValue<IGC::UmdRegkeyMetadata::name>(                                                                 \
      IGC_GET_REGKEY(name), (CheckHashRange(IGC_GET_REGKEY(name)) || CheckEntryPoint(IGC_GET_REGKEY(name))) &&         \
                                IGC_GET_REGKEY(name).isReleaseMode))
#define IGC_GET_REGKEYSTRING(name)                                                                                     \
  (((CheckHashRange(IGC_GET_REGKEY(name)) || CheckEntryPoint(IGC_GET_REGKEY(name))) &&                                 \
    IGC_GET_REGKEY(name).isReleaseMode)                                                                                \
       ? IGC_GET_REGKEY(name).m_string                                                                                 \
       : "")
#define IGC_IS_FLAG_SET(name)                                                                                          \
  (IsUmdAwareFlagSet<IGC::UmdRegkeyMetadata::name>(IGC_GET_REGKEY(name), CheckHashRange(IGC_GET_REGKEY(name)) ||       \
                                                                             CheckEntryPoint(IGC_GET_REGKEY(name))))
#else
#define IGC_GET_FLAG_VALUE(name)                                                                                       \
  (GetUmdAwareFlagValue<IGC::UmdRegkeyMetadata::name>(                                                                 \
      IGC_GET_REGKEY(name), CheckHashRange(IGC_GET_REGKEY(name)) || CheckEntryPoint(IGC_GET_REGKEY(name))))
#define IGC_GET_REGKEYSTRING(name)                                                                                     \
  ((CheckHashRange(IGC_GET_REGKEY(name)) || CheckEntryPoint(IGC_GET_REGKEY(name))) ? IGC_GET_REGKEY(name).m_string : "")
#define IGC_IS_FLAG_SET(name)                                                                                          \
  (IsUmdAwareFlagSet<IGC::UmdRegkeyMetadata::name>(IGC_GET_REGKEY(name), CheckHashRange(IGC_GET_REGKEY(name)) ||       \
                                                                             CheckEntryPoint(IGC_GET_REGKEY(name))))
#endif

#define IGC_GET_FLAG_DEFAULT_VALUE(name) (IGC_GET_REGKEY(name).defaultValue)
#define IGC_IS_FLAG_ENABLED(name) (IGC_GET_FLAG_VALUE(name) != 0)
#define IGC_IS_FLAG_DISABLED(name) (!IGC_IS_FLAG_ENABLED(name))
#define IGC_SET_FLAG_VALUE(name, regkeyValue) (IGC_GET_REGKEY(name).m_Value = regkeyValue)
#define IGC_SET_IMPLIED_REGKEY(name, setOnValue, subname, subvalue)                                                    \
  (setImpliedRegkey(IGC_GET_REGKEY(name), (IGC_GET_REGKEY(name).m_Value == setOnValue), IGC_GET_REGKEY(subname),       \
                    subvalue))
#define IGC_SET_IMPLIED_REGKEY_ANY(name, subname, subvalue)                                                            \
  (setImpliedRegkey(IGC_GET_REGKEY(name), (IGC_GET_REGKEY(name).m_Value != 0), IGC_GET_REGKEY(subname), subvalue))
#define IGC_REGKEY_OR_FLAG_ENABLED(name, flag)                                                                         \
  (IGC_IS_FLAG_ENABLED(name) || IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::flag))

#if defined(_WIN64) || defined(_WIN32)
struct DEVICE_INFO {
  std::string description;
  DWORD deviceID;
  DWORD revisionID;
  DWORD pciBus;
  DWORD pciDevice;
  DWORD pciFunction;
  std::string driverRegistryPath;

  DEVICE_INFO(DEVINST deviceInstance);

  void get_device_property(DEVINST deviceInstace, DWORD property);
};
#endif

void GetKeysSetExplicitly(std::string *KeyValuePairs, std::string *OptionKeys);
void LoadIGCFlagsFromRegistryAndGetString(std::string &outToken);
void DumpIGCRegistryKeyDefinitions();
void DumpIGCRegistryKeyDefinitions3(std::string driverRegistryPath, unsigned long pciBus, unsigned long pciDevice,
                                    unsigned long pciFunction);
void InitializeRegKeys();
void LoadRegistryKeys(const std::string &options = "", std::string *optionsParseError = nullptr);
extern "C" bool ReadIGCRegistry(const char *pName, void *pValue, unsigned int size, IGCFlagType type);
void SetCurrentDebugHash(const ShaderHash &hash);
void SetCurrentEntryPoints(const std::vector<std::string> &entry_points);
void ClearCurrentEntryPoints();
IGCFlag *FindIGCFlagByName(const char *name);
#undef LINUX_RELEASE_MODE
#else
static inline void GetKeysSetExplicitly(std::string *KeyValuePairs, std::string *OptionKeys) {
  IGC_UNUSED(KeyValuePairs);
  IGC_UNUSED(OptionKeys);
}
static inline void LoadIGCFlagsFromRegistryAndGetString(std::string &outToken) { IGC_UNUSED(outToken); }
static inline void SetCurrentDebugHash(const ShaderHash &hash) { IGC_UNUSED(hash); }
static inline void SetCurrentEntryPoints(const std::vector<std::string> &entry_points) { IGC_UNUSED(entry_points); }
static inline void InitializeRegKeys() {};
static inline void LoadRegistryKeys(const std::string &options = "", std::string *optionsParseError = nullptr) {
  IGC_UNUSED(options);
  IGC_UNUSED(optionsParseError);
}
#define IGC_SET_FLAG_VALUE(name, regkeyValue) true
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)                               \
  static const unsigned int regkeyName##default = (unsigned int)defaultValue;
namespace IGC {
class DebugVariable {
public:
#include "igc_regkeys.h"
};
}; // namespace IGC
#undef DECLARE_IGC_REGKEY

template <typename T> bool IsEnabled(const T &value) { return value != 0; }

#define IGC_GET_FLAG_VALUE(name)                                                                                       \
  (IGC::GetUmdOverrideOrDefault<IGC::UmdRegkeyMetadata::name>(#name, IGC::DebugVariable::name##default))
#define IGC_IS_FLAG_ENABLED(name) (::IsEnabled(IGC_GET_FLAG_VALUE(name)))
#define IGC_IS_FLAG_DISABLED(name) (!IGC_IS_FLAG_ENABLED(name))
#define IGC_IS_FLAG_SET(name) (IGC::IsUmdRegkeySet<IGC::UmdRegkeyMetadata::name>(#name))
#define IGC_GET_FLAG_DEFAULT_VALUE(name) (IGC::DebugVariable::name##default)
#define IGC_GET_REGKEYSTRING(name) ("")
#define IGC_REGKEY_OR_FLAG_ENABLED(name, flag)                                                                         \
  (IGC_IS_FLAG_ENABLED(name) || IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::flag))
#define IGC_SET_IMPLIED_REGKEY(name, setOnValue, subname, subvalue)                                                    \
  {                                                                                                                    \
  }
#define IGC_SET_IMPLIED_REGKEY_ANY(name, subname, subvalue)                                                            \
  {                                                                                                                    \
  }
inline bool doesRegexMatch(const std::string &src, const char *regex) { return false; }
#endif

// unset: Unlimited/Enable - return true
//     0: Disabled         - return false
//   >=1: Enable           - key_value -= 1; return true
#define IGC_COUNT_FLAG_IN_LIMIT(name)                                                                                  \
  (!IGC_IS_FLAG_SET(name)      ? true                                                                                  \
   : !IGC_GET_FLAG_VALUE(name) ? false                                                                                 \
                               : (IGC_SET_FLAG_VALUE(name, IGC_GET_FLAG_VALUE(name) - 1) || true))

namespace IGC {
enum class TriboolFlag : int {
#define TRIBOOL_OPTION(Name, Val) Name = Val,
#include "igc_regkeys_enums_defs.h"
  TRIBOOL_OPTIONS
#undef TRIBOOL_OPTION
#undef TRIBOOL_OPTIONS
};
enum class NewInlineRaytracingMask : unsigned {
  None = 0,
#define NEW_INLINE_RAYTRACING_FLAG(Name, Val, Description) Name = Val,
#include "igc_regkeys_enums_defs.h"
  NEW_INLINE_RAYTRACING_MASK
#undef NEW_INLINE_RAYTRACING_FLAG
#undef NEW_INLINE_RAYTRACING_MASK
};
} // namespace IGC

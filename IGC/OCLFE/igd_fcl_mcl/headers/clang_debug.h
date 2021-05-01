/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CLANG_DEBUG_H
#define CLANG_DEBUG_H

#if defined _WIN32

#if defined(_DEBUG) || defined(_INTERNAL)

#include "../headers/RegistryAccess.h"

using namespace std;

#define CLANG_REGKEY   "Software\\Intel\\IGFX\\OCL"

namespace clang
{

  /*****************************************************************************
  MACRO: OCL_CLANG_DeclareBool
  PURPOSE: Declares a new BOOL debug variable. Use only inside
  DEBUG_VARIABLES struct definition.
  \*****************************************************************************/
#define OCL_CLANG_DeclareBool( name, group, description )    \
  struct SDebugVariableMetaData_##name : public SDebugVariableMetaData \
  {                                                       \
  const char* GetName() const                         \
  {                                                       \
  return #name;                                       \
}                                                       \
  const char* GetGroup() const                        \
  {                                                       \
  return group;                                       \
}                                                       \
  const char* GetType() const                         \
  {                                                       \
  return "bool";                                      \
}                                                       \
  const char* GetDescription() const                  \
  {                                                       \
  return description;                                 \
}                                                       \
  const DWORD Value() const                           \
  {                                                       \
  return m_Value;                                     \
}                                                       \
} name


  /*****************************************************************************\
  MACRO: OCL_CLANG_DeclareDword
  PURPOSE: Declares a new DWORD debug variable. Use only inside
  DEBUG_VARIABLES struct definition.
  \*****************************************************************************/
#define OCL_CLANG_DeclareDword( name, group, description )    \
  struct SDebugVariableMetaData_##name : public SDebugVariableMetaData \
  {                                                       \
  const char* GetName() const                         \
  {                                                       \
  return #name;                                       \
}                                                       \
  const char* GetGroup() const                        \
  {                                                       \
  return group;                                       \
}                                                       \
  const char* GetType() const                         \
  {                                                       \
  return "dword";                                     \
}                                                       \
  const char* GetDescription() const                  \
  {                                                       \
  return description;                                 \
}                                                       \
  const DWORD Value() const                           \
  {                                                       \
  return m_Value;                                     \
}                                                       \
} name


  /*****************************************************************************\
  MACRO: OCL_CLANG_DeclareBitmask
  PURPOSE: Declares a new bitmask debug variable. Use only inside
  DEBUG_VARIABLES struct definition.
  \*****************************************************************************/
#define OCL_CLANG_DeclareBitmask( name, group, description, values )    \
  struct SDebugVariableMetaData_##name : public SDebugVariableMetaData \
  {                                                       \
  const char* GetName() const                         \
  {                                                       \
  return #name;                                       \
}                                                       \
  const char* GetGroup() const                        \
  {                                                       \
  return group;                                       \
}                                                       \
  const char* GetType() const                         \
  {                                                       \
  return "bitmask";                                   \
}                                                       \
  const char* GetDescription() const                  \
  {                                                       \
  return description;                                 \
}                                                       \
  const DWORD Value() const                           \
  {                                                       \
  return m_Value;                                     \
}                                                       \
  virtual void DumpDefinition( FILE* fp ) const       \
  {                                                       \
  DumpBitmaskDefinition( fp, values, sizeof(values)/sizeof(SDebugVariableBitmaskValue) ); \
}                                                       \
} name

  /*****************************************************************************\
  MACRO: OCL_CLANG_DeclareString
  PURPOSE: Declares a new string debug variable. Use only inside
  DEBUG_VARIABLES struct definition.
  \*****************************************************************************/
#define OCL_CLANG_DeclareString( name, group, description )          \
  struct SDebugVariableMetaData_##name : public SDebugVariableMetaData \
  {                                                       \
  const char* GetName() const                         \
  {                                                   \
  return #name;                                   \
}                                                   \
  const char* GetGroup() const                        \
  {                                                   \
  return group;                                   \
}                                                   \
  const char* GetType() const                         \
  {                                                   \
  return "string";                                \
}                                                   \
  const char* GetDescription() const                  \
  {                                                   \
  return description;                             \
}                                                   \
} name

  /*****************************************************************************\
  MACRO: OCL_CLANG_DeclareEnum
  PURPOSE: Declares a new enum debug variable. Use only inside
  DEBUG_VARIABLES struct definition.
  \*****************************************************************************/
#define OCL_CLANG_DeclareEnum( name, group, description, values )    \
  struct SDebugVariableMetaData_##name : public SDebugVariableMetaData \
  {                                                       \
  const char* GetName() const                         \
  {                                                       \
  return #name;                                       \
}                                                       \
  const char* GetGroup() const                        \
  {                                                       \
  return group;                                       \
}                                                       \
  const char* GetType() const                         \
  {                                                       \
  return "enum";                                      \
}                                                       \
  const char* GetDescription() const                  \
  {                                                       \
  return description;                                 \
}                                                       \
  const DWORD Value() const                           \
  {                                                       \
  return m_Value;                                     \
}                                                       \
  virtual void DumpDefinition( FILE* fp ) const       \
  {                                                       \
  DumpEnumDefinition( fp, values, sizeof(values)/sizeof(SDebugVariableEnumValue) ); \
}                                                       \
} name

  //00
  /*****************************************************************************\
  STRUCT: SDebugVariableBitmaskValue
  PURPOSE: Defines a possible debug variable bitmask value
  \*****************************************************************************/
  struct SDebugVariableBitmaskValue
  {
    const char* Name;
    const DWORD Value;
  };

  //01
  /*****************************************************************************\
  STRUCT: SDebugVariableEnumValue
  PURPOSE: Defines a possible debug variable enum value
  \*****************************************************************************/
  struct SDebugVariableEnumValue
  {
    const char* Name;
    const DWORD Value;
  };

  //02
  /*****************************************************************************\
  STRUCT: SDebugVariableMetaData
  PURPOSE: Defines meta data for holding/defining debug variables
  \*****************************************************************************/
  struct SDebugVariableMetaData
  {
    DWORD m_Value;
    BOOL m_IsSet;

    SDebugVariableMetaData()
      : m_Value(0), m_IsSet(FALSE)
    {

    }

    virtual const char* GetName() const = 0;
    virtual const char* GetDescription() const = 0;
    virtual const char* GetType() const = 0;
    virtual const char* GetGroup() const = 0;

    virtual void DumpDefinition(FILE* fp) const
    {
      fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" description=\"%s\" />\n",
        GetName(),
        GetType(),
        "HKLM\\" CLANG_REGKEY,
        GetDescription());
    }

    void DumpEnumDefinition(FILE* fp, const SDebugVariableEnumValue* pList, const size_t count) const
    {
      fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" description=\"%s [VALUES] ",
        GetName(),
        GetType(),
        "HKLM\\" CLANG_REGKEY,
        GetDescription());
      for (size_t i = 0; i < count; i++)
      {
        const char* comma = (i == 0) ? "" : ",";
        fprintf(fp, "%s%s=0x%08x", comma, pList[i].Name, pList[i].Value);
      }
      fprintf(fp, "\" />\n");
    }

    void DumpBitmaskDefinition(FILE* fp, const SDebugVariableBitmaskValue* pList, const size_t count) const
    {
      fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" description=\"%s [VALUES] ",
        GetName(),
        GetType(),
        "HKLM\\" CLANG_REGKEY,
        GetDescription());
      for (size_t i = 0; i < count; i++)
      {
        const char* comma = (i == 0) ? "" : ",";
        fprintf(fp, "%s%s=0x%08x", comma, pList[i].Name, pList[i].Value);
      }
      fprintf(fp, "\" />\n");
    }
  };

  /*****************************************************************************\
  External API call to DumpRegistryDefinations
  \*****************************************************************************/

  struct DEBUG_VARIABLES_REGKEY;

  extern DEBUG_VARIABLES_REGKEY g_DebugVariables;

  typedef void(__cdecl *PFNLOADDEBUGVARIABLES)(void);

  extern PFNLOADDEBUGVARIABLES pfnLoadDebugVariables;

  extern "C" __declspec(dllexport) void __cdecl DumpRegistryKeyDefinitions(void);

} // namespace clang

#endif // defined(_DEBUG) || defined(_INTERNAL)

#endif // _WIN32

#endif // CLANG_DEBUG_H

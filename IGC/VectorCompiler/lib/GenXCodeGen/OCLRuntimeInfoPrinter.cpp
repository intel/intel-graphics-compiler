/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file implements interfaces for OCL runtime info printing.
//
// LLVM YAML serializer is used to transform descriptive structures into YAML.
//
//===----------------------------------------------------------------------===//

#include "OCLRuntimeInfoPrinter.h"

#include "Probe/Assertion.h"

#include <llvm/Support/Error.h>
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

using namespace llvm;
using CompiledModuleT = GenXOCLRuntimeInfo::CompiledModuleT;
using ModuleInfoT = GenXOCLRuntimeInfo::ModuleInfoT;
using SectionInfo = GenXOCLRuntimeInfo::SectionInfo;
using DataInfo = GenXOCLRuntimeInfo::DataInfo;

LLVM_YAML_IS_SEQUENCE_VECTOR(vISA::ZESymEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(vISA::ZERelocEntry)

#define ENUM_MAP(type, name) io.enumCase(Val, #name, type::name)
template <> struct yaml::ScalarEnumerationTraits<vISA::GenSymType> {
  static void enumeration(yaml::IO &io, vISA::GenSymType &Val) {
    ENUM_MAP(vISA::GenSymType, S_NOTYPE);
    ENUM_MAP(vISA::GenSymType, S_UNDEF);
    ENUM_MAP(vISA::GenSymType, S_FUNC);
    ENUM_MAP(vISA::GenSymType, S_GLOBAL_VAR);
    ENUM_MAP(vISA::GenSymType, S_GLOBAL_VAR_CONST);
    ENUM_MAP(vISA::GenSymType, S_CONST_SAMPLER);
    ENUM_MAP(vISA::GenSymType, S_KERNEL);
  }
};

template <> struct yaml::ScalarEnumerationTraits<vISA::GenRelocType> {
  static void enumeration(yaml::IO &io, vISA::GenRelocType &Val) {
    ENUM_MAP(vISA::GenRelocType, R_NONE);
    ENUM_MAP(vISA::GenRelocType, R_SYM_ADDR);
    ENUM_MAP(vISA::GenRelocType, R_SYM_ADDR_32);
    ENUM_MAP(vISA::GenRelocType, R_SYM_ADDR_32_HI);
    ENUM_MAP(vISA::GenRelocType, R_PER_THREAD_PAYLOAD_OFFSET_32);
  }
};
#undef ENUM_MAP

#define MAPPING(name) io.mapOptional(#name, Info.name)
template <> struct yaml::MappingTraits<CompiledModuleT> {
  static void mapping(yaml::IO &io, CompiledModuleT &Info) {
    MAPPING(ModuleInfo);
  }
};

template <> struct yaml::MappingTraits<ModuleInfoT> {
  static void mapping(yaml::IO &io, ModuleInfoT &Info) {
    MAPPING(Constant);
    MAPPING(Global);
  }
};

template <> struct yaml::MappingTraits<SectionInfo> {
  static void mapping(yaml::IO &io, SectionInfo &Info) {
    MAPPING(Data);
    MAPPING(Symbols);
    MAPPING(Relocations);
  }
};

template <> struct yaml::MappingTraits<DataInfo> {
  static void mapping(yaml::IO &io, DataInfo &Info) {
    MAPPING(Buffer);
    MAPPING(Alignment);
    MAPPING(AdditionalZeroedSpace);
  }
};

template <> struct yaml::MappingTraits<vISA::ZESymEntry> {
  static void mapping(yaml::IO &io, vISA::ZESymEntry &Info) {
    MAPPING(s_type);
    MAPPING(s_offset);
    MAPPING(s_size);
    MAPPING(s_name);
  }
};

template <> struct yaml::MappingTraits<vISA::ZERelocEntry> {
  static void mapping(yaml::IO &io, vISA::ZERelocEntry &Info) {
    MAPPING(r_type);
    MAPPING(r_offset);
    MAPPING(r_symbol);
  }
};
#undef MAPPING

void vc::printOCLRuntimeInfo(raw_ostream &OS,
                             const CompiledModuleT &CompiledModule) {
  yaml::Output OutYAML{OS};
  OutYAML << const_cast<CompiledModuleT &>(CompiledModule);
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CISALinker.h"
#include <stdio.h>

// *** Macros ***

#define ASSERT(assertion, ...)                                                 \
  if (!(assertion)) {                                                          \
    fprintf(stderr, "Error: " __VA_ARGS__);                                    \
    fprintf(stderr, "!\n");                                                    \
    return 1;                                                                  \
  }

#define ALLOC_ASSERT(allocation)                                               \
  ASSERT(allocation, "Memory allocation failure at %s:%u", __FILE__, __LINE__);

#define INTERNAL_ASSERT(assertion)                                             \
  ASSERT(assertion, "Internal error at %s:%u", __FILE__, __LINE__)

#define TRY(x)                                                                 \
  if ((x)) {                                                                   \
    return 1;                                                                  \
  }

#define BUF_APPEND_SCALAR(X)                                                   \
  memcpy((char *)_cisaObjInfos[_numCisaObjInfos].buf + _cisaEmitBufOffset, &X, \
         sizeof(X));                                                           \
  _cisaEmitBufOffset += sizeof(X);

#define BUF_APPEND_VECTOR(X, Y)                                                \
  memcpy((char *)_cisaObjInfos[_numCisaObjInfos].buf + _cisaEmitBufOffset,     \
         (X), Y);                                                              \
  _cisaEmitBufOffset += Y;

#define CLOSURE(x) ((x).scratch)
#define UNITCLOSURE(x) (((CompiledUnitClosure *)CLOSURE(x)))
#define VARCLOSURE(x) (((CompiledVarClosure *)CLOSURE(x)))

#define cisaBytePos byte_pos

// *** Public functions ***

CISALinker::CISALinker(int numOptions, const char *options[],
                       vISA::Mem_Manager &mem,
                       ExternalHeapAllocator *externalAllocator)
    : _numOptions(numOptions), _options(options), _mem(mem),
      _countLinkedUnits(0), _countLinkedVars(0), _cisaEmitBufOffset(0),
      _numCisaObjInfos(0), _cisaObjInfos(NULL) {
  _linkedUnitIndexToClosureMap.reserve(64);
  if (externalAllocator) {
    _allocator = externalAllocator;
  } else {
    _allocator = (ExternalHeapAllocator *)malloc;
  }
}

CISALinker::~CISALinker() {}

int CISALinker::LinkCisaMemObjs(const char *knlName, int numCisaObjs,
                                const CisaObj cisaObjs[],
                                CisaObj &cisaLinkedObj) {
  TRY(AllocateCisaObjInfos(numCisaObjs));

  for (int i = 0; i < numCisaObjs; i++) {
    TRY(ReadCisaMemObj(cisaObjs[i], _cisaObjInfos[i]));
  }

  std::string knlNameStr(knlName);
  TRY(LinkCisaObjInfos(knlNameStr));
  TRY(WriteCisaMemObj(cisaLinkedObj));

  return 0;
}

int CISALinker::LinkCisaFileObjs(const char *knlName, int numCisaObjs,
                                 const char *cisaObjs[],
                                 const char *cisaLinkedObj) {
  TRY(AllocateCisaObjInfos(numCisaObjs));

  for (int i = 0; i < numCisaObjs; i++) {
    const char *cisaExt = ".isa";
    const char *cisaObjFile = cisaObjs[i];
    const char *argExt = strstr(cisaObjFile, cisaExt);
    ASSERT(argExt != NULL && argExt[4] == '\0',
           "CISA object file %s must have .isa extension", cisaObjFile);
    TRY(ReadCisaFileObj(cisaObjFile, _cisaObjInfos[i]));
  }

  std::string knlNameStr(knlName);
  TRY(LinkCisaObjInfos(knlNameStr));
  TRY(WriteCisaFileObj(cisaLinkedObj));

  return 0;
}

// *** Private functions ***

int CISALinker::LinkCisaObjInfos(const std::string &knlName) {
  CisaObjInfo &cisaLinkedObjInfo = _cisaObjInfos[_numCisaObjInfos];
  TRY(InitGlobalInfoTables());
  CompiledUnitClosureList cisaKnlClosureList;
  TRY(CreateKernelClosures(knlName, cisaKnlClosureList));
  CompiledUnitClosureList::iterator iter = cisaKnlClosureList.begin();

  for (; iter != cisaKnlClosureList.end(); ++iter) {
    CompiledUnitClosure &knlClosure = **iter;
    CisaObjInfo &knlObjInfo = *knlClosure.unitObjInfo;
    CompiledUnitClosureList localUnitClosureList;
    localUnitClosureList.push_back(&knlClosure);
    TRY(LinkLocalCisaObjInfos(knlObjInfo, localUnitClosureList));
    TRY(LinkExternCisaObjInfos());
  }

  TRY(BuildLinkedCisaObj(cisaKnlClosureList));
  TRY(UpdateLinkedCisaImageOffsetsAndSize());
  ClearGlobalInfoTables();

  return 0;
}

int CISALinker::InitGlobalInfoTables() {
  for (int i = 0; i < _numCisaObjInfos; i++) {
    CisaObjInfo &cisaObjInfo = _cisaObjInfos[i];

    for (int j = 0; j < cisaObjInfo.hdr.num_global_functions; j++) {
      int k = j + cisaObjInfo.hdr.num_extern_functions;
      CompiledUnitInfo &unit = cisaObjInfo.hdr.functions[k];
      CompiledUnitClosure *unitClosure =
          (CompiledUnitClosure *)_mem.alloc(sizeof(CompiledUnitClosure));
      unitClosure->unit = &unit;
      unitClosure->unitObjInfo = &cisaObjInfo;
      unitClosure->linkedUnitIndex = -1;
      CLOSURE(unit) = unitClosure;
      std::string key(unit.name, unit.name_len);
      std::pair<GlobalUnitNameToInfoMap::iterator, bool> loc =
          _globalUnitNameToInfoMap.insert(
              GlobalUnitNameToInfoMap::value_type(key, &unit));
      ASSERT(loc.second, "Found duplicate global functions for %s",
             key.c_str());
    }
  }

  for (int i = 0; i < _numCisaObjInfos; i++) {
    CisaObjInfo &cisaObjInfo = _cisaObjInfos[i];

    for (int j = 0; j < cisaObjInfo.hdr.num_global_variables; j++) {
      int k = j + cisaObjInfo.hdr.num_extern_variables;
      CompiledVarInfo &var = cisaObjInfo.hdr.filescope_variables[k];
      CompiledVarClosure *varClosure =
          (CompiledVarClosure *)_mem.alloc(sizeof(CompiledVarClosure));
      varClosure->var = &var;
      varClosure->varObjInfo = &cisaObjInfo;
      varClosure->linkedVarIndex = -1;
      CLOSURE(var) = varClosure;
      std::string key((char *)var.name, var.name_len);
      std::pair<GlobalVarNameToInfoMap::iterator, bool> loc =
          _globalVarNameToInfoMap.insert(
              GlobalVarNameToInfoMap::value_type(key, &var));
      ASSERT(loc.second, "Found duplicate global variables for %s",
             key.c_str());
    }
  }

  return 0;
}

inline void CISALinker::ClearGlobalInfoTables() {
  _globalUnitNameToInfoMap.clear();
  _globalVarNameToInfoMap.clear();
}

inline int CISALinker::GetGlobalUnitInfo(const std::string &unitName,
                                         CompiledUnitInfo *&globalUnitInfo) {
  GlobalUnitNameToInfoMap::iterator loc =
      _globalUnitNameToInfoMap.find(unitName);
  ASSERT(loc != _globalUnitNameToInfoMap.end(),
         "Cannot find global function %s", unitName.c_str());
  globalUnitInfo = (*loc).second;

  return 0;
}

inline int
CISALinker::GetGlobalUnitClosure(const std::string &unitName,
                                 CompiledUnitClosure *&globalUnitClosure) {
  CompiledUnitInfo *globalUnitInfo = NULL;
  TRY(GetGlobalUnitInfo(unitName, globalUnitInfo));
  globalUnitClosure = UNITCLOSURE(*globalUnitInfo);
  INTERNAL_ASSERT(globalUnitClosure);

  return 0;
}

inline int CISALinker::GetGlobalVarInfo(const std::string &unitName,
                                        CompiledVarInfo *&globalVarInfo) {
  GlobalVarNameToInfoMap::iterator loc = _globalVarNameToInfoMap.find(unitName);
  ASSERT(loc != _globalVarNameToInfoMap.end(), "Cannot find global variable %s",
         unitName.c_str());
  globalVarInfo = (*loc).second;

  return 0;
}

inline int
CISALinker::GetGlobalVarClosure(const std::string &unitName,
                                CompiledVarClosure *&globalVarClosure) {
  CompiledVarInfo *globalVarInfo = NULL;
  TRY(GetGlobalVarInfo(unitName, globalVarInfo));
  globalVarClosure = VARCLOSURE(*globalVarInfo);
  INTERNAL_ASSERT(globalVarClosure);

  return 0;
}

int CISALinker::CreateKernelClosures(const std::string &knlName,
                                     CompiledUnitClosureList &knlClosureList) {
  for (int i = 0; i < _numCisaObjInfos; i++) {
    CisaObjInfo &cisaObjInfo = _cisaObjInfos[i];

    for (int j = 0; j < cisaObjInfo.hdr.num_kernels; j++) {
      CompiledUnitInfo &kernel = cisaObjInfo.hdr.kernels[j];

      if ((knlName.size() == 0) ||
          (knlName.size() == kernel.name_len &&
           strncmp(kernel.name, knlName.c_str(), knlName.size()) == 0)) {
        CompiledUnitClosure *knlClosure =
            (CompiledUnitClosure *)_mem.alloc(sizeof(CompiledUnitClosure));
        ALLOC_ASSERT(knlClosure);
        knlClosure->unit = &kernel;
        knlClosure->unitObjInfo = &cisaObjInfo;
        knlClosure->linkedUnitIndex = 0;
        CLOSURE(kernel) = knlClosure;
        knlClosureList.push_back(knlClosure);

        if (knlName.size() && knlClosureList.size()) {
          break;
        }
      }

      if (knlName.size() && knlClosureList.size()) {
        break;
      }
    }
  }

  if (knlName.size()) {
    ASSERT(knlClosureList.size() > 0, "Cannot find kernel %s", knlName.c_str());
  } else {
    ASSERT(knlClosureList.size() > 0,
           "Cannot find any kernel in input CISA objects");
  }

  return 0;
}

int CISALinker::LinkLocalCisaObjInfos(
    CisaObjInfo &cisaObjInfo, CompiledUnitClosureList &localUnitClosureList) {
  _localUnitMap = cisaObjInfo.hdr.functions;
  _localVarMap = cisaObjInfo.hdr.filescope_variables;
  CompiledUnitClosureList::iterator iter = localUnitClosureList.begin();

  for (; iter != localUnitClosureList.end(); ++iter) {
    TRY(LinkInDepUnits(**iter));
    TRY(LinkInDepVars(**iter));
  }

  for (iter = _pendingLocalClosures.begin();
       iter != _pendingLocalClosures.end(); ++iter) {
    TRY(LinkInDepUnits(**iter));
    TRY(LinkInDepVars(**iter));
  }

  for (iter = _pendingLocalClosures.begin();
       iter != _pendingLocalClosures.end(); ++iter) {
    TRY(RelocFunctionSyms(**iter));
    TRY(RelocVariableSyms(**iter));
  }

  for (iter = localUnitClosureList.begin(); iter != localUnitClosureList.end();
       ++iter) {
    TRY(RelocFunctionSyms(**iter));
    TRY(RelocVariableSyms(**iter));
  }

  _pendingLocalClosures.clear();
  _localUnitMap = NULL;
  _localVarMap = NULL;

  return 0;
}

int CISALinker::LinkExternCisaObjInfos() {
  CompiledUnitClosureList::iterator iter = _pendingExternClosures.begin();

  for (; iter != _pendingExternClosures.end(); ++iter) {
    CompiledUnitClosureList localUnitClosuresList;
    localUnitClosuresList.push_back(*iter);

    CompiledUnitClosureList::iterator jter = iter;

    for (++jter; jter != _pendingExternClosures.end();) {

      if ((*jter)->unitObjInfo == (*iter)->unitObjInfo) {
        localUnitClosuresList.push_back(*jter);
        CompiledUnitClosureList::iterator kter = jter++;
        _pendingExternClosures.erase(kter);
      } else {
        ++jter;
      }
    }

    CisaObjInfo &unitObjInfo = *(*iter)->unitObjInfo;
    TRY(LinkLocalCisaObjInfos(unitObjInfo, localUnitClosuresList));
  }

  _pendingExternClosures.clear();

  return 0;
}

int CISALinker::LinkInDepUnits(CompiledUnitClosure &cisaUnitClosure) {
  CompiledUnitInfo &cisaUnitInfo = *cisaUnitClosure.unit;
  int globalStartIndex = cisaUnitClosure.unitObjInfo->hdr.num_extern_functions;
  int staticStartIndex =
      globalStartIndex + cisaUnitClosure.unitObjInfo->hdr.num_global_functions;

  for (int i = 0; i < cisaUnitInfo.function_reloc_symtab.num_syms; i++) {
    int inputRefIndex =
        cisaUnitInfo.function_reloc_symtab.reloc_syms[i].resolved_index;

    if (inputRefIndex >= staticStartIndex) {

      if (CLOSURE(_localUnitMap[inputRefIndex]) == NULL) {
        CompiledUnitClosure *depUnitClosure =
            (CompiledUnitClosure *)_mem.alloc(sizeof(CompiledUnitClosure));
        CLOSURE(_localUnitMap[inputRefIndex]) = depUnitClosure;
        depUnitClosure->unit =
            &cisaUnitClosure.unitObjInfo->hdr.functions[inputRefIndex];
        depUnitClosure->unitObjInfo = cisaUnitClosure.unitObjInfo;
        depUnitClosure->linkedUnitIndex = _countLinkedUnits++;
        _linkedUnitIndexToClosureMap.push_back(depUnitClosure);
        _pendingLocalClosures.push_back(depUnitClosure);
      }
    } else if (inputRefIndex >= globalStartIndex) {
      CompiledUnitClosure *depUnitClosure =
          UNITCLOSURE(_localUnitMap[inputRefIndex]);
      INTERNAL_ASSERT(depUnitClosure);

      if (depUnitClosure->linkedUnitIndex == -1) {
        depUnitClosure->linkedUnitIndex = _countLinkedUnits++;
        _linkedUnitIndexToClosureMap.push_back(depUnitClosure);
        _pendingLocalClosures.push_back(depUnitClosure);
      }
    } else {
      CompiledUnitInfo &unit =
          cisaUnitClosure.unitObjInfo->hdr.functions[inputRefIndex];
      CompiledUnitClosure *externUnitClosure = NULL;
      std::string unitName(unit.name, unit.name_len);
      TRY(GetGlobalUnitClosure(unitName, externUnitClosure));

      if (externUnitClosure->linkedUnitIndex == -1) {
        externUnitClosure->linkedUnitIndex = _countLinkedUnits++;
        _linkedUnitIndexToClosureMap.push_back(externUnitClosure);
        _pendingExternClosures.push_back(externUnitClosure);
      }
    }
  }

  return 0;
}

int CISALinker::LinkInDepVars(CompiledUnitClosure &cisaUnitClosure) {
  CompiledVarInfo *vars = cisaUnitClosure.unitObjInfo->hdr.filescope_variables;
  CompiledUnitInfo &cisaUnitInfo = *cisaUnitClosure.unit;
  int globalStartIndex = cisaUnitClosure.unitObjInfo->hdr.num_extern_variables;
  int staticStartIndex =
      globalStartIndex + cisaUnitClosure.unitObjInfo->hdr.num_global_variables;

  for (int i = 0; i < cisaUnitInfo.variable_reloc_symtab.num_syms; i++) {
    int inputRefIndex =
        cisaUnitInfo.variable_reloc_symtab.reloc_syms[i].resolved_index;

    if (inputRefIndex >= staticStartIndex) {

      if (CLOSURE(_localVarMap[inputRefIndex]) == NULL) {
        CompiledVarClosure *depVarClosure =
            (CompiledVarClosure *)_mem.alloc(sizeof(CompiledVarClosure));
        CLOSURE(_localVarMap[inputRefIndex]) = depVarClosure;
        CompiledVarInfo &var = vars[inputRefIndex];
        depVarClosure->var = &var;
        depVarClosure->varObjInfo = cisaUnitClosure.unitObjInfo;
        depVarClosure->linkedVarIndex = _countLinkedVars++;
        _linkedVarIndexToClosureMap.push_back(depVarClosure);
      }
    } else if (inputRefIndex >= globalStartIndex) {
      CompiledVarClosure *depVarClosure =
          VARCLOSURE(_localVarMap[inputRefIndex]);
      INTERNAL_ASSERT(depVarClosure);

      if (depVarClosure->linkedVarIndex == -1) {
        depVarClosure->linkedVarIndex = _countLinkedVars++;
        _linkedVarIndexToClosureMap.push_back(depVarClosure);
      }
    } else {
      CompiledVarInfo &variable = vars[inputRefIndex];
      CompiledVarClosure *externVarClosure = NULL;
      std::string varName((char *)variable.name, variable.name_len);
      TRY(GetGlobalVarClosure(varName, externVarClosure));

      if (externVarClosure->linkedVarIndex == -1) {
        externVarClosure->linkedVarIndex = _countLinkedVars++;
        _linkedVarIndexToClosureMap.push_back(externVarClosure);
      }
    }
  }

  return 0;
}

int CISALinker::RelocFunctionSyms(CompiledUnitClosure &localUnitClosure) {
  CompiledUnitInfo &localUnitInfo = *localUnitClosure.unit;

  for (int i = 0; i < localUnitInfo.function_reloc_symtab.num_syms; i++) {
    int inputRefIndex =
        localUnitInfo.function_reloc_symtab.reloc_syms[i].resolved_index;
    int offset = localUnitClosure.unitObjInfo->hdr.num_extern_functions;

    if (inputRefIndex >= offset) {
      int linkedUnitIndex =
          UNITCLOSURE(_localUnitMap[inputRefIndex])->linkedUnitIndex;
      INTERNAL_ASSERT(linkedUnitIndex >= 0);
      localUnitInfo.function_reloc_symtab.reloc_syms[i].resolved_index =
          linkedUnitIndex;
    } else {
      CompiledUnitInfo *units = localUnitClosure.unitObjInfo->hdr.functions;
      CompiledUnitInfo &unit = units[inputRefIndex];
      CompiledUnitClosure *externUnitClosure = NULL;
      std::string unitName(unit.name, unit.name_len);
      TRY(GetGlobalUnitClosure(unitName, externUnitClosure));
      int linkedUnitIndex = externUnitClosure->linkedUnitIndex;
      INTERNAL_ASSERT(linkedUnitIndex >= 0);
      localUnitInfo.function_reloc_symtab.reloc_syms[i].resolved_index =
          linkedUnitIndex;
    }
  }

  return 0;
}

int CISALinker::RelocVariableSyms(CompiledUnitClosure &localUnitClosure) {
  CompiledUnitInfo &localUnitInfo = *localUnitClosure.unit;
  CompiledVarInfo *vars =
      (CompiledVarInfo *)localUnitClosure.unitObjInfo->hdr.filescope_variables;

  for (int i = 0; i < localUnitInfo.variable_reloc_symtab.num_syms; i++) {
    int inputRefIndex =
        localUnitInfo.variable_reloc_symtab.reloc_syms[i].resolved_index;
    int offset = localUnitClosure.unitObjInfo->hdr.num_extern_variables;

    if (inputRefIndex >= offset) {
      int linkedVarIndex =
          VARCLOSURE(_localVarMap[inputRefIndex])->linkedVarIndex;
      INTERNAL_ASSERT(linkedVarIndex >= 0);
      localUnitInfo.variable_reloc_symtab.reloc_syms[i].resolved_index =
          linkedVarIndex;
    } else {
      CompiledVarInfo &variable = vars[inputRefIndex];
      CompiledVarClosure *externVarClosure = NULL;
      std::string unitName((char *)variable.name, variable.name_len);
      TRY(GetGlobalVarClosure(unitName, externVarClosure));
      int linkedVarIndex = externVarClosure->linkedVarIndex;
      INTERNAL_ASSERT(linkedVarIndex >= 0);
      localUnitInfo.variable_reloc_symtab.reloc_syms[i].resolved_index =
          externVarClosure->linkedVarIndex;
    }
  }

  return 0;
}

int CISALinker::BuildLinkedCisaObj(CompiledUnitClosureList &knlClosureList) {
  CisaObjInfo &linkedObjInfo = _cisaObjInfos[_numCisaObjInfos];
  const CisaHeader &firstKnlHdr = knlClosureList.front()->unitObjInfo->hdr;

  linkedObjInfo.hdr.magic_number = firstKnlHdr.magic_number;
  linkedObjInfo.hdr.major_version = firstKnlHdr.major_version;
  linkedObjInfo.hdr.minor_version = firstKnlHdr.minor_version;
  linkedObjInfo.hdr.num_kernels = knlClosureList.size();
  linkedObjInfo.hdr.num_extern_variables = 0;
  linkedObjInfo.hdr.num_global_variables = 0;
  linkedObjInfo.hdr.num_static_variables = _countLinkedVars;
  linkedObjInfo.hdr.num_filescope_variables = _countLinkedVars;
  linkedObjInfo.hdr.num_extern_functions = 0;
  linkedObjInfo.hdr.num_global_functions = 0;
  linkedObjInfo.hdr.num_static_functions = _countLinkedUnits;
  linkedObjInfo.hdr.num_functions = _countLinkedUnits;
  linkedObjInfo.hdr.kernels = (CompiledUnitInfo *)_mem.alloc(
      sizeof(CompiledUnitInfo) * knlClosureList.size());
  ALLOC_ASSERT(linkedObjInfo.hdr.kernels);
  linkedObjInfo.hdr.filescope_variables = (CompiledVarInfo *)_mem.alloc(
      sizeof(CompiledUnitInfo) * _countLinkedVars);

  if (_countLinkedUnits) {
    linkedObjInfo.hdr.functions = (CompiledUnitInfo *)_mem.alloc(
        sizeof(CompiledUnitInfo) * _countLinkedUnits);
    ALLOC_ASSERT(_countLinkedUnits == 0 || linkedObjInfo.hdr.functions);
  } else {
    linkedObjInfo.hdr.functions = NULL;
  }

  TRY(BuildLinkedVarInfo(linkedObjInfo.hdr));
  CompiledUnitClosureList::iterator iter = knlClosureList.begin();

  for (int i = 0; iter != knlClosureList.end(); ++iter, ++i) {
    CompiledUnitClosure &knlClosure = **iter;
    CompiledUnitInfo &knlInfo = *knlClosure.unit;
    std::string knlName(knlInfo.name, knlInfo.name_len);
    TRY(BuildLinkedUnitInfo(knlName, knlClosure, linkedObjInfo.hdr.kernels[i]));
  }

  for (int i = 0; i < _countLinkedUnits; i++) {
    CompiledUnitClosure &funcClosure = *_linkedUnitIndexToClosureMap[i];
    CompiledUnitInfo &funcInfo = *funcClosure.unit;
    std::string unitName(funcInfo.name, funcInfo.name_len);
    TRY(BuildLinkedUnitInfo(unitName, funcClosure,
                            linkedObjInfo.hdr.functions[i]));
  }

  linkedObjInfo.buf = NULL;
  linkedObjInfo.size = 0;

  return 0;
}

int CISALinker::BuildLinkedUnitInfo(const std::string &unitName,
                                    CompiledUnitClosure &objUnitClosure,
                                    CompiledUnitInfo &linkedUnitInfo) {
  CompiledUnitInfo &objUnit = *objUnitClosure.unit;

  linkedUnitInfo.name_len = unitName.size();
  strncpy(linkedUnitInfo.name, unitName.c_str(), unitName.size());
  linkedUnitInfo.offset = objUnit.offset;
  linkedUnitInfo.size = objUnit.size;
  linkedUnitInfo.input_offset = objUnit.input_offset;
  BuildRelocSymTab(linkedUnitInfo.variable_reloc_symtab,
                   objUnit.variable_reloc_symtab);
  BuildRelocSymTab(linkedUnitInfo.function_reloc_symtab,
                   objUnit.function_reloc_symtab);
  linkedUnitInfo.num_gen_binaries = 0;
  linkedUnitInfo.cisa_binary_buffer =
      (char *)objUnitClosure.unitObjInfo->buf + objUnit.offset;

  return 0;
}

int CISALinker::BuildRelocSymTab(RelocTab &dst, RelocTab &src) {
  dst.num_syms = src.num_syms;

  if (dst.num_syms) {
    dst.reloc_syms = (reloc_sym *)_mem.alloc(sizeof(reloc_sym) * src.num_syms);
    ALLOC_ASSERT(dst.reloc_syms);

    for (int i = 0; i < src.num_syms; i++) {
      dst.reloc_syms[i].symbolic_index = src.reloc_syms[i].symbolic_index;
      dst.reloc_syms[i].resolved_index = src.reloc_syms[i].resolved_index;
    }
  } else {
    dst.reloc_syms = NULL;
  }

  return 0;
}

int CISALinker::BuildLinkedVarInfo(CisaHeader &linkedUnitHdr) {
  for (int i = 0; i < _countLinkedVars; i++) {
    CompiledVarInfo &objVariable = *_linkedVarIndexToClosureMap[i]->var;
    CompiledVarInfo &linkedVariable = linkedUnitHdr.filescope_variables[i];
    linkedVariable.name_len = objVariable.name_len;
    linkedVariable.name = objVariable.name;
    linkedVariable.bit_properties = objVariable.bit_properties;
    linkedVariable.num_elements = objVariable.num_elements;
    linkedVariable.attribute_count = objVariable.attribute_count;
    linkedVariable.attributes = objVariable.attributes;
  }

  return 0;
}

int CISALinker::UpdateLinkedCisaImageOffsetsAndSize() {
  CisaObjInfo &linkedObjInfo = _cisaObjInfos[_numCisaObjInfos];
  CisaHeader &linkedObjHdr = linkedObjInfo.hdr;

  _linkedCisaImageSize = 0;
  _linkedCisaImageSize += sizeof(linkedObjHdr.magic_number);
  _linkedCisaImageSize += sizeof(linkedObjHdr.major_version);
  _linkedCisaImageSize += sizeof(linkedObjHdr.minor_version);
  _linkedCisaImageSize += sizeof(linkedObjHdr.num_kernels);

  for (int i = 0; i < linkedObjHdr.num_kernels; i++) {
    _linkedCisaImageSize += CalculateSize(linkedObjHdr.kernels[i], false);
  }

  _linkedCisaImageSize += sizeof(linkedObjHdr.num_filescope_variables);

  for (int i = 0; i < linkedObjHdr.num_filescope_variables; i++) {
    _linkedCisaImageSize += CalculateSize(linkedObjHdr.filescope_variables[i]);
  }

  _linkedCisaImageSize += sizeof(linkedObjHdr.num_functions);

  for (int i = 0; i < linkedObjHdr.num_functions; i++) {
    _linkedCisaImageSize += CalculateSize(linkedObjHdr.functions[i], true);
  }

  for (int i = 0; i < linkedObjHdr.num_kernels; i++) {
    CompiledUnitInfo &knlUnitInfo = linkedObjHdr.kernels[i];
    unsigned knlUnitDataStartOffset = _linkedCisaImageSize;
    knlUnitInfo.input_offset =
        knlUnitInfo.input_offset - knlUnitInfo.offset + knlUnitDataStartOffset;
    knlUnitInfo.offset = knlUnitDataStartOffset;
    _linkedCisaImageSize += knlUnitInfo.size;
  }

  for (int i = 0; i < linkedObjHdr.num_functions; i++) {
    CompiledUnitInfo &unitInfo = linkedObjHdr.functions[i];
    unsigned unitDataStartOffset = _linkedCisaImageSize;
    unitInfo.offset = unitDataStartOffset;
    _linkedCisaImageSize += unitInfo.size;
  }

  linkedObjInfo.size = _linkedCisaImageSize;

  return 0;
}

int CISALinker::CalculateSize(CompiledUnitInfo &unit, bool isFunction) {
  unsigned size = 0;
  if (isFunction == true) {
    size += sizeof(unit.linkage);
  }

  size += sizeof(unit.name_len) + unit.name_len;
  size += sizeof(unit.offset);
  size += sizeof(unit.size);

  if (isFunction == false) {
    size += sizeof(unit.input_offset);
  }

  size += CalculateSize(unit.variable_reloc_symtab);
  size += CalculateSize(unit.function_reloc_symtab);

  if (isFunction == false) {
    size += sizeof(unit.num_gen_binaries);
  }

  return size;
}

int CISALinker::CalculateSize(CompiledVarInfo &var) {
  unsigned size = 0;
  size += sizeof(var.linkage);
  size += sizeof(var.name_len) + var.name_len;
  size += sizeof(var.bit_properties);
  size += sizeof(var.num_elements);
  size += sizeof(var.attribute_count);

  for (int i = 0; i < var.attribute_count; i++) {
    size += sizeof(var.attributes[i].nameIndex);
    size += sizeof(var.attributes[i].size) + var.attributes[i].size;
  }

  return size;
}

int CISALinker::CalculateSize(RelocTab &symTab) {
  unsigned size = 0;
  size += sizeof(symTab.num_syms);
  size += sizeof(reloc_sym) * symTab.num_syms;

  return size;
}

int CISALinker::AllocateCisaObjInfos(int numCisaObjs) {
  _cisaObjInfos =
      (CisaObjInfo *)_mem.alloc(sizeof(CisaObjInfo) * (numCisaObjs + 1));
  ALLOC_ASSERT(_cisaObjInfos);
  _numCisaObjInfos = numCisaObjs;

  for (int i = 0; i < numCisaObjs + 1; i++) {
    _cisaObjInfos[i].index = i;
  }

  return 0;
}

int CISALinker::ReadCisaMemObj(const CisaObj &cisaObj,
                               CisaObjInfo &cisaObjInfo) {
  unsigned cisaBytePos = 0;

  if (ExtractCisaMemObjHdr(cisaObjInfo.hdr, cisaBytePos, cisaObj.buf)) {
    return 1;
  }

  ASSERT(cisaBytePos <= cisaObj.size, "Corrupted CISA object");
  cisaObjInfo.buf = cisaObj.buf;
  cisaObjInfo.size = cisaObj.size;

  return 0;
}

inline int CISALinker::ExtractCisaMemObjHdr(CisaHeader &cisaHdr,
                                            unsigned &cisaBytePos,
                                            const void *cisaBuffer) {
  TRY(::processCommonISAHeader(cisaHdr, cisaBytePos, cisaBuffer, &_mem));
  ASSERT(cisaHdr.major_version >= 3, "Linking is supported only for CISA 3.0+");

  return 0;
}

int CISALinker::ReadCisaFileObj(const char *cisaFileName,
                                CisaObjInfo &cisaObjInfo) {
  FILE *cisaFile = fopen(cisaFileName, "rb");
  ASSERT(cisaFile != NULL, "Cannot open %s", cisaFileName);
  fseek(cisaFile, 0, SEEK_END);
  unsigned fileSize = ftell(cisaFile);
  char *cisaBuffer = (char *)_mem.alloc(fileSize);
  fseek(cisaFile, 0, SEEK_SET);
  ALLOC_ASSERT(cisaBuffer);
  ASSERT(fread(cisaBuffer, 1, fileSize, cisaFile) != 0,
         "Cannot read CISA object %s", cisaFileName);
  fclose(cisaFile);
  CisaObj cisaObj;
  cisaObj.buf = cisaBuffer;
  cisaObj.size = fileSize;

  return ReadCisaMemObj(cisaObj, cisaObjInfo);
}

int CISALinker::WriteCisaMemObj(CisaObj &cisaObj) {
  TRY(WriteCisaMemObj());
  cisaObj.size = _linkedCisaImageSize;
  cisaObj.buf = (char *)(*_allocator)(_linkedCisaImageSize);
  CisaObjInfo &linkedObjInfo = _cisaObjInfos[_numCisaObjInfos];
  memcpy((void *)cisaObj.buf, linkedObjInfo.buf, cisaObj.size);
  ALLOC_ASSERT(cisaObj.buf);

  return 0;
}

int CISALinker::WriteCisaMemObj() {
  CisaObjInfo &linkedObjInfo = _cisaObjInfos[_numCisaObjInfos];
  CisaHeader &linkedObjHdr = linkedObjInfo.hdr;
  CompiledUnitInfo &knlUnitInfo = linkedObjHdr.kernels[0];

  linkedObjInfo.size = _linkedCisaImageSize;
  linkedObjInfo.buf = (char *)_mem.alloc(_linkedCisaImageSize);
  ALLOC_ASSERT(linkedObjInfo.buf);

  BUF_APPEND_SCALAR(linkedObjHdr.magic_number);
  BUF_APPEND_SCALAR(linkedObjHdr.major_version);
  BUF_APPEND_SCALAR(linkedObjHdr.minor_version);
  BUF_APPEND_SCALAR(linkedObjHdr.num_kernels);

  for (int i = 0; i < linkedObjHdr.num_kernels; i++) {
    WriteCisaCompiledUnitInfo(linkedObjHdr.kernels[i], false);
  }

  BUF_APPEND_SCALAR(linkedObjHdr.num_filescope_variables);

  for (int i = 0; i < linkedObjHdr.num_filescope_variables; i++) {
    WriteCisaVarInfo(linkedObjHdr.filescope_variables[i]);
  }

  BUF_APPEND_SCALAR(linkedObjHdr.num_functions);
  for (int i = 0; i < linkedObjHdr.num_functions; i++) {
    WriteCisaCompiledUnitInfo(linkedObjHdr.functions[i], true);
  }

  for (int i = 0; i < linkedObjHdr.num_kernels; i++) {
    WriteCisaCompiledUnitData(linkedObjHdr.kernels[i]);
  }

  for (int i = 0; i < linkedObjHdr.num_functions; i++) {
    WriteCisaCompiledUnitData(linkedObjHdr.functions[i]);
  }

  INTERNAL_ASSERT(linkedObjInfo.size == _linkedCisaImageSize);

  return 0;
}

int CISALinker::WriteCisaCompiledUnitInfo(CompiledUnitInfo &unit,
                                          bool isFunction) {
  if (isFunction == true) {
    BUF_APPEND_SCALAR(unit.linkage);
  }

  BUF_APPEND_SCALAR(unit.name_len);
  BUF_APPEND_VECTOR(unit.name, unit.name_len);
  BUF_APPEND_SCALAR(unit.offset);
  BUF_APPEND_SCALAR(unit.size);

  if (isFunction == false) {
    BUF_APPEND_SCALAR(unit.input_offset);
  }

  WriteRelocSymTab(unit.variable_reloc_symtab);
  WriteRelocSymTab(unit.function_reloc_symtab);

  if (isFunction == false) {
    unit.num_gen_binaries = 0;
    BUF_APPEND_SCALAR(unit.num_gen_binaries);
  }

  return 0;
}

int CISALinker::WriteCisaVarInfo(CompiledVarInfo &var) {
  BUF_APPEND_SCALAR(var.linkage);
  BUF_APPEND_SCALAR(var.name_len);
  BUF_APPEND_VECTOR(var.name, var.name_len);
  BUF_APPEND_SCALAR(var.bit_properties);
  BUF_APPEND_SCALAR(var.num_elements);
  BUF_APPEND_SCALAR(var.attribute_count);

  for (int i = 0; i < var.attribute_count; i++) {
    BUF_APPEND_SCALAR(var.attributes[i].nameIndex);
    BUF_APPEND_SCALAR(var.attributes[i].size);
    BUF_APPEND_VECTOR(var.attributes[i].value.stringVal,
                      var.attributes[i].size);
  }

  return 0;
}

int CISALinker::WriteRelocSymTab(RelocTab &symTab) {
  BUF_APPEND_SCALAR(symTab.num_syms);

  for (int i = 0; i < symTab.num_syms; i++) {
    BUF_APPEND_SCALAR(symTab.reloc_syms[i].symbolic_index);
    BUF_APPEND_SCALAR(symTab.reloc_syms[i].resolved_index);
  }

  return 0;
}

int CISALinker::WriteCisaCompiledUnitData(CompiledUnitInfo &unitInfo) {
  if (unitInfo.size) {
    BUF_APPEND_VECTOR(unitInfo.cisa_binary_buffer, unitInfo.size);
  }

  return 0;
}

int CISALinker::WriteCisaFileObj(const char *cisaFileName) {
  WriteCisaMemObj();
  FILE *cisaFile = fopen(cisaFileName, "wb");
  ASSERT(cisaFile != NULL, "Cannot open %s for write", cisaFileName);
  CisaObjInfo &cisaObjInfo = _cisaObjInfos[_numCisaObjInfos];
  ASSERT(fwrite(cisaObjInfo.buf, 1, cisaObjInfo.size, cisaFile) ==
             cisaObjInfo.size,
         "Error in writing linked CISA object");
  fclose(cisaFile);

  return 0;
}

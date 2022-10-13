/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __CISALINKER_H__
#define __CISALINKER_H__

#include <list>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Common_ISA.h"
#include "../Mem_Manager.h"
#include "../include/visa_igc_common_header.h"

class CISALinker {
public:
  // *** Public types ***

  struct CisaObj {
    const void *buf;
    unsigned size;
  };
  typedef void *ExternalHeapAllocator(unsigned size);

  // *** Public functions ***

  CISALinker(int numOptions, const char *options[], vISA::Mem_Manager &mem,
             ExternalHeapAllocator *externalAllocator = NULL);
  ~CISALinker();

  int LinkCisaMemObjs(const char *kernelName, int numCisaObjs,
                      const CisaObj cisaObjs[], CisaObj &cisaLinkedObj);
  int LinkCisaFileObjs(const char *kernelName, int numCisaObjs,
                       const char *cisaObjs[], const char *cisaLinkedObj);

private:
  // *** Private types ***

  typedef reloc_symtab RelocTab;
  typedef common_isa_header CisaHeader;
  struct CisaObjInfo {
    CisaHeader hdr;
    int index;
    const void *buf;
    unsigned size;
  };
  typedef compiled_unit_info_t CompiledUnitInfo;
  struct CompiledUnitClosure {
    CompiledUnitInfo *unit;
    CisaObjInfo *unitObjInfo;
    int linkedUnitIndex;
  };
  typedef filescope_var_info_t CompiledVarInfo;
  typedef std::vector<CompiledUnitClosure *> UnitToGlobalUnitClosuresMap;
  typedef std::unordered_map<std::string, CompiledUnitInfo *>
      GlobalUnitNameToInfoMap;
  typedef std::vector<CompiledUnitClosure *> LinkedUnitIndexToClosureMap;
  typedef std::list<CompiledUnitClosure *> CompiledUnitClosureList;
  struct CompiledVarClosure {
    CompiledVarInfo *var;
    CisaObjInfo *varObjInfo;
    int linkedVarIndex;
  };
  typedef std::unordered_map<std::string, CompiledVarInfo *>
      GlobalVarNameToInfoMap;
  typedef std::vector<CompiledVarClosure *> LinkedVarIndexToClosureMap;

  // *** Private functions ***

  int InitGlobalInfoTables();
  void ClearGlobalInfoTables();
  int GetGlobalUnitInfo(const std::string &unitName,
                        CompiledUnitInfo *&globalUnitInfo);
  int GetGlobalUnitClosure(const std::string &unitName,
                           CompiledUnitClosure *&globalUnitClosure);
  int GetGlobalVarInfo(const std::string &unitName,
                       CompiledVarInfo *&globalUnitInfo);
  int GetGlobalVarClosure(const std::string &unitName,
                          CompiledVarClosure *&globalVarClosure);
  int CreateKernelClosures(const std::string &knlName,
                           CompiledUnitClosureList &knlClosureList);
  int LinkCisaObjInfos(const std::string &knlName);
  int LinkLocalCisaObjInfos(CisaObjInfo &cisaObjInfo,
                            CompiledUnitClosureList &localUnitClosureList);
  int LinkExternCisaObjInfos();
  int LinkInDepUnits(CompiledUnitClosure &cisaUnitClosure);
  int LinkInDepVars(CompiledUnitClosure &cisaUnitClosure);
  int RelocFunctionSyms(CompiledUnitClosure &localUnitClosure);
  int RelocVariableSyms(CompiledUnitClosure &localUnitClosure);
  int BuildLinkedCisaObj(CompiledUnitClosureList &cisaKnlClosureList);
  int BuildLinkedUnitInfo(const std::string &unitName,
                          CompiledUnitClosure &objUnitClosure,
                          CompiledUnitInfo &linkedUnitInfo);
  int BuildLinkedVarInfo(CisaHeader &linkedUnitHdr);
  int BuildRelocSymTab(RelocTab &dst, RelocTab &src);
  int UpdateLinkedCisaImageOffsetsAndSize();
  int CalculateSize(CompiledUnitInfo &unit, bool isFunction);
  int CalculateSize(CompiledVarInfo &var);
  int CalculateSize(RelocTab &symTab);
  int AllocateCisaObjInfos(int numCisaObjs);
  int ReadCisaFileObj(const char *cisaFileName, CisaObjInfo &cisaObjInfo);
  int WriteCisaFileObj(const char *cisaFileName);
  int ReadCisaMemObj(const CisaObj &cisaObj, CisaObjInfo &cisaObjInfo);
  int WriteCisaMemObj(CisaObj &cisaObj);
  int WriteCisaMemObj();
  int WriteCisaCompiledUnitInfo(CompiledUnitInfo &knlUnitInfo, bool isFunction);
  int WriteCisaVarInfo(CompiledVarInfo &var);
  int WriteRelocSymTab(RelocTab &symTab);
  int WriteCisaCompiledUnitData(CompiledUnitInfo &unitInfo);
  int ExtractCisaMemObjHdr(CisaHeader &cisaHdr, unsigned &cisaBytePos,
                           const void *cisaBuffer);

  // *** Private data ***

  vISA::Mem_Manager &_mem;
  ExternalHeapAllocator *_allocator;
  int _numOptions;
  const char **_options;
  GlobalUnitNameToInfoMap _globalUnitNameToInfoMap;
  CompiledUnitInfo *_localUnitMap;
  LinkedUnitIndexToClosureMap _linkedUnitIndexToClosureMap;

  GlobalVarNameToInfoMap _globalVarNameToInfoMap;
  CompiledVarInfo *_localVarMap;
  LinkedVarIndexToClosureMap _linkedVarIndexToClosureMap;

  CompiledUnitClosureList _pendingLocalClosures;
  CompiledUnitClosureList _pendingExternClosures;

  int _countLinkedUnits;
  int _countLinkedVars;
  int _numCisaObjInfos;
  CisaObjInfo *_cisaObjInfos;
  unsigned _linkedCisaImageSize;
  unsigned _cisaEmitBufOffset;
};

#endif // __CISALINKER_H__

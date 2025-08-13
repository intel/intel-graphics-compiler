/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "CLElfTypes.h"

#if defined(_WIN32)
#define ELF_CALL __stdcall
#else
#define ELF_CALL
#endif

namespace CLElfLib {
/******************************************************************************\

 Class:         CElfReader

 Description:   Class to provide simpler interaction with the ELF standard
                binary object.  SElf64Header defines the ELF header type and
                SElf64SectionHeader defines the section header type.

\******************************************************************************/
class CElfReader {
public:
  static CElfReader *ELF_CALL Create(const char *pElfBinary, const size_t elfBinarySize);

  static void ELF_CALL Delete(CElfReader *&pElfObject);

  static bool ELF_CALL IsValidElf(const void *pBinary, const size_t binarySize);

  const SElfHeader *ELF_CALL GetElfHeader();

  const SElfSectionHeader *ELF_CALL GetSectionHeader(unsigned int sectionIndex);

  const SElfSectionHeader *ELF_CALL GetSectionHeader(const char *sectionName);

  const char *ELF_CALL GetSectionName(unsigned int sectionIndex);

  E_RETVAL ELF_CALL GetSectionData(const unsigned int sectionIndex, char *&pData, size_t &dataSize);

  E_RETVAL ELF_CALL GetSectionData(const char *sectionName, char *&pData, size_t &dataSize);

protected:
  ELF_CALL CElfReader(const char *pElfBinary, const size_t elfBinarySize);

  ELF_CALL ~CElfReader();

  CElfReader(const CElfReader &) = delete;
  CElfReader &operator=(const CElfReader &) = delete;

  SElfHeader *m_pElfHeader; // pointer to the ELF header
  const char *m_pBinary;    // portable ELF binary
  char *m_pNameTable;       // pointer to the string table
  size_t m_nameTableSize;   // size of string table in bytes
};

/******************************************************************************\

 Class:         CElfReaderDeleter

 Description:   Dummy class to be used with unique_ptr to call Delete on
                CElfReader when going out of scope.

\******************************************************************************/
class CElfReaderDeleter {
public:
  void operator()(CElfReader *ptr) const { CElfReader::Delete(ptr); }
};

/******************************************************************************\

 Class:         RAIIElf

 Description:   Dummy class to call the Cleanup() method on CElfReader when
                going out of scope.

\******************************************************************************/
class RAIIElf {
public:
  explicit RAIIElf(CElfReader *pElf);
  ~RAIIElf();
  RAIIElf(const RAIIElf &) = delete;
  RAIIElf &operator=(const RAIIElf &) = delete;

private:
  CElfReader *p;
};

} // namespace CLElfLib

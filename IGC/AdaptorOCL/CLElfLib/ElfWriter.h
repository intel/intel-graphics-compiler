/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#pragma once
#include "CLElfTypes.h"
#include <queue>
#include <string>

#if defined(_WIN32) && (__KLOCWORK__ == 0)
  #define ELF_CALL __stdcall
#else
  #define ELF_CALL
#endif

using namespace std;

namespace CLElfLib
{
static const unsigned int g_scElfHeaderAlignment    = 16;   // allocation alignment restriction
static const unsigned int g_scInitialElfSize        = 2048; // initial elf size (in bytes)
static const unsigned int g_scInitNumSectionHeaders = 8;

struct SSectionNode
{
    E_SH_TYPE    Type;
    unsigned int Flags;
    string Name;
    char* pData;
    unsigned int DataSize;

    SSectionNode()
    {
        Type     = SH_TYPE_NULL;
        Flags    = 0;
        pData    = NULL;
        DataSize = 0;
    }

    ~SSectionNode()
    {
    }
};

/******************************************************************************\

 Class:         CElfWriter

 Description:   Class to provide simpler interaction with the ELF standard
                binary object.  SElf64Header defines the ELF header type and
                SElf64SectionHeader defines the section header type.

\******************************************************************************/
class CElfWriter
{
public:
    static CElfWriter* ELF_CALL Create(
        E_EH_TYPE type,
        E_EH_MACHINE machine,
        Elf64_Xword flags );

    static void ELF_CALL Delete( CElfWriter* &pElfWriter );

    E_RETVAL ELF_CALL AddSection(
        SSectionNode* pSectionNode );

    E_RETVAL ELF_CALL ResolveBinary(
        char* const pBinary,
        size_t& dataSize );

    E_RETVAL ELF_CALL Initialize();
    E_RETVAL ELF_CALL PatchElfHeader( char* const pBinary );

protected:
    ELF_CALL CElfWriter(
        E_EH_TYPE type,
        E_EH_MACHINE machine,
        Elf64_Xword flags );

    ELF_CALL ~CElfWriter();

    E_EH_TYPE m_type;
    E_EH_MACHINE m_machine;
    Elf64_Xword m_flags;

    std::queue<SSectionNode*> m_nodeQueue;

    unsigned int m_dataSize;
    unsigned int m_numSections;
    size_t       m_stringTableSize;
    size_t       m_totalBinarySize;
};

/******************************************************************************\

 Class:         CElfWriterDeleter

 Description:   Dummy class to be used with unique_ptr to call Delete on
                CElfReader when going out of scope.

\******************************************************************************/
class CElfWriterDeleter
{
public:
  void operator()(CElfWriter* ptr) const
  {
    CElfWriter::Delete(ptr);
  }
};

} // namespace ELFLib

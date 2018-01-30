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

// CM FC patch info
//

#pragma once

#ifndef __CM_FC_PATCHINFO_H__
#define __CM_FC_PATCHINFO_H__

#include <cstddef>
#include <cstdint>

namespace cm {
namespace patch {

typedef uint32_t PInfo_Offset;  // File offset
typedef uint32_t PInfo_Addr;    // Virtual address.
typedef uint32_t PInfo_U32;
typedef uint16_t PInfo_U16;

const PInfo_U32 MAGIC = 0x49504D43U;   // 'I', 'P', 'M', 'C', i.e. 'CMPI'

/// Version of patch info.
enum {
  PV_0    = 0,  ///< Revision 0.
};

/// Platform of patch info.
enum {
  PP_NONE   = 0,  ///< Placeholder for invalid platform
  PP_SNB    = 1,  ///< Sandybridge
  PP_IVB    = 2,  ///< Ivybridge
  PP_HSW    = 3,  ///< Haswell
  PP_BDW    = 4,  ///< Broadwell
  PP_CHV    = 5,  ///< CherryView
  PP_SKL    = 6,  ///< Skylake
  PP_BXT    = 7,  ///< Broxton
};

/// Patch info header.
struct PInfoHdr {
  PInfo_U32     Magic;      ///< Magic word.
  PInfo_U16     Version;    ///< Patch info version.
  PInfo_U16     Platform;   ///< Platform, e.g. BDW, SKL, and etc.
  PInfo_U16     ShNum;      ///< Number of entries of section header.
  PInfo_U16     PgNum;      ///< Number of entries of program header.
  PInfo_Offset  ShOffset;   ///< File offset to section header.
  PInfo_Offset  PgOffset;   ///< File offset to program header.

  bool checkMagic() const {
    return Magic == MAGIC;
  }

  bool isValidPlatform() const {
    switch (Platform) {
    case PP_SNB:
    case PP_IVB:
    case PP_HSW:
    case PP_BDW:
    case PP_CHV:
    case PP_SKL:
    case PP_BXT:
      return true;
    default: break;
    }
    return false;
  }
};

/// Patch info section header.
struct PInfoSectionHdr {
  PInfo_U16     ShType;     ///< Section type.
  PInfo_U16     ShLink;     ///< Link to section referenced by this section,
                            ///  e.g., relocation links to its symbol table,
                            //   symbol table links to its string table.
  PInfo_U16     ShLink2;    ///< Another link to section referenced by this
                            ///  section, e.g., symbol table links to its
                            //   binary table, register access table links to
                            //   its binary table, token table links to is
                            //   binary table.
  PInfo_U16     ShPadding;
  PInfo_Offset  ShOffset;   ///< File offset to the section data.
  PInfo_U32     ShSize;     ///< Size of section in bytes.
};

/// Patch info section type.
enum {
  PSHT_NONE     = 0,    ///< Invalid section type.
  PSHT_BINARY   = 1,    ///< Binary machine code.
  PSHT_REL      = 2,    ///< Relocation entries.
  PSHT_SYMTAB   = 3,    ///< Symbol table.
  PSHT_STRTAB   = 4,    ///< String table.
};

/// Entry of symbol table.
struct PInfoSymbol {
  PInfo_U32   SymName;  ///< Symbol name, index to string table.
  PInfo_Addr  SymValue; ///< Symbol value, the relative address from the
                        ///  section.
  PInfo_U16   SymShndx; ///< Which section it's defined.
  PInfo_U16   SymExtra; ///< Extra information.
};

enum {
  PSHN_UNDEF = 0, ///< Undefined section.
};

/// Entry of relocation table.
struct PInfoRelocation {
  PInfo_Addr  RelAddr;  ///< Location.
  PInfo_U32   RelSym;   ///< Symbol table index.
};


} // End patch namespace
} // End cm namespace

#endif // __CM_FC_PATCHINFO_H__

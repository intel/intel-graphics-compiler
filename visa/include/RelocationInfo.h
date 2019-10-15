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
/*  ---------------------------------------------------------------------------
**
**  File Name     : RelocationInfo.h
**
**  Abastract     : This file contains the definition of Relocation Table and
**                  Symbol Table which are shared between Compiler and Driver
**  -------------------------------------------------------------------------- */
#ifndef RELOCATION_INFO_H
#define RELOCATION_INFO_H

namespace vISA {

static const uint32_t MAX_SYMBOL_NAME_LENGTH = 256;

/// GenSymType - Specify the symbol's type
enum GenSymType {
    S_NOTYPE           = 0,    // The symbol's type is not specified
    S_UNDEF            = 1,    // The symbol is undefined in this module
    S_FUNC             = 2,    // The symbol is associated with a function
    S_GLOBAL_VAR       = 3,    // The symbol is associated with a global variable in global address space
    S_GLOBAL_VAR_CONST = 4,    // The symbol is associated with a global variable in constant address space
    S_CONST_SAMPLER   = 5      // The symbol is associated with a constant sampler
};

/// GenSymEntry - An symbol table entry
typedef struct {
    uint32_t   s_type;            // The symbol's type
    uint32_t   s_offset;          // The binary offset of this symbol. This field is ignored if s_type is S_UNDEF
    uint32_t   s_size;
    char       s_name[MAX_SYMBOL_NAME_LENGTH]; // The symbol's name
} GenSymEntry;

/// GenRelocType - Specify the relocation's type
enum GenRelocType {
    R_NONE     = 0,
    R_SYM_ADDR = 1,      //64-bit type
    R_SYM_ADDR_32 = 2,   //lower 32-bit of 64-bit address.
    R_SYM_ADDR_32_HI = 3 //higher 32bits of 64-bit address
};

/// GenRelocEntry - An relocation table entry
typedef struct {
    uint32_t   r_type;        // The relocation's type
    uint32_t   r_offset;      // The binary offset of the relocated target
    char       r_symbol[MAX_SYMBOL_NAME_LENGTH]; // The relocation target symbol's name
} GenRelocEntry;

}

#endif

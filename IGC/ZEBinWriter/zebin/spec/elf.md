<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# ELF Layout
The version information of ZEBIN can be found in version.md. ELF changes should
be documented properly in version.md.

| Contents | Description | Section Type |
| ------ | ------ |  ------ |
| ELF header | Standard ELF header contains version and platform information ||
| Section headers | Standard ELF section headers ||
| .text.{*kernel_name*} | ISA binary of kernel in this compiled module | SHT_PROGBITS |
| .text | ISA binary of this compiled module that is not included in above .text.* sections. E.g. external functions | SHT_PROGBITS |
| .data.const | Constant data section (if any) | SHT_PROGBITS |
| .bss.const | Constant data with zero-initialized variables (if any) | SHT_NOBITS |
| .data.global | Global data section (if any) | SHT_PROGBITS |
| .bss.global | Global data with zero-initialized variables (if any) | SHT_NOBITS |
| .symtab | Symbol table (if any) | SHT_SYMTAB |
| .rel.{*kernel_name*} | Relocation table (if any) | SHT_REL |
| .spv | Spir-v of the module (if required) | SHT_ZEBIN_SPIRV |
| .visaasm.{*visa_module_name*} | vISA asm of the module (if required) | SHT_ZEBIN_VISAASM |
| .debug_* | the debug information (if required) | SHT_PROGBITS |
| .ze_info | the metadata section for runtime information | SHT_ZEBIN_ZEINFO |
| .gtpin_info.{*kernel_name*\|*function_name*} | the metadata section for gtpin information (if any) | SHT_ZEBIN_GTPIN_INFO |
| .misc.{*misc_name*} | the miscellaneous data for multiple purposes. For example, the section _.misc.buildOptions_ contains the build options used for compiling this binary.  | SHT_ZEBIN_MISC |
| .note.intelgt.compat | the compatibility notes for runtime information | SHT_NOTE |
| .strtab | the string table for section/symbol names | SHT_STRTAB |

An ZE binary contains information of one compiled module. A compiled module
could contain more than one kernel, each kernel binary represented in a text
section. *kernel_name* is the name of the kernel, the same as represented in ZE
Info **functions** attributes' **name**. *function_name* is the name of
a function.

## ELF Header Values

**e_ident**
~~~
EI_MAG0:    0x7f
EI_MAG1:    'E'
EI_MAG2:    'L'
EI_MAG3:    'F'
EI_CLASS:   ELFCLASS32/ELFCLASS64 (for 32/64 objects)
EI_DATA:    ELFDATA2LSB (2's complement, little endian)
EI_VERSION: 1 (EV_CURRENT)
EI_OSABI:   0 (ELFOSABI_NONE)
EI_ABIVERSION: (See Below)
EI_PAD:     0 (Start of padding bytes of e_ident)

Value of `EI_ABIVERSION` is 1.
~~~

**e_machine**

~~~
// ELF machine architecture
enum {
    EM_INTELGT = 205,
};
~~~

**e_version**
~~~
1(EV_CURRENT)
~~~

**e_flags**

~~~
0(TBD)
~~~

All others fields in ELF header follow what are defined in the standard.

## ZE Info Sections

**sh_type**
~~~
enum SHT_ZEBIN : uint32_t
{
    SHT_ZEBIN_SPIRV      = 0xff000009, // .spv.kernel section, value the same as SHT_OPENCL_SPIRV
    SHT_ZEBIN_ZEINFO     = 0xff000011, // .ze_info section
    SHT_ZEBIN_GTPIN_INFO = 0xff000012, // .gtpin_info section
    SHT_ZEBIN_VISAASM    = 0xff000013, // .visaasm section
    SHT_ZEBIN_MISC       = 0xff000014  // .misc section
}
~~~

**sh_link and and sh_info Interpretation**

Two members in the section header, sh_link and sh_info, hold special
information, depending on section type.

| sh_type | sh_link | sh_info |
| ------ | ------ |  ------ |
| SHT_ZEBIN_GTPIN_INFO | 0 | The symbol table index to the corresponding kernel/function symbol |

## ELF note type for INTELGT

**n_type**
Currently there are 8 note types defined for INTELGT and the notes are placed
in the .note.intelgt.compat section. The consumer of the ZE binary file should
recognize both the owner name (INTELGT) and the type of an ELF note entry to
interpret its description.
~~~
enum {
    // The description is the Product family stored in a 4-byte ELF word
    NT_INTELGT_PRODUCT_FAMILY = 1,
    // The description is the GFXCORE family stored in a 4-byte ELF word
    NT_INTELGT_GFXCORE_FAMILY = 2,
    // The description is the TargetMetadata structure defined below
    NT_INTELGT_TARGET_METADATA = 3,
    // The description represents the ZEBIN ELF file version that reflects the
    // attribute and section changes. The content is stored in a nul-terminated
    // string and the format is "<Major number>.<Minor number>".
    NT_INTELGT_ZEBIN_VERSION = 4,
    // The description represents VISA ABI version used in generated code.
    // Note that VISA ABI is valid only when stack calls are used. Without
    // stack calls, VISA ABI field may be absent.
    NT_INTELGT_VISA_ABI_VERSION = 5,
    // The description is stored in a 4-byte ELF word and is used instead
    // of NT_INTELGT_PRODUCT_FAMILY, NT_INTELGT_GFXCORE_FAMILY and
    // NT_INTELGT_TARGET_METADATA because it contains all required info to
    // validate program for a target device
    NT_INTELGT_PRODUCT_CONFIG = 6,
    // The description is the version of Indirect Access Detection implementation
    // stored in a 4-byte ELF word.
    NT_INTELGT_INDIRECT_ACCESS_DETECTION_VERSION = 7,
    // The descritpion is the major version of Indirect Access Buffer layout
    // stored in a 4-byte ELF word.
    NT_INTELGT_INDIRECT_ACCESS_BUFFER_MAJOR_VERSION = 8,
};
~~~

**The description of NT_INTELGT_TARGET_METADATA note**
~~~
struct TargetMetadata {
    // bit[7:0]: dedicated for specific generator (meaning based on generatorId)
    enum GeneratorSpecificFlags : uint8_t {
        NONE = 0
    };
    // bit[23:21]: generator of this device binary
    enum GeneratorId : uint8_t {
        UNREGISTERED = 0,
        IGC          = 1,
        NGEN         = 2
    };

    union {
        struct {
            // bit[7:0]: dedicated for specific generator (meaning based on generatorId)
            GeneratorSpecificFlags generatorSpecificFlags : 8;
            // bit[12:8]: values [0-31], min compatbile device revision Id (stepping)
            uint8_t minHwRevisionId : 5;
            // bit[13:13]:
            // 0 - full validation during decoding (safer decoding)
            // 1 - no validation (faster decoding - recommended for known generators)
            bool validateRevisionId : 1;
            // bit[14:14]:
            // 0 - ignore minHwRevisionId and maxHwRevisionId
            // 1 - underlying device must match specified revisionId info
            bool disableExtendedValidation : 1;
            // bit[15:15]: reserved bit for future use
            bool reservedBit : 1;
            // bit[20:16]:  max compatible device revision Id (stepping)
            uint8_t maxHwRevisionId : 5;
            // bit[23:21]: generator of this device binary. Value defined in above GeneratorId
            uint8_t generatorId : 3;
            // bit[31:24]: MBZ, reserved for future use
            uint8_t reserved : 8;
        };
        uint32_t packed = 0U;
    };
};
~~~

**The description of NT_INTELGT_PRODUCT_CONFIG note**
~~~
typedef struct GFX_GMD_ID_DEF
{
    union
    {
        struct
        {
            unsigned int    RevisionID : 6;
            unsigned int    Reserved : 8;
            unsigned int    GMDRelease : 8;
            unsigned int    GMDArch : 10;
        }GmdID;
        unsigned int    Value;
    };
}GFX_GMD_ID;
~~~

**The description of NT_INTELGT_INDIRECT_ACCESS_DETECTION_VERSION note**
~~~
An indirect access happens when GPU loads from an address that was not
directly given as one of the kernel arguments. It's usually a pointer
loaded from memory pointed by a kernel argument. IGC has an analysis
that is able to detect whether any indirect access is used in a kernel.
If there are no indirect accesses, then IGC passes this information to
Neo through ZEBin. Based on that information Neo can disable indirect
accesses which results in performance improvements.

In case IGC has any bugs in Indirect Access Detection mechanism, it may
happen that not all indirect accesses get detected in a kernel, therefore
falsely reporting to Neo that indirect accesses can be disabled, resulting
in functional issues.

NT_INTELGT_INDIRECT_ACCESS_DETECTION_VERSION is a versioning mechanism for
Indirect Access Detection mechanism in IGC. It should be bumped up every
time a new functional issue is implemented. The note indicates with what
version of Indirect Access Detection mechanism, a kernel was compiled.
If Neo knows that the newest version of the detection mechanism is 2,
then it can trigger recompilation of all AOT-compiled kernels with version
lower than 2.
~~~

## Gen Relocation Type
Relocation type for **ELF32_R_TYPE** or **ELF64_R_TYPE**
~~~
enum GenRelocType {
    R_NONE                         = 0,
    R_SYM_ADDR                     = 1, // 64-bit address
    R_SYM_ADDR_32                  = 2, // 32-bit address or lower 32-bit of a 64-bit address
    R_SYM_ADDR_32_HI               = 3, // higher 32bits of a 64-bit address
    R_PER_THREAD_PAYLOAD_OFFSET_32 = 4, // *** Deprecated ***
    R_GLOBAL_IMM_32                = 5, // 32-bit global immediate
    R_SEND                         = 6, // send instruction offset, used for BTI patching
    R_SYM_ADDR_16                  = 7  // 16-bit address or immediate
};
~~~

## ZE symbols
In ZEBinary file for each kernel there is a corresponding ELF symbol with the
same name as the kernel emitted in the .symtab. The kernel symbol is local and
points to offset 0 of the section. Currently for each kernel a local symbol
"_entry" is emitted to represent the actual kernel start, and the symbol type is
STT_NOTYPE and the size is 0. A kernel may start with some prolog code and it
is useful to know the actual kernel start offset in some cases.

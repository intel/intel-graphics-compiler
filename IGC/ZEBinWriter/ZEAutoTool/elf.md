# ELF Layout

| Contents | Description | Section Type |
| ------ | ------ |  ------ |
| ELF header | Standard ELF header contains version and platform information ||
| Section headers | Standard ELF section headers ||
| .text.{*kernel_name*} | Gen binary of kernel/functions in this compiled module | SHT_PROGBITS |
| .data.const | Constant data section (if any) | SHT_PROGBITS |
| .data.global | Global data section (if any) | SHT_PROGBITS |
| .symtab | Symbol table (if any) | SHT_SYMTAB |
| .rel.{*kernel_name*} | Relocation table (if any) | SHT_REL |
| .spv | Spir-v of the module (if required) | SHT_ZEBIN_SPIRV |
| .debug_info | the debug information (if required) | SHT_PROGBITS |
| .ze_info | the metadata section | SHT_ZEBIN_ZEINFO |
| .strtab | the string table for section/symbol names | SHT_STRTAB |

An ZE binary contains information of one compiled module.
A compiled module could contain more than one kernel, each kernel binary represented in a text section.
*kernel_name* is the name of the kernel, the same as represented in ZE Info **functions** attributes' **name**

## ELF Header Values

**e_ident**
~~~
EI_MAG0:    0x7f
EI_MAG1:    'E'
EI_MAG2:    'L'
EI_MAG3:    'F'
EI_CLASS:   ELFCLASS32/ELFCLASS64 (for 32/64 objects)
EI_DATA:    ELFDATA2LSB (2's complement, little endian)
EI_VERSION: 0 (**TBD**)
EI_PAD:     0 (Start of padding bytes of e_ident)
~~~


**e_type**

ET_ZEBIN_REL and ET_ZEBIN_DYN are used for bianry-level static linking.
We only supports ET_ZEBIN_EXE for now that all compiler produced binaries are executables
~~~
enum ELF_TYPE_ZEBIN : uint16_t
{
    ET_ZEBIN_REL = 0xff11,     // A relocatable ZE binary file
    ET_ZEBIN_EXE = 0xff12,     // An executable ZE binary file
    ET_ZEBIN_DYN = 0xff13,     // A shared object ZE binary file
};
~~~


**e_machine**

PRODUCT_FAMILY or GFXCORE_FAMILY, depends on **e_flags** (TargetFlags::machineEntryUsesGfxCoreInsteadOfProductFamily)

**Question**:Do we need to specify the enum value of PRODUCT_FAMILY and GFXCORE_FAMILY(already have) and make it part of the spec?


**e_version**
~~~
0(TBD)
~~~


**e_flags**
~~~
struct TargetFlags {
    union {
        struct{
            // bit[7:0]: dedicated for specific generator (meaning based on generatorId)
            uint8_t generatorSpecificFlags : 8;

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

            // bit[15:15]:
            // 0 - elfFileHeader::machine is PRODUCT_FAMILY
            // 1 - elfFileHeader::machine is GFXCORE_FAMILY
            bool machineEntryUsesGfxCoreInsteadOfProductFamily : 1;

            // bit[20:16]:  max compatbile device revision Id (stepping)
            uint8_t maxHwRevisionId : 5;

            // bit[23:21]: generator of this device binary
            // 0 - Unregistered
            // 1 - IGC
            uint8_t generatorId : 3;

            // bit[31:24]: MBZ, reserved for future use
            uint8_t reserved : 8;
        };
        uint32_t packed = 0U;
    };
};
~~~

All others fields in ELF header follow what are defined in the standard.

## ZE Info Section Type

**sh_type**
~~~
enum SHT_ZEBIN : uint32_t
{
    SHT_ZEBIN_SPIRV  = 0xff000009, // .spv.kernel section, value the same as SHT_OPENCL_SPIRV
    SHT_ZEBIN_ZEINFO = 0xff000011  // .ze_info section
}
~~~

## Gen Relocation Type
Relocation type for **ELF32_R_TYPE** or **ELF64_R_TYPE**
~~~
enum GenRelocType {
    R_ZE_NONE           = 0,
    R_ZE_SYM_ADDR       = 1, // 64-bit address
    R_ZE_SYM_ADDR_32    = 2, // 32-bit address or lower 32-bit of a 64-bit address
    R_ZE_SYM_ADDR_32_HI = 3  // higher 32bits of a 64-bit address
};
~~~

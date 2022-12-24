<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# The Virtual ISA Object Format

A virtual ISA object file consists of a stream of bytes, and the binary
format is presented in pseudo-structures similar to Java class file
format. Items in a structure are declared using the fundamental data
types (UW, UD, etc.). Each item in a structure takes at least one byte
of storage. If an item is a bit-field, its content is stored from the
LSB (bit 0) to the MSB. Multiple bit-field items may be packed into a
single byte, and the unused bits in a byte must be set to zero. If an
item is a byte sequence (e.g., a double word), its content is stored in
little-endian format starting from the least significant byte to the
most significant byte. Successive items in a structure are stored
sequentially without padding or alignment. The virtual ISA file contains
several variable-sized tables whose size is specified in another
number-of-elements item in the same structure (usually immediately
preceding the table). The table may itself contain other variable-sized
tables.

In addition to the binary format, a textual specification of the virtual
ISA format is also available. When in text format, a virtual ISA
assembly file is in ASCII text. Lines are separated by the newline
character ('n'). Whitespace is used to separate tokens in the assembly
file. Operands and variables in the text format description are
surrounded by angular brackets '&lt;&gt;', which are not part of the
assembly. Square brackets '\[\]' are used to signify that the field is
optional. Vertical bar '|' is used to represent that only one of the
items separated by it will be candidate. A number of pre-defined
directives may be used to declare variables and specify properties about
the kernel.

**Pre-defined Directives**

| Directive    | Meaning                            |
| --- | --- |
| .kernel_attr | Attributes (global)                |
| .decl        | Variable declaration               |
| .input       | Input declaration                  |
| .version     | Version number                     |
| .kernel      | Name of the kernel                 |
| .function    | Name of the function               |

## Virtual ISA Header

The virtual ISA file header has the following format:


### common_isa_header
  common_isa_header {
    UD magic_number;
    UB major_version;
    UB minor_version;
    UW num_kernels;
    kernel_info kernel_info[num_kernels];
    UW num_variables;
    file_scope_var_info file_scope_var_info[num_variables];
    UW num_functions;
    function_info function_info[num_functions];
  }

- **magic_number:** A magic number used to identify a virtual ISA binary file.  This number is equal to 0x41534943 ('CISA' in hex).
- **major_version:** Version number for the virtual ISA binary file.  For backward-compatibility, the vISA finalizer should accept any virtual ISA file whose version is less than its declared version.
- **minor_version:** Version number for the virtual ISA binary file.  For backward-compatibility, the vISA finalizer should accept any virtual ISA file whose version is less than its declared version.
- **num_kernels:** Number of kernels included in the virtual ISA file.  The maximum number of kernels allowed in a virtual ISA file is 512.
- **kernel_info:** Metadata information of a vISA kernel.
- **num_variables:** Number of file scope variable declarations included in the virtual ISA object.
- **file_scope_var_info:** Metadata information of a vISA variable. See file_scope_var_info for details.
- **num_functions:** Number of functions included in the virtual ISA.
- **function_info:** Metadata information of a vISA function. See function_info for details.

Each kernel in the table has the format:


### kernel_info
  kernel_info {
    UW name_len;
    UB name[name_len];
    UD offset;
    UD size;
    UD input_offset;
    UW num_syms_variable;
    variable_reloc_symtab variable_reloc_symtab[num_syms_variable];
    UW num_syms_function;
    function_reloc_symtab function_reloc_symtab[num_syms_function];
    UB num_gen_binaries;
    gen_binary_info gen_binary_info[num_gen_binaries];
  }

- **name_len:** The length of the name of the kernel, must be between 1 to 65535 characters.
- **name:** The name of the kernel.  It is not null-terminated.
- **offset:** The byte offset of the kernel, from the beginning of the file.  The format of the kernel object is described in Section 4.5.8.
- **size:** The size of the kernel object in bytes.  This does not include the optional GEN binary of the kernel.
- **input_offset:** The byte offset to the num_inputs field of the kernel object (see Section 4.5.8), from the beginning of the file.  This field is used by the virtual ISA runtime to quickly access the input symbol table and set up thread payload.
- **num_syms_variable:** The number of file-scope variables refered to in this kernel.
- **variable_reloc_symtab:** The relocation maps for each file scope general variable symbolic reference. The format is described by reloc_sym
- **num_syms_function:** The number of file-scope functions refered to in this kernel.
- **function_reloc_symtab:** the relocation information (maps) for each function symbolic reference in this vISA object. The format is described by reloc_sym.
- **num_gen_binaries:** Virtual ISA provides optional fat binary support by allowing a kernel's GEN binary to be directly embedded in its virtual ISA file.  Valid values are 0-4.
- **gen_binary_info:** The format is described by gen_binary_info.

The format of the relocation information for a symbol is described by:


### variable_reloc_symtab
  variable_reloc_symtab {
    UW symbolic_index;
    UW resolved_index;
  }

- **symbolic_index:** The original index used by a kernel to refer to another function or file-scope variable.
- **resolved_index:** The resolved index into either the variables or the functions table.


The format of the Gen Binary described by:


### gen_binary_info
  gen_binary_info {
    UB gen_platform;
    UD binary_offset;
    UD binary_size;
  }

- **gen_platform:** GEN platform for this binary. Valid values are:

  - 3:  BDW
  - 5:  SKL
  - 6:  BXT
  - 10:  ICLLP
  - 12:  TGLLP

- **binary_offset:** The byte offset of the GEN binary for this kernel, from the beginning of the file.
- **binary_size:** The size of the GEN binary for this kernel in bytes.

The file_scope_var_info structure describes the characteristics of a general variable and has the following format:


### file_scope_var_info
  file_scope_var_info {
    UB linkage;
    UW name_len;
    UB name[name_len];
    UB bit_properties;
    UW num_elements;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

- **linkage:** Valid values are


  - 0:  extern
  - 1:  static
  - 2:  global

- **name_len:** The length of the name of the file scope variable; it must be between 1 to 255 characters.
- **name:** The name of the file scope variable. It is not null-terminated.
- **bit_properties:** This byte contains two items: type and alignment.

  - type (bit 0-3): the data type of the variable. A variable may have type UD, D, UW, W, UB, B, UQ, Q, DF, and F.
  - alignment (bit 4-7): the minimum byte alignment requirement for this variable. Valid values are:


| Binary Value| Alignment| Bytes           |
| --- | --- | --- |
| 0            | BYTE      | 1 (no alignment) |
| 1            | WORD      | 2                |
| 2            | DWORD     | 4                |
| 3            | QWORD     | 8                |
| 4            | OWORD     | 16               |
| 5            | GRF       | 32               |
| 6            | 2_GRF     | 64               |
| 7            | HWORD     | 32               |
| 8            | 32WORD    | 64               |
| 9            | 64WORD    | 128              |


- **num_elements:** Describes the number of elements in this variable. Valid range is [1, 1024].
- **attribute_count:** Attributes for this variable.
- **attribute_info:** Description of Attributes for this variable. The format is described by attribute_info.

Metadata information of a vISA function object.  Its format is described by


### function_info
  function_info {
    UB linkage;
    UW name_len;
    UB name[name_len];
    UD offset;
    UD size;
    UW num_syms_variable;
    variable_reloc_symtab variable_reloc_symtab[num_syms_variable];
    UW num_syms_function;
    function_reloc_symtab function_reloc_symtab[num_syms_function];
  }

- **linkage:** For functions with extern linkage, the offset and size field must be 0. Valid values are


  - 0:  extern
  - 1:  static
  - 2:  global

- **name_len:** The length of the name of the function, must be between 1 to 65535 characters.
- **name:** The name of the function. It is not null-terminated.
- **offset:** The byte offset of the kernel, from the beginning of the file. The format of the kernel object is described in Section 4.5.8.
- **size:** The size of the kernel object in bytes. This does not include the optional GEN binary of the kernel.
- **num_syms_variable:** The number of file-scope variables refered to in this function.
- **variable_reloc_symtab:** The relocation maps for each file scope general variable symbolic reference. The format is described by reloc_sym
- **num_syms_function:** The number of file-scope functions refered to in this kernel.
- **function_reloc_symtab:** the relocation information (maps) for each function symbolic reference in this vISA object. The format is described by reloc_sym.


## vISA Object Format

| |
| --- |
| vISA Header                 |
| FN0 vISA Object             |
| FN1 vISA Object             |
| ...                         |
| KL0 vISA Object             |
| KL0 GEN Binary (Optional)   |
| ...                         |

**Kernels**

| **KL0**   | Name length| Name string | vISA offset | vISA size | vISA input offset | GEN binaries | GEN binary offsets |
| --- | --- | --- | --- | --- | --- | --- | --- |

**Functions**

| **FN0 Name** | Linkage | Name length | Name string | vISA offset | vISA size |
| --- | --- | --- | --- | --- | --- |



## String Pool

The virtual ISA kernel binary maintains a table of string literals used
by a kernel. Each entry contains a string represent the names of a
variable, function, or attribute. The strings may have variable length.
The maximum length of each string is determined by whether it's used as
a variable/attribute/function/kernel name. In the object file, the
string pool appears as a byte stream where each string is stored
consecutively and separated by the null character. The first string in
the table (**n0**) is reserved to represent the null string ("").


## Attributes

The kernel declaration itself as well as most variable declarations may
contain one or more attributes. The purpose of an attribute is to
provide extensible mechanism for the front-end compiler to pass
auxiliary information about the vISA object (e.g., whether a variable's
life range is limited to one subroutine) that may assist the finalizer
in its code generation. It is the front end compiler's responsibility to
ensure that the appropriate attribute values are applied as the
finalizer will not verify their correctness. The finalizer will silently
ignore attributes that it does not recognize.

Two types of attributes are supported: one is an attribute with integer
constant value, and the other with string constant value. An integer
attribute's size can be of 0 to 4 bytes. An integer attribute without
value (or 0 byte as its value) is considered as a boolean attribute, which
is equivalent to an integer attribute with 0 or 1 as its value. vISA APIs
allow clients to choose what size to use for an integer attribute.

The **attribute_info** structure has the following format:



### attribute_info
  attribute_info {
    UD name;
    UB size;
    value;
  }

- **name:** Index of the string storing the name of the attribute. It may have a maximum length of 64 bytes, and each byte must be one of the ASCII printable characters.
- **size:** The size in bytes of the attribute value.
- **value:** The attribute values in raw bytes, to be interpreted by the JIT-compiler.


In text format, global attributes may be specified through a sequence of .kernel_attr
directive as follows, with one attribute for each directive:

**.kernel_attr** &lt;name&gt;[=&lt;value&gt;]

Attributes of a variable declaration are in the following format:

&nbsp;&nbsp;&nbsp;&nbsp;  **attrs={<a0>,<a1>,...,<an>}**

where '{' and '}' are used to group all attributes, and each attribute **<ai>** is in this form:

&nbsp;&nbsp;&nbsp;&nbsp;  **<attrName>[=<attrValue>]**

If a variable does have attributes, this form should appear after the basic information of
variable declaration, which will be shown in the following sections.


In the subsequent sections we will list the pre-defined attributes
associated with each kind of variables.


## Immediate Constants

The virtual ISA supports integer and floating-point immediate constants.
Immediate constants may be directly used as operands in virtual ISA
instructions, and a type is always associated with its value in the text
format.

In binary format, a dword is used to store the bit representation of the
immediate constant's value for all types except DF, Q, and UQ, for which
two dwords are used. In text format, an immediate constant may be
expressed with the C constant notation, with the exception that type
suffix is not allowed since the constant type is already specified in
the operand.

The packed types may only be used by immediate constants. In binary
format, a dword is used to store the bit representation of the packed
vector's value. In text format, an immediate constant with type V and VF
is also expressed by the hexadecimal string representing its binary
value.


## Variables

Each kernel and function object in a virtual ISA file has a single
global scope for variables. Variables are classified based on their
characteristics, and a symbol table is created for each kind of
variables to store their information. The lifetime of a function
variable extends from entry into the function until the exit of the
function. Except for the input and the pre-defined variables, a variable
has unintialized value at function entry.

Following table lists the available variables in a virtual ISA object file:


| Variable Type| Storage Class| Max Count| R/W| Input| Indexable|
| --- | --- | --- | --- | --- | --- |
| G             | General       | 65536     | R/W | Y     | Y         |
| A             | Address       | 4096      | R/W | N     | N         |
| P             | Predicate     | 4096      | R/W | N     | N         |
| S             | Sampler       | 32        | R   | Y     | Y         |
| T             | Surface       | 256       | R   | Y     | Y         |



## General Variables

They are general-purpose read-write variables. The **var_info**
structure describes the characteristics of a general variable and has
the following format:



### var_info
  var_info {
    UD name_index;
    UB bit_properties;
    UW num_elements;
    UD alias_index;
    UW alias_offset;
    UB alias_scope_specifier;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

- **name_index:** Index of the string storing the original, possibly mangled name of the variable. The maximum length of the name string is 64 characters. This is mainly used for debugging purposes.
- **bit_properties:** This byte contains two items: type and alignment.

  - type (bit 0-3): the data type of the variable. A variable may have type UD, D, UW, W, UB, B, UQ, Q, DF, HF, and F.
  - alignment (bit 4-6): the minimum byte alignment requirement for this variable.

- **num_elements:** Describes the number of elements in this variable. Valid range is [1, 4096], and the variable size (num_elements * sizeof(type)) must be less than 4K bytes.
- **alias_index:** Indicates that the current variable is an alias of the variable indexed at alias_index, starting at byte alias_offset. An aliased variable does not have its own storage but is instead a reference to a subset of the elements in the base variable. A value of zero (the NULL variable) indicates that the variable is not aliased.
- **alias_offset:** See the descriptions for alias_index. Must also be zero if alias_index is set to zero. It is an error if the offset is not aligned to the type.
- **alias_scope_specifier:** Indicates if the current variable is an alias of a general variable in the local symbol table or a file scope variable in the global symbol table. Valid values are:



  - 0:  local
  - 1:  global

- **attribute_count:** Attributes for this variable.


Thirty-two variables (V0-V31) are reserved for pre-defined variables,
which have special meanings in the program.

| Name                  | Size   | Type   | R/W                             | Can be aliased   | Description |
| --- | --- | --- | --- | --- | --- |
| V0 (%null)            | 1      | N/A    | R                               | No               | The NULL variable. It represents the non-existence of a variable. It is used to provide default values for some fields in variable declarations (e.g., **alias_index**) and normally should not appear in virtual ISA instructions.    |
| V1(%thread_x)         | 1      | UW     | R                               | No               | The x-coordinate of the current thread in the thread space.                                                                                                                                                                            |
| V2(%thread_y)         | 1      | UW     | R                               | No               | The y-coordinate of the current thread in the thread space                                                                                                                                                                             |
| V3(%group_id_x)       | 1      | UD     | R                               | No               | The x component of the thread group id.                                                                                                                                                                                                |
| V4(%group_id_y)       | 1      | UD     | R                               | No               | The y component of the thread group id.                                                                                                                                                                                                |
| V5(%group_id_z)       | 1      | UD     | R                               | No               | The z component of the thread group id.                                                                                                                                                                                                |
| V6(%tm):              | 5      | UD     | TmLow: R TmHigh: R TmEvent: R   | No               | The timestamp register.                                                                                                                                                                                                                |
|                       |        |        | pause counter: R/W              |                  | **TmLow** **(tm.0):** the lower 32-bit of the timestamp.                                                                                                                                                                               |
|                       |        |        |                                 |                  | **TmHigh (tm.1):** the higher 32-bit of the teimstamp.                                                                                                                                                                                 |
|                       |        |        |                                 |                  | **TmEvent (tm.2)**: bit 0 indicates whether a time-impacting event (e.g., context switch) occurred since the performance counter was last accessed, therefore making its value suspect\ **.**                                          |
|                       |        |        |                                 |                  | **Pause counter (tm.4): {ICLLP+}** bit0-9 stores the pause duration. Bit0-4 must be zero.                                                                                                                                              |
|                       |        |        |                                 |                  | Writing to the pause counter causes the thread to pause (no new instructions issued) for approximately the cycles specified.                                                                                                           |
| V7(%r0)               | 8      | UD     | R                               | Yes              | The r0 register. The variable consists of eight dwords that represent the R0 thread payload header.\ ** **                                                                                                                             |
| V8(%arg)              | 256    | UD     | R/W                             | Yes              | The argument variable. It consists of up to 256 dwords and is used for argument passing between functions. The actual number of elements used by each function is specified in the vISA function object.                               |
| V9(%retval)           | 96     | UD     | R/W                             | Yes              | The return value variable. It consists of up to 96 dwords and is used to store the return value for function calls. The actual number of elements used by each function is specified in the vISA function object.                      |
| V10(%sp)              | 1      | UD     | R/W                             | No               | The stack pointer variable.                                                                                                                                                                                                            |
| V11(%fp)              | 1      | UD     | R/W                             | No               | The frame pointer varible.                                                                                                                                                                                                             |
| V12(%hw_id)           | 1      | UD     | R                               | No               | The HW thread id. It is a unique identifier for all concurrent threads, with range from [0, max_num_HW_threads-1]. The maximum number of hardware threads is platform and configuration dependent.                                     |
| V13(%sr0)             | 4      | UD     | R/W                             | No               | The state register.                                                                                                                                                                                                                    |
|                       |        |        |                                 |                  | **DMask (sr0.2):** the 32-bit mask specifying which channels are active at dispatch time                                                                                                                                               |
| V14(%cr0)             | 1      | UD     | R/W                             | No               | The control register. It can be used to control the floating-point computation mode for subsequent instructions in this thread.                                                                                                        |
|                       |        |        |                                 |                  | **F mode:** Bit 0 controls single floating point mode:                                                                                                                                                                                 |
|                       |        |        |                                 |                  | 0: IEEE mode for F type                                                                                                                                                                                                                |
|                       |        |        |                                 |                  | 1: ALT mode for F type                                                                                                                                                                                                                 |
|                       |        |        |                                 |                  | **FPU Rounding mode:** Bit 4-5                                                                                                                                                                                                         |
|                       |        |        |                                 |                  | 00b = Round to Nearest or Even (RTNE)                                                                                                                                                                                                  |
|                       |        |        |                                 |                  | 01b = Round Up, toward +inf (RU)                                                                                                                                                                                                       |
|                       |        |        |                                 |                  | 10b = Round Down, toward -inf (RD)                                                                                                                                                                                                     |
|                       |        |        |                                 |                  | 11b = Round Toward Zero (RTZ)                                                                                                                                                                                                          |
|                       |        |        |                                 |                  | **DF denorm mode:** Bit 6                                                                                                                                                                                                              |
|                       |        |        |                                 |                  | 0: Flush denorms to zero                                                                                                                                                                                                               |
|                       |        |        |                                 |                  | 1: Allow denorm values                                                                                                                                                                                                                 |
|                       |        |        |                                 |                  | **F denorm mode:** Bit 7                                                                                                                                                                                                               |
|                       |        |        |                                 |                  | 0: Flush denorms to zero                                                                                                                                                                                                               |
|                       |        |        |                                 |                  | 1: Allow denorm values                                                                                                                                                                                                                 |
|                       |        |        |                                 |                  | **HF denorm mode:** Bit 10                                                                                                                                                                                                             |
|                       |        |        |                                 |                  | 0: Flush denorms to zero                                                                                                                                                                                                               |
|                       |        |        |                                 |                  | 1: Allow denorm values                                                                                                                                                                                                                 |
|                       |        |        |                                 |                  | All other bits are reserved and may not be written to.                                                                                                                                                                                 |
| V15(%ce0)             | 1      | UD     | R                               | No               | The channel enable register. It contains the 32 bit execution mask for the current instruction.                                                                                                                                        |
| V16(%dbg0)            | 2      | UD     | R/W                             | No               | Debug register                                                                                                                                                                                                                         |
| V17(%color)           | 1      | UW     | R                               | No               | Color bit for media dispatch                                                                                                                                                                                                           |
| V18(%implicit_arg_ptr)| 1      | UQ     | R/W                             | Yes              | This pre-defined variable holds A64 pointer to implicit argument buffer. Buffer format is decided between IGC and NEO. Pointer value is initialized in kernel prolog. Stack call functions can access implicit arguments from this buffer. This pointer is valid only when implicit argument buffer is enabled on pre-XEHP. This pointer is not valid for XEHP+. On XEHP+ implicit arg buffer is available as a cross thread argument. So a stack call function only needs valid r0 to read the buffer. We dont need to preserve this pointer separately anywhere.           |
| V19(%implicit_local_id_buf_ptr)| 1      | UQ     | R/W                             | Yes              | This pre-defined variable holds A64 pointer to local_id buffer. Buffer format is decided between IGC and NEO. Pointer value is initialized in kernel prolog. Stack call functions can access local_id for each work-item from this buffer. This pointer is valid only when implicit argument buffer is enabled on pre-XEHP. This pointer is not valid for XEHP+. On XEHP+ local_id buffer is available as part of implicit_arg_ptr cross thread argument. So a stack call function only needs valid r0 to read the buffer. We dont need to preserve this pointer separately anywhere.            |

%thread_x, %thread_y, and %color are only valid with the media mode,
while %group_id_x, %group_id_y, and %group_id_z are only valid
with the GPGPU mode. %arg, %retval, %sp, and %fp have undefined values
at kernel start and must be explicitly initialized by the vISA program.
V20-V31 are currently reserved and may not be used.

In text format, a general variable may be declared with te following
syntax:

**.decl** var_name v_type=G type=<data_type> num_elts=<num_elements> [align=<align>] [alias=(<alias_variable_name>,<alias_offset>)] [attrs={<a0>,<a1>,...}]

The number of declared variables must be less than the maximum count
specified in this :ref:`table<table_VariableCategories>`. Variables are referred by their
name in the virtual ISA assembly.

In text format, variables can also be declared inside the scope tokens "{" and "}".

Variable names cannot be redefined in the same scope. (This includes pre-defined
variables in the kernel scope)

Variables defined in the kernel scope are visible in all scopes, and variables
defined in any outer scope is visible to all inner scopes.

Pre-defined Attributes:

**Scope** - Scope of the variable

-  Name: Scope

-  Size: 1

-  Value:

   -  0: kernel scope

   -  1: subroutine scope

   -  Other values are reserved

-  Description: provides information to the finalizer on the scope of
   this variable. A variable with kernel scope is visible anywhere in a
   kernel, including all of its subroutines. A variable with subroutine
   scope is visible only within the subroutine.
-  Note that this is an attribute used by the finalizer. It is not related
   to the scope tokens used in the text format.

**Output** - variable is output

-  Name: Output

-  Size: 0

-  Value: N/A

-  Description: if set, indicates that this variable should be kept live
   at kernel exit.


## Address Variables

An address variable is used to perform indirect access to elements in a
general variable. It may also point to a surface variable to support
vectors of surfaces. Address arithmetic may be performed using the
special ADDR_ADD instruction. The type of an address must be UW. The
address_info structure describes the characteristics of an address
variable and has the following format:



### address_info
  address_info {
    UD name_index;
    UW num_elements;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

The items have identical meaning as their counterparts in the
**var_info** structure. The legal range for **num_elements** is \[1,
16\]. In text format, an address variable may be declared with the
following syntax:

**.decl** var_name v_type=A num_elts=&lt;num_elements&gt; [attrs={<a0>,<a1>,...}]



## Predicate Variables

A predicate variable is used to facilitate conditional execution of
instructions. There are two ways to assign to a predicate variable:
through comparison instructions, or a*\* SETP*\* instruction with an
immediate value as its source, in which case the predicate variable will
be updated with the bit-values of the constant from the LSB. Predicate
variables may also be manipulated with logic instructions. The type of a
predicate must be bool. The **predicate_info** structure describes the
characteristics of a predicate variable and has the following format:



### predicate_info
  predicate_info {
    UD name_index;
    UW num_elements;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

The items again have the same meaning as their counterparts in the
**var_info** structure. The legal values for **num_elements** are {1,
2, 4, 8, 16, 32}. The predicate variable "**P0**" is reserved to
represent the case where there is no predication. In text format, a
predicate variable may be declared with the following syntax:

**.decl** var_name v_type=P num_elts=&lt;num_elements&gt; [attrs={<a0>,<a1>,...,<an>}]

"P0" is pre-defined and may not be declared in assembly.


## Sampler Variables

A sampler variable represents a handle to the sampler state information
when accessing the sampling engine. Sampler variables may not be created
in the kernel and instead must be passed in as kernel input arguments.
Their primary usage is as a parameter in sampler instructions.
S31 is reserved and represents bindless samplers.

| Sampler Index | Name  | Description      |
| ---           | ---   | ---              |
| 31            | S31   | Bindless Sampler |

In text format, a sampler variable may be declared with the following
syntax:

**.decl** &lt;var_name&gt; v_type = S num_elts=&lt;num_elements&gt; [attrs={<a0>,<a1>,...,<an>}]


## Surface Variables

A surface variable represents a handle to the underlying surface when
performing memory accesses. Surface variables may not be created in the
kernel and instead should be passed in as kernel input arguments. Their
primary usage is as a parameter in memory access, sampler, and VME
instructions.

T0-T5 are pre-defined surfaces that have special meanings in the
program.

In text format, a surface variable may be declared with the following
syntax:

**.decl** &lt;var_name&gt; v_type=T num_elts=&lt;num_elements&gt; [attrs={<a0>,<a1>,...,<an>}]

The pre-defined surfaces may not be declared in assembly.

Sampler and surface variables share the same format in the vISA file:



### surface_info
  surface_info {
    UD name_index;
    UW num_elements;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

Surface and sampler variables are permitted to have more than one
element. An element has a size of 4 byte.


## Labels

A label serves as the target of scalar control flow instructions (jump
and subroutine call). A label may be declared by the special LABEL
instruction, and once declared its location is fixed. The
**label_info** structure has the following format:



### label_info
  label_info {
    UD name_index;
    UB kind;
    UB attribute_count;
    attribute_info attribute_info[attribute_count];
  }

- **name_index:** Index of the string storing the original, possibly mangled name of the label. The maximum length of the name string is 1024 characters. This is mainly used for debugging purposes.
- **kind:** There are two kinds of labels, and bit 0 is used to encode this value:

  - Block ('0'): marks the start of a basic block.  Only block labels may be the target of a jump instruction.
  - Subroutine ('1'): marks the start of a subroutine.  Only subroutine labels may be the target of a subroutine call.

- **attribute_count:** Number of attributes for this label.


In text format, a label variable may be declared with the following
syntax:

**&lt;label_string&gt;:**


## Input Variables

A kernel may declare up to 256 input variables. An input variable may
have one of the following storage classes: General, Sampler, and
Surface. Input variables have pre-determined storage locations and are
read-only. The **input_info** structure describes the characteristics
of an input variable and has the following format:



### input_info
  input_info {
    B kind;
    UD id;
    W offset;
    UW size;
  }

- **kind:**

  - bits 0-1 are used to represent the category of the variable.  Valid values are

    - 00: General
    - 01: Sampler
    - 10: Surface

  - bit 2: must be zero (reserved for future expansion of category)
  - bits 7..3 form the provenance field, used to communicate to the runtime running the kernel where the value needs to come from. Currently the only runtime that allows a non-zero value in the provenance field is the CM runtime, which uses it to indicate whether the argument is a CM runtime implicit argument rather than a user supplied kernel argument. The CM runtime currently offers these implicit arguments:

    - 0: none; this is a user supplied kernel argument. All user supplied kernel arguments must come before all implicit arguments in the table.
    - 1: local_size (general, 3 x ud, allowing for Z dimension) [kernel]
    - 2: group_count (general, 3 x ud, allowing for Z dimension) [kernel]
    - 3: local_id (general, 3 x ud, allowing for Z dimension) [thread] other values up to 31: reserved for future use
    - 4: scoreboarding_dependency (general, 16 x ub) [kernel]
    - 5: scoreboarding_bti (surface, i32) [kernel]

    The CM runtime prefers thread implicit arguments to have a higher offset than kernel implicit arguments; ignoring this can impact performance by having to insert movs at the start of the kernel.

- **id:** The index of the variable in the corresponding symbol table.
- **offset:** The offset in bytes of where this input is stored in Gen's general register file. The offset must be aligned to the input variable's natural alignment. State input variables (sampler, surface) must be dword-aligned.
- **size:** The size of the input in bytes. Its value must be equal to type_size * num_elements for the variable.


It is an error if two inputs have overlapping offsets. An input general
variable has the following additional restrictions:

-   Its **alias_index** must be zero.
-   The offset must be GRF-aligned if the variable's size is larger than
    or equal to one GRF.
-   The variable must fit in one GRF if its size is less than one GRF
    (that is, it will not cross GRF-boundary).

In text format, an input with provenance field 0 may be specified with
the following format:

**.input var_name offset=&lt;offset&gt; size=&lt;size&gt;**

Where &lt;var_name&gt; must be one of general/sampler/surface variables
already declared.

An input with a non-zero provenance field may be specified with the
following format:

.implicit_UNDEFINED_&lt;n&gt; var_name offset=&lt;offset&gt;
size=&lt;size&gt;

where &lt;n&gt; is the value of the provenance field. For certain values
of &lt;n&gt;, a CM runtime specific mnemonic name may be used:

-   **.implicit_LOCAL_SIZE** is the same as
    **.implicit_UNDEFINED_1**
-   **.implicit_GROUP_COUNT** is the same as
    **.implicit_UNDEFINED_2**
-   **.implicit_LOCAL_ID** is the same as **.implicit_UNDEFINED_3**

Kernel
----------
Each kernel in a virtual ISA object has the following format:


### kernel_data
  kernel_data {
    UD string_count;
    string_pool string_pool[string_count];
    UD name_index;
    UD variable_count;
    var_info var_info[variable_count];
    UW address_count;
    address_info address_info[address_count];
    UW predicate_count;
    predicate_info predicate_info[predicate_count];
    UW label_count;
    label_info label_info[label_count];
    UB sampler_count;
    sampler_info sampler_info[sampler_count];
    UB surface_count;
    surface_info surface_info[surface_count];
    UB vme_count;
    vme_info vme_info[vme_count];
    UD num_inputs;
    input_info input_info[num_inputs];
    UD size;
    UD entry;
    UW attribute_count;
    attribute_info attribute_info[attribute_count];
    instructions;
  }

- **string_count:** Number of strings used in the kernel.  Valid range is [1 , 131072].
- **string_pool:** The string pool table, which stores all names used in the kernel.  String literals are represented by their index in this table (n#).  n0 is reserved to represent the null-string.
- **name_index:** Index to the string storing the name of the kernel, with a maximum length of 1023 characters.
- **variable_count:** Number of general-purpose variables declared in the kernel.  It does not include the pre-defined variables.
- **var_info:** They are general-purpose read-write variables. The var_info structure describes the characteristics of a general variable and has the following format:
- **address_count:** Number of address variables declared in the kernel.
- **address_info:** A global symbol table that declares all address variables used in the kernel. Address variables are represented by their index in this table in the code (a#). See description of the address_info structure for more details.
- **predicate_count:** Number of predicate variables declared in the kernel. It does not include the pre-defined variable p0.
- **predicate_info:** A global symbol table that declares all predicate variables used in the kernel. Predicates are represented by their index in this table in the code (P). The first entry (p0) is reserved to represent the case where there is no predication, so the first element in this able will be p1. The Most Significant Bit (MSB) of the predicate variable index controls whether the predicate value should be inverted. See description of the predicate_info structure for more details.
- **label_count:** Number of labels declared in the kernel.
- **label_info:** A global symbol table that declares all labels used in the kernel. Labels are represented by their index in this table in the code (l#). See description of the label_info structure for more details.
- **sampler_count:** The number of samplers declared for this kernel.
- **sampler_info:** A global symbol table that declares the sampler variables used in the kernel. See description of the state_var_info structure for more details.
- **surface_count:** The number of surfaces declared for this kernel.  It does not include the pre-defined surfaces.
- **surface_info:** A global symbol table that declares the surface variables used in the kernel. Surface variable T0-T4 are reserved and do not appear in the table (i.e., the first surface entry will be T5). See description of the state_var_info structure for more details.
- **vme_count:** The number of VME variables declared for this kernel.
- **vme_info:** A global symbol table that declares the VME variables used in the kernel. See description of the state_var_info structure for more details.
- **num_inputs:** The number of input arguments for this kernel.
- **input_info:** A global symbol table that declares all inputs for the kernel. Their index in this table corresponds to their original position in the source parameter list. See description of the input_info structure for more details.
- **size:** The size in bytes of the instructions for this kernel.
- **entry:** The byte offset of the first instruction in the kernel, from the start of this kernel object.
- **attribute_count:** Number of attributes for this kernel.
- **attribute_info:** Information about an attribute. See description of the attribute_info structure for more details.


In text format, a virtual ISA kernel has the following form:

**.kernel** &lt;name&gt;

One or more **.decl** directives for variable declarations

Zero or more **.kernel_attr** directives for attribute lists

Virtual ISA instructions

Note that we do not restrict the declaration ordering for the various
variable classes. Also, the length of each symbol table is not specified
as they may be derived from the number of declarations for that variable
class.

Pre-defined Attributes:
Note that an integer attribute can have its size to be of 0 to 4 bytes, the
size shown here is just a recommended value.

**OutputAsmPath** - Name of the GEN assembly file for this kernel

-   Name: AsmName
-   Size: 1-256
-   Value: string with a max length of 256. It is not null-terminated.
-   Description: gives the name of the GEN assembly file generated by
    the front end compiler for this kernel. It is intended for
    simulation mode only.

**Target** - which platform target is the kernel for.

-   Name: Target
-   Size: 1
-   Value: 0 (CM), 1(IGC).
-   Description: The target indicates which platform (CM, IGC, etc) generates this kernel.
    Currently, only 0 and 1 are used.

**SimdSize** - kernel dispatch simd size.

-   Name: SimdSize
-   Size: 1
-   Value: 8|16|32.
-   Description: It indicates the dispatch simd size for this kernel.


**SLMSize** - size of the SLM used by each thread group

-   Name: SLMSize
-   Size: 1
-   Value: 0-64 representing the SLM size in unit of 1KB memory blocks.
    {0, 1, 2, 4, 8, 16, 32, 64} are supported, all other values will be
    rounded up to the next power of two.
-   Description: gives the size of the shared local memory used by each
    thread group of the kernel. The runtime uses this value to perform
    memory management for SLM among the thread groups. A value of zero
    means SLM is not enabled for this kernel, and it is an error to
    access T0 in this kernel.

**SpillMemOffset** - The starting offset for vISA scratch-space

-   Name: SpillMemOffset
-   Size: 4
-   Value: representing the starting offset for vISA scratch-space.
-   Description: this is used by the front-end compiler to reserve
    scratch-space. If the kernel spills, its spill space will start
    at this offset.  SpillMemOffset is in bytes and must be multiple
    of the GRF size.

**ArgSize** - maximum size of the argument variable (%arg) for this
kernel in GRFs

-   Name: ArgSize
-   Size: 1
-   Value: 0-32.
-   Description: this is used for functions and must be present.

**RetValSize** - maximum size of the return value variable (%retval) for
this kernel in GRFs

-   Name: RetValSize
-   Size: 1
-   Value: 0-12.
-   Description: this is used for functions and must be present.

Function
------------
Each ISA function object has the following format:


### function_data
  function_data {
    UD string_count;
    string_pool string_pool[string_count];
    UD name_index;
    UD variable_count;
    var_info var_info[variable_count];
    UW address_count;
    address_info address_info[address_count];
    UW predicate_count;
    predicate_info predicate_info[predicate_count];
    UW label_count;
    label_info label_info[label_count];
    UB sampler_count;
    sampler_info sampler_info[sampler_count];
    UB surface_count;
    surface_info surface_info[surface_count];
    UB vme_count;
    vme_info vme_info[vme_count];
    UD size;
    UD entry;
    UB input_size;
    UB return_value_size;
    UW attribute_count;
    attribute_info attribute_info[attribute_count];
    instructions;
  }

It is identical to a kernel object except that a function does not have
inputs; function arguments are instead passed through explicit read and
writes to the pre-defined variable **%arg**. Similarly, return value is
conveyed through the pre-defined variable **%retval**. Two additional
fields are used to specify the size of the input and return value:

-   **input_size:** the size in GRFs for this funtion's input
    arguments.

    Inputs for this function must reside in the first **input_size**
    GRFs of the Arg variable (%arg). Valid values are \[0-32\].

-   **return_value_size:** the size in GRFs for this function's return
    value.

    Return values for this function must reside in the first
    **return_value_size** of the RetVal variable (%retval). Valid
    values are \[0-12\].

In text format, a virtual ISA assembly file has the following form:

**.global_function** &lt;name&gt;

One or more **.decl** directives for variable declarations

zero or more **.kernel_attr** directives for attribute lists

Virtual ISA instructions


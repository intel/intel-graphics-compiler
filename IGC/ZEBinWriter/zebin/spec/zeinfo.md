<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Version
The version information of ZEBIN can be found in version.md. ZEINFO changes
should be documented properly in version.md.

## Grammar

~~~
attribute_seq   -> attribute, attribute_seq
                   attribute

attribute       -> attribute_key : attribute_value

attribute_value -> { attribute_seq }
                   literal
~~~

## Type

All **literals** have one of the following types:

- int64: typedef int64_t     zeinfo_int64_t
- int32: typedef int32_t     zeinfo_int32_t
- bool: typedef bool        zeinfo_bool_t
- str: typedef std::string zeinfo_str_t
- float: typedef float     zeinfo_float_t

An **array of literals** type could be created by specifying `TypexN` as a type.
For example, `int32x3` is used to describe `reqd_work_group_size`, and `int32xN`
could be used to describe a dynamic sized int32 array.

# Container
| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ----- | ----- |
| version | str | Required | ZE Info version number. See Version above. |
| kernels | KernelsTy | Required | vector |
| functions | FunctionsTy | Optional | vector |
| global_host_access_table | HostAccessesTy | Optional | vector |
| kernels_misc_info | KernelsMiscInfoTy | Optional | vector. Other miscellaneous kernel information which is required by certain API requesters, but is not necessary for kernel execution. |
| kernels_cost_info | KernelsCostInfoTy | Optional | vector. kernel cost information which is experimental and required by certain API requesters, but is not necessary for kernel execution. |
<!--- Container --->

# Kernel Attributes
| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | |
| user_attributes | UserAttribute | Optional | A set of attributes given by the user in the kernel signature. |
| execution_env | ExecutionEnv | Required | |
| payload_arguments | PayloadArgumentsTy | Optional | vector |
| per_thread_payload_arguments | PerThreadPayloadArgumentsTy | Optional | vector |
| binding_table_indices | BindingTableIndicesTy | Optional | vector |
| per_thread_memory_buffers | PerThreadMemoryBuffersTy | Optional | vector |
| inline_samplers | InlineSamplersTy | Optional | vector |
| experimental_properties | ExperimentalProperties | Optional | A set of experimental attributes. |
| debug_env | DebugEnv | Optional | |
<!--- Kernel Kernels --->

A ze_info section may contain more than one kernel's attributes, each is
represented in a kernel attribute. The name attribute in kernel represent the
kernel's name.

The attributes that are supported in kernel are: **name**,
**Execution Environment**, **Payload Data** and **Memory Buffer**.

# Function Attributes

~~~
functions:
  - name: "string"
    attribute_seq
~~~

| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | |
| execution_env | ExecutionEnv | Required | |
<!--- Function Functions --->

Function attribute represents a non-kernel function's information. A ze_info
section may contains more than one function's attributes, each is
represented in a function attribute. The name attribute in function represent the
function name.

Function attributes may only present when the function can be externally called.

The attributes that are supported in function are: **name** and **Execution Environment**.

### Supported attributes in User Attributes:
This section defines the supported user_attributes.
The user attributes are kernel attributes set by the user in the kernel signature.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ |  ------ | ------ | ------ |
| intel_reqd_sub_group_size  | int32 | Optional | 0 | |
| intel_reqd_workgroup_walk_order  | int32x3 | Optional | [0, 0, 0] | |
| invalid_kernel  | str | Optional | | Describe the invalid kernel reason |
| reqd_work_group_size  | int32x3 | Optional | [0, 0, 0] | |
| vec_type_hint  | str | Optional | | |
| work_group_size_hint  | int32x3 | Optional | [0, 0, 0] | |
| intel_reqd_thread_group_dispatch_size | int32 | Optional | 0 | Hardware dispatches thread groups in batches, the size of which is controlled by the thread_group_dispatch_size parameter. When this parameter is set, it overrides driver heuristics by selecting the specified value for thread_group_dispatch_size. Only values supported by the hardware can be set. |
<!--- UserAttribute -->

## Execution Environment

~~~
execution_env:
  - attribute_seq
~~~

### Supported attributes in execution environment:
This section defines the execution_env attribute.
If an attribute is **Required**, it must be present in execution_env. If it's **Optional** and it's not present, the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ |  ------ | ------ | ------ |
| barrier_count | int32 | Optional | 0 | Number of barriers |
| disable_mid_thread_preemption | bool | Optional | false | |
| grf_count | int32 | Required | | |
| has_4gb_buffers | bool | Optional | false | Compiled for greater than 4GB buffers |
| has_device_enqueue| bool | Optional | false | |
| has_dpas | bool | Optional | false | |
| has_fence_for_image_access | bool | Optional | false | |
| has_global_atomics | bool | Optional | false | |
| has_multi_scratch_spaces | bool | Optional | false | |
| has_no_stateless_write | bool | Optional | false | |
| has_stack_calls | bool | Optional | false | When this value is true, it indicates that program uses stack calls. The implicit_arg_buffer is allocated accordingly.  |
| has_printf_calls | bool | Optional | false | When this value is true, it indicates that the kernel/function itself uses printf calls in its body. |
| require_assert_buffer | bool | Optional | false | When this value is true, it indicates that the kernel/function itself uses assert buffer in its body. |
| require_sync_buffer | bool | Optional | false | When this value is true, it indicates that the kernel/function itself uses sync buffer in its body. |
| has_indirect_calls | bool | Optional | false | When this value is true, it indicates that the kernel itself uses indirect calls in its body. |
| require_disable_eufusion | bool | Optional | false | When this value is true, it indicates that program requires EU fusion disable |
| indirect_stateless_count | int32 | Optional | 0 | |
| inline_data_payload_size | int32 | Optional | 0 | Size of inline data in cross-thread-payload in byte. The value is 0 when inline data is disabled (default). |
| offset_to_skip_per_thread_data_load | int32 | Optional | 0 | |
| offset_to_skip_set_ffid_gp | int32 | Optional | 0 | |
| required_sub_group_size | int32 | Optional | 0 | The value is given by users in kernel attributes "intel_reqd_sub_group_size" |
| required_work_group_size | int32x3 | Optional | [0, 0, 0] | The value of this key is a sequence of three int32, for example [256, 2, 1]. The values are given by users in kernel attributes "reqd_work_group_size" |
| simd_size | int32 | Required | | Valid value {1, 8, 16, 32} |
| slm_size | int32 | Optional | 0 | SLM size in bytes |
| private_size | int32 | Optional | 0 | This value represents the total stack size for private-only stacks design, or the total private variable size in bytes for shared stacks design. |
| spill_size | int32 | Optional | 0 | This value represents the total stack size for spill/fill-only stacks design, or the total spill/fill variable size in bytes for shared stacks design. |
| subgroup_independent_forward_progress | bool | Optional | false | |
| thread_scheduling_mode | <thread_scheduling_mode> | Optional | | Suggested thread arbitration policy. |
| work_group_walk_order_dimensions | int32x3 | Optional | [0, 1, 2] | The value of this key is a sequence of three int32. Valid values are x: [0, 0, 0] , xy: [0, 1, 0], xyz: [0, 1, 2], yx: [1, 0, 0], zyx: [2, 1, 0] |
| eu_thread_count | int32 | Optional | 0 | Number of threads per EU. If not specified, the information can be derived from grf_count. |
| has_sample | bool | Optional | false | |
| has_rtcalls | bool | Optional | false | |
| quantum_size | int32 | Optional | 0 | The required quantum size. When set, quantum dispatch is enabled and set to the provided quantum size. |
| quantum_walk_order | int32 | Optional | 0 | The required walk order of quantum. |
| quantum_partition_dimension | int32 | Optional | 0 | The partitioning dimension of quantum. |
| generate_local_id | bool | Optional | false | Flag of HW local ID capable info in cross-thread-payload. |
| has_lsc_stores_with_non_default_l1_cache_controls | bool | Optional | false | Flag of HasLscStoresWithNonDefaultL1CacheControls to determine if the resource barriers should flush UAV coherency. |
| require_iab | bool | Optional | false | When set to false, implicit arg buffer is not used by the program. So runtime may decide to not program the buffer. However, if debugger connects, the runtime would have to program implicit arg buffer. This flag may be false only when has_stack_calls is also false. It is an error to explicitly set this flag to false when has_stack_calls is true. When set to true, runtime must allocate implicit arg buffer. |
<!--- ExecutionEnv -->

### Supported thread scheduling mode:
Supported <thread_scheduling_mode> of execution_env.

| Thread Scheduling Mode | Description |
| ----- | ----- |
| age_based | |
| round_robin | |
| round_robin_stall | |
<!--- <thread_scheduling_mode> ArgThreadSchedulingMode -->

## Payload Arguments
This section defines payload_arguments attribute.
There are two kinds of payload arguments: **Payload Argument** and **Per Thread Payload Arguments**.
The Payload arguments defined here include explicit user arguments of a kernel, such as payload_arguments with arg_type that is arg_byvalue or arg_bypointer,
and implicit arguments inserted by the compiler, such as arguments with local_size arg_type.

### Supported attributes in payload arguments:
If an attribute is **Required**, it must be present in payload arguments. If it's **Optional** and it's not present, it's either not applicable to the specific argument_type, or the **Default** value is used.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ----- |
| arg_type | <argument_type> | Required | | |
| offset | int32 | Required | | |
| size | int32 | Required | | The number of bytes needed for allocating the argument. Allocated size is aligned to 4. |
| arg_index | int32 | Optional | -1 | Present when arg_type is "arg_bypointer", "arg_byvalue", "buffer_offset", or other implicit *image_* and sampler_* types. The value is the index of the associated kernel argument. |
| addrmode | <memory_addressing_mode> | Optional | | Present when arg_type is "arg_bypointer", or when arg_type is "const_base", "global_base", "inline_sampler" |
| addrspace | <address_space> | Optional | | Present when arg_type is "arg_bypointer" or "inline_sampler" |
| access_type | <access_type> | Optional | | Present when arg_type is "arg_bypointer" |
| sampler_index | int32 | Optional | -1 | Present when arg_type is "arg_bypointer" and address_space is "sampler", or when arg_type is "inline_sampler" |
| source_offset | int32 | Optional | -1 | Present when arg_type is "arg_byvalue" and the arg is a flattened aggregate element |
| slm_alignment | int32 | Optional | 0 | Present when arg_type is "arg_bypointer", addrmode is "slm" and address_space is "local" |
| image_type | <image_type> | Optional | | Present when addrspace is "image" |
| image_transformable | bool | Optional | false | Present when addrspace is "image" and image is transformable |
| sampler_type | <sampler_type> | Optional | | Present when addrspace is "sampler" |
| is_pipe | bool | Optional | false | Present when arg_type is "arg_bypointer" and type qualifier is pipe |
| is_ptr | bool | Optional | false | Present when arg_type is "arg_byvalue" and arg is used as pointer |
| bti_value | int32 | Optional | -1 | Present when arg_type is "const_base", "global_base" and when the buffer is promoted to BTI |
<!--- PayloadArgument PayloadArguments -->

### Supported argument types:
Supported <argument_type> of payload_arguments or per_thread_payload_arguments.

| Argument Type | Size | Description |
| ----- | ------ | ------ |
| packed_local_ids | int16x3 | Compacted local id x, y, z for simd1 kernel |
| local_id | int16 x N x n | N is the simd_size <br> n is the number of dimensions derived from work_group_walk_order_dimensions <br> Per id have to be GRF aligned |
| local_size | int32x3 | Number of work-items in a group |
| group_count | int32x3 | Number of group |
| work_dimensions | int32 | Work dimensions |
| global_size | int32x3 | OpenCL specific feature. The total number of work-items in each dimension |
| enqueued_local_size | int32x3 | OpenCL specific feature. The size returned by OCL get_enqueued_local_size API  |
| global_id_offset | int32x3 | |
| private_base_stateless | int64 | The base address of private buffer specified at per_thread_memory_buffers |
| buffer_address | | The address for buffer reference of stateful only memory access. |
| buffer_offset | | The extra offset for buffer reference to satisfy the alignment requirement of stateful memory access. |
| printf_buffer | | The address of printf_buffer which holds the printf strings information. |
| implicit_arg_buffer | int64 | The base address of implicit arg buffer |
| sync_buffer | int64 | The base address of sync buffer that is the return value of OCL "__builtin_IB_get_sync_buffer" |
| rt_global_buffer | int64 | The base address of rt global buffer that is the return value of OCL "__builtin_IB_intel_get_implicit_dispatch_globals" |
| assert_buffer | int64 | The base address of assert buffer that is the return value of OCL "__builtin_IB_get_assert_buffer" |
| indirect_data_pointer | int64 | Pointer to indirect data |
| scratch_pointer | int64 | Pointer to scratch surface |
| arg_byvalue | | Explicit kernel argument |
| arg_bypointer | | Explicit kernel argument |
| image_height | | Image height |
| image_width | | Image width |
| image_depth | | Image depth |
| image_num_mip_levels | | The number of mip-levels |
| image_channel_data_type | | Image channel data type |
| image_channel_order | | Image channel order |
| image_srgb_channel_order | | Image srgb channel order |
| image_array_size | | Image array size |
| image_num_samples | | The number of samples |
| sampler_address | | Sampler descriptor specifying the image addressing mode |
| sampler_normalized | | Sampler descriptor specifying whether the coordinates are passed in as normalized or unnormalized values |
| sampler_snap_wa | | Deprecated - To be removed in the next major version. Sampler descriptor specifying whether snap coordinate workaround is required |
| inline_sampler | | Implicit argument for OpenCL inline sampler in bindless addressing mode |
| const_base | | The base address of constant buffer, or the bindless offset of constant buffer if addrmode = "bindless" |
| global_base | | The base address of global buffer, or the bindless offset of global buffer if addrmode = "bindless" |
| region_group_size | int32x3 | The size of a region group in each dimension, in the order of dimension X, Y, Z. |
| region_group_dimension | int32 | The partitioning dimension of a region group |
| region_group_wg_count | int32 | The number of work groups in a region group |
| region_group_barrier_buffer | int64 | The address of region/subregion barrier buffer. The argument, when presents, indicates region/subregion barrier buffer is required for this kernel |
| buffer_size | int64 | Size in bytes of corresponding buffer |
<!--- <argument_type> ArgType -->

arg_byvalue and arg_bypointer are user arguments that are explicitly passed in from the applications. Other kinds of arguments are implicit arguments that are passed in by runtime.

int32x3 argument types can be partially programmed using size field of respective payload argument. When size == 4, only .x channel is programmed, when size == 8, both .x and .y channels are programmed, whereas when size == 12, all 3 channels are programmed.

By setting size attribute, local_id can be partially programmed. Considering SIMD32 scenario, when only x component is needed, size can be set to 64. When x and y component are needed, size can be set to 128. When x, y, and z components are needed, size can be set to 192. When local id is not needed, the argument type can be skipped. These sizes scale as per SIMD width of the program.

### Supported memory addressing modes:
Supported <memory_addressing_mode> of payload_arguments.

| Memory Addressing Mode | Description |
| ----- | ----- |
| stateless | |
| stateful | If an argument has stateful memory addressing mode, its binding table index will be specified in a **binding_table_indexes** with the same arg_index |
| bindless | |
| slm | |
<!--- <memory_addressing_mode> ArgAddrMode -->

### Supported address spaces:
Supported <address_space> of payload_arguments.

| Address Space | Description |
| ----- | ----- |
| global | |
| local | |
| constant | |
| image | |
| sampler  | |
<!--- <address_space> ArgAddrSpace -->

### Supported access types:
Supported <access_type> of payload_arguments.

| Access Type | Description |
| ----- | ----- |
| readonly | |
| writeonly | |
| readwrite | |
<!--- <access_type> ArgAccessType -->

### Supported image types:
Supported <image_type> of payload_arguments.

| Access Type | Description |
| ----- | ----- |
| image_buffer | A 1D image created from a buffer object |
| image_1d | A 1D image |
| image_1d_array | A 1D image array |
| image_2d | A 2D image |
| image_2d_array | A 2D image array |
| image_3d | A 3D image |
| image_cube | A cube image |
| image_cube_array | A cube image array |
| image_2d_depth | A 2D depth image |
| image_2d_array_depth | A 2D depth image array |
| image_2d_msaa | A 2D multi-sample color image |
| image_2d_msaa_depth | A 2D multi-sample depth image |
| image_2d_array_msaa | A 2D multi-sample color image array |
| image_2d_array_msaa_depth | A 2D multi-sample depth image array |
| image_2d_media | A 2D media image |
| image_2d_media_block | A 2D media block image |
<!--- <image_type> ArgImageType -->

### Supported sampler types:
Supported <sampler_type> of payload arguments.

| Access Type | Description |
| ----- | ----- |
| texture | A texture sampler |
| sample_8x8 | A 8x8 sampler |
| sample_8x8_2dconvolve | A 8x8 2D convolution sampler |
| sample_8x8_erode | A 8x8 erode sampler |
| sample_8x8_dilate | A 8x8 dilate sampler |
| sample_8x8_minmaxfilter | A 8x8 minmax filter sampler |
| sample_8x8_minmax | A 8x8 minmax sampler |
| sample_8x8_centroid | A 8x8 centroid sampler |
| sample_8x8_bool_centroid | A 8x8 bool centroid sampler |
| sample_8x8_bool_sum | A 8x8 bool sum sampler |
<!--- <sampler_type> ArgSamplerType -->

## Per Thread Payload Arguments
This section defines per_thread_payload_arguments attribute.
Per Thread Payload Arguments are implicit arguments inserted by the compiler. They are allocated per-thread.

| Attribute | Type | Description |
| ------ | ------ | ------ |
| arg_type | <argument_type> | |
| offset | int32 | |
| size | int32 | |
<!--- PerThreadPayloadArgument PerThreadPayloadArguments -->

## Binding Table Indices
This section defines binding_table_indices attribute.
Binding table index of the corresponding payload_argument.
The payload_argument must have **arg_bypointer** arg_type and **stateful** addrmode

| Attribute | Type | Description |
| ------ | ------ | ------ |
| bti_value | int32 | |
| arg_index | int32 | |
<!--- BindingTableIndex BindingTableIndices -->

## Per Thread Memory Buffer
This section defines the per_thread_memory_buffers attribute,
which indicate the memory buffer required by the Compiler.

| Attribute | Type | Required/Optional | Default | Description |
| ----- | ----- | ----- | ----- | ----- |
| type           | <allocation_type> | Required | | |
| usage          | <memory_usage>    | Required | | |
| size           | int32             | Required | | the buffer size in byte |
| slot           | int32             | Optional | 0     | Present when type is "scratch". Indicate the slot id of this scratch buffer. |
| is_simt_thread | bool              | Optional | false | Present when type is "global". Indicate if the global buffer is allocated per-SIMT-thread. If set to false, the buffer is allocated per-hardware-thread |
<!--- PerThreadMemoryBuffer PerThreadMemoryBuffers -->

### Supported allocation types:
Supported <allocation_type> of Per Thread Memory Buffer.

| Allocation Type | Description |
| ----- | ----- |
| global | Only the memory usage "private_space" can have global type <br> The base address of the global buffer will be passed in by payload_argument with private_base_stateless type
| scratch | Scratch could be bindless surface or stateless <br> The base offset of this scratch will be passed in r0.5. If more than one scratch buffer is requested, the scratch index of each is set by convention |
| slm | |
<!--- <allocation_type> MemBufferType -->

### Supported memory usages:
Supported <memory_usage> of Per Thread Memory Buffer.

| Memory Usage | Description |
| ----- | ----- |
| private_space | Memory space for private variables allocation and arguments passing for stack call |
| spill_fill_space | Memory space for register spill/fill and caller/callee saved for stack call |
| single_space | All compiler required memory space (privates, arguments passing, spill/fill, call/callee saved) are allocated in one single buffer |
<!--- <memory_usage> MemBufferUsage -->

## Inline samplers
This section defines inline_samplers of a kernel or an external function.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ------ |
| sampler_index | int32 | Required | | sampler index |
| addrmode | <sampler_desc_addrmode> | Required | | addressing mode |
| filtermode | <sampler_desc_filtermode> | Required | | filter mode |
| normalized | bool | Optional | false | normalized coordinates, present when inline sampler is normalized |
<!--- InlineSampler InlineSamplers -->

### Supported sampler addressing modes:
Supported <sampler_desc_addrmode> of inline sampler.

| Access Type | Description |
| ----- | ----- |
| none | CLK_ADDRESS_NONE |
| clamp_border | CLK_ADDRESS_CLAMP |
| clamp_edge | CLK_ADDRESS_CLAMP_TO_EDGE |
| repeat | CLK_ADDRESS_REPEAT |
| mirror | CLK_ADDRESS_MIRRORED_REPEAT |
<!--- <sampler_desc_addrmode> ArgSamplerAddrMode -->

### Supported sampler filter modes:
Supported <sampler_desc_filtermode> of inline sampler.

| Access Type | Description |
| ----- | ----- |
| nearest | CLK_FILTER_NEAREST |
| linear | CLK_FILTER_LINEAR |
<!--- <sampler_desc_filtermode> ArgSamplerFilterMode -->

## Experimental Properties
This section defines experimental_properties of a kernel/function.
The experimental attributes are experimental information for early evaluation or experiment.
They must not affect the kernel execution correctness and are subject to be changed or removed.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ------ |
| has_non_kernel_arg_load   | int32 | Optional | -1 | If this kernel/function contains load that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable.   |
| has_non_kernel_arg_store  | int32 | Optional | -1 | If this kernel/function contains store that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable.  |
| has_non_kernel_arg_atomic | int32 | Optional | -1 | If this kernel/function contains atomic that cannot be traced back to kernel arguments. 0 if false, 1 if true, -1 if not applicable. |
<!--- ExperimentalProperties -->

## Debug Environment
This section defines debug_env attributes to represent all debug related
information. Currently only SIP surface information is provided.

| Attribute | Type | Required/Optional | Default | Description |
| ------ | ------ | ------ | ------ | ------ |
| sip_surface_bti | int32 | Optional | -1 | |
| sip_surface_offset | int32 | Optional | -1 | |
<!--- DebugEnv -->

## Host Access Attributes
This section defines mapping between device and host name of a global variable.

| Attribute | Type | Description |
| ----- | ----- | ----- |
| device_name | str | Mangled global variable name derived from Name parameter LinkageAttributes. |
| host_name | str | User-specified global variable name derived from HostAccesINTEL decoration. Used to access global variable from host API calls. |
<!--- HostAccess HostAccesses --->

# Kernel Misc Attributes
This section defines the supported attribute for kernel_misc_info.

| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | kernel name |
| args_info | ArgsInfoTy | Optional | vector. kernel arguments info used by OpenCL API: clGetKernelArgInfo. |
<!--- KernelMiscInfo KernelsMiscInfo --->

## Kernel Argument Info Attributes
This section defines the supported attribute for args_info.

| Attribute | Type | Required/Optional | Default | Description |
| ----- | ----- | ------ | ----- | ----- |
| index | int32 | Required | | argument index |
| name | str | Optional | | argument name |
| address_qualifier | str | Required | | |
| access_qualifier | str | Required | | |
| type_name | str | Required | | |
| type_qualifiers | str | Required | | |
<!--- ArgInfo ArgsInfo --->

# Kernel Cost Attributes
This section defines the supported attribute for kernel_cost_info.

| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- |
| name | str | Required | kernel name |
| kcm_args_sym | KCMArgsSymTy | Optional | vector. Argument symbols used for kernel cost model. |
| kcm_loop_count_exps | KCMLoopCountExpsTy | Required | vector. Loop count expression for every loop used for kernel cost estimation. |
| Kcm_loop_costs| KCMLoopCostsTy | Required |vector. Cost of every loop in the kernel in cycles bytes loaded and stored. |
<!--- KernelCostInfo KernelsCostInfo --->


## KCMArgSym Attributes
This section defines the supported attribute for kernel cost model argument symbol .

| Attribute | Type | Required/Optional | Default | Description |
| ----- | ----- | ------ | ----- | ----- |
| argNo | int32 | Required | | position of the kernel argument this symbol is based on |
| byteOffset | int32 | Required | | offset from memory pointer if indirect argument symbol|
| sizeInBytes | int32 | Required | | the size of this argument symbol in bytes|
| isInDirect | bool | Required | |true when argument symbol is a pointer |
<!--- KCMArgSym KCMArgsSym --->

## KCMLoopCountExp Attributes
This section defines the supported attribute for kernel cost model loop count expression.
Loop count expression keeps track of argument index and uses factor and constant (C) to generate LCE

| Attribute | Type | Required/Optional | Default | Description |
| ----- | ----- | ------ | ----- | ----- |
| factor | float | Required | | LCE = factor * argument symbol + C |
| argsym_index | int32 | Required | | index for the argument symbol used for LCE|
| C | float | Required | |LCE = factor * argument symbol + C |
<!--- KCMLoopCountExp KCMLoopCountExps --->

## KCMLoopCost Attributes
This section defines the supported attribute for kernel cost model loop cost information.
Loop Cost is multiplied by Loop count expression to generate total kernel cost

| Attribute | Type | Required/Optional | Description |
| ----- | ----- | ------ | ----- | ----- |
| cycle | int32 | Required | Total cycles for the of a single loop |
| bytes_loaded | int32 | Required | total bytes loaded by a loop|
| bytes_stored | int32 | Required | total bytes stored by a loop |
| num_loops | int32 | Required | number of immediate child loops |
<!--- KCMLoopCost KCMLoopCosts --->


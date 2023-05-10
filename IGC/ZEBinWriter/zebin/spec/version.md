<!---======================= begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# ZEBIN Version
Version 1.29

## Versioning
Format: \<_Major number_\>.\<_Minor number_\>
- Major number: Increase when non-backward-compatible features are added. For example, rename attributes or remove attributes.
- Minor number: Increase when backward-compatible features are added. For example, add new attributes.

## Change Note
- **Version 1.30**: Add assert_buffer to payload arguments.
- **Version 1.29**: Add has_rtcalls to execution_env.
- **Version 1.28**: Add const_base and global_base to payload arguments.
- **Version 1.27**: Add has_sample to execution_env.
- **Version 1.26**: Add rt_global_buffer to payload arguments.
- **Version 1.25**: Add is_ptr attribute to payload arguments.
- **Version 1.24**: Add eu_thread_count to execution_env.
- **Version 1.23**: Add is_pipe attribute to payload arguments.
- **Version 1.22**: Add sync_buffer to payload arguments.
- **Version 1.21**: Add indirect_stateless_count to execution_env.
- **Version 1.20**: Add inline samplers to kernel.
- **Version 1.19**: Add a top layer attribute kernel_misc_info with arg_info attribute.
- **Version 1.18**: Add user_attributes to kernels.
- **Version 1.17**: Add buffer_address to payload argument type.
- **Version 1.16**: Add NT_INTELGT_ZEBIN_VERSION note to represent the ZEBIN ELF file version.
- **Version 1.15**: Add image_type, image_transformable, and sampler_type attributes to payload argument. Add new image and sampler argument types.
- **Version 1.14**: Add slm_alignment to payload argument.
- **Version 1.13**: Add functions with the name and execution env.
- **Version 1.12**: Add global_host_access_table to container.
- **Version 1.11**: Add require_disable_eufusion attribute.
- **Version 1.10**: Add thread_scheduling_mode to execution_env.
- **Version 1.9**: Add source_offset to payload argument.
- **Version 1.8**: Add inline_data_payload_size to execution_env.
- **Version 1.7**: Add debug_env to kernel.
- **Version 1.6**: Remove actual_kernel_start_offset from execution environment.
- **Version 1.5**: Add payload_argument type work_dimensions.
- **Version 1.4**: Add sampler_index to payload arguments.
- **Version 1.3**: Add printf_buffer to argument_type.
- **Version 1.2**: Add buffer_offset to argument_type.
- **Version 1.1**: Add experimental_properties to kernel.
- **Version 1.0**: Add version number. Add slot to per_thread_memory_buffers. Rename shared_local_memory to slm in memory_addressing_mode.

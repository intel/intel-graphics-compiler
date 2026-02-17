<!---======================= begin_copyright_notice ============================

Copyright (C) 2022-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# ZEBIN Version
Version 1.65
=======

## Versioning
Format: \<_Major number_\>.\<_Minor number_\>
- Major number: Increase when non-backward-compatible features are added. For example, rename attributes or remove attributes.
- Minor number: Increase when backward-compatible features are added. For example, add new attributes.

## Change Note
- **Version 1.65**: Deprecate sampler_snap_wa arg.
- **Version 1.64**: Remove dead-code flat_image args.
- **Version 1.63**: Internal feature.
- **Version 1.62**: Internal feature.
- **Version 1.61**: Add execution env require_assert_buffer and require_sync_buffer.
- **Version 1.60**: Internal feature.
- **Version 1.59**: Add execution env has_printf_calls and has_indirect_calls.
- **Version 1.58**: Add new enum value NT_INTELGT_INDIRECT_ACCESS_BUFFER_VERSION
- **Version 1.57**: Internal feature.
- **Version 1.56**: Internal feature.
- **Version 1.55**: Internal feature.
- **Version 1.54**: Add execution env flag require_iab.
- **Version 1.53**: Internal changes.
- **Version 1.52**: Add has_lsc_stores_with_non_default_l1_cache_controls flag in zeinfo environment.
- **Version 1.51**: Attribute size in payload_arguments now shows the number of bytes needed for allocating the argument. It is no longer aligned to 4.
- **Version 1.50**: Add generateLocalID flag.
- **Version 1.49**: Add support for quantum_size, quantum_walk_order and quantum_partition_dimension
- **Version 1.48**: Add relocation type "R_SYM_ADDR_16" and update supported relocation types.
- **Version 1.47**: Added kernel cost analysis.
- **Version 1.46**: Change cmake target directory from IGC/zebinlib to IGC/Libraries/zebinlib
- **Version 1.45**: Add .text section into ELF section content.
- **Version 1.44**: Add implicit arg for inline sampler in bindless mode.
- **Version 1.43**: Add support for region_group_barrier_buffer.
- **Version 1.42**: Enable is_ptr attribute in indirect payload arguments.
- **Version 1.41**: Add intel_reqd_thread_group_dispatch_size to user_attributes.
- **Version 1.40**: Add buffer_size to argument_type.
- **Version 1.39**: Add private_size and spill_size to execution_env.
- **Version 1.38**: Add new enum value NT_INTELGT_INDIRECT_ACCESS_DETECTION_VERSION to .note.intelgt.compat section.
- **Version 1.37**: Specify bindless addrmode for const_base and global_base arguments.
- **Version 1.36**: Specify values for EI_ABIVERSION field.
- **Version 1.35**: Update eu_thread_count to be an optional attribute.
- **Version 1.34**: Add support for region_group_size, region_group_dimension and region_group_wg_count.
- **Version 1.33**: Add new enum value NT_INTELGT_PRODUCT_CONFIG to .note.intelgt.compat section.
- **Version 1.32**: Add new enum value for VISA ABI to .note.intelgt.compat section.
- **Version 1.31**: Add support for indirect_data_pointer and scratch_pointer.
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

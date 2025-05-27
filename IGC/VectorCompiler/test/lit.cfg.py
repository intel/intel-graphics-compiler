# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import lit.formats
import lit.util
import os

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst
from lit.llvm.subst import FindTool

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = 'vc-opt'

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.ll']

# excludes: A list of directories  and files to exclude from the testsuite.
config.excludes = ['CMakeLists.txt']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.test_run_dir, 'test_output')

llvm_config.with_environment('LD_LIBRARY_PATH', config.cm_opt_lib_dir, append_path=True)

llvm_config.use_default_substitutions()

config.substitutions.append(('%PATH%', config.environment['PATH']))
config.substitutions.append(('%VC_PRINTF_OCL_BIF_TYPED_PTRS%', '{}/VCBiFPrintfOCL64.typed.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_PRINTF_OCL_BIF_OPAQUE_PTRS%', '{}/VCBiFPrintfOCL64.opaque.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_SPIRV_BIF_TYPED_PTRS%', '{}/VCSPIRVBuiltins64.typed.opt.bc'.format(config.vc_bif_binary_dir)))
config.substitutions.append(('%VC_SPIRV_BIF_OPAQUE_PTRS%', '{}/VCSPIRVBuiltins64.opaque.opt.bc'.format(config.vc_bif_binary_dir)))

platforms = config.vc_platform_list.split(";")
for platform in platforms:
  bif_file_typed_ptrs = '{}/VCBuiltins64_{}.typed.vccg.bc'.format(config.vc_bif_binary_dir, platform)
  bif_file_opaque_ptrs = '{}/VCBuiltins64_{}.opaque.vccg.bc'.format(config.vc_bif_binary_dir, platform)
  if config.opaque_pointers_enabled == 1:
    bif_file_default = bif_file_opaque_ptrs
  else:
    bif_file_default = bif_file_typed_ptrs
  config.substitutions.append(('%VC_BIF_{}%'.format(platform), bif_file_default))
  config.substitutions.append(('%VC_BIF_{}_TYPED_PTRS%'.format(platform), bif_file_typed_ptrs))
  config.substitutions.append(('%VC_BIF_{}_OPAQUE_PTRS%'.format(platform), bif_file_opaque_ptrs))

tool_dirs = [
  config.cm_opt_bin_dir,
  config.oneapi_readelf_dir,
  config.llvm_tools_dir]

# Add extra args for opt to remove boilerplate from tests.
vc_extra_args_legacy_pm = ['-load', config.llvm_plugin]
vc_extra_args_new_pm = ['-load-pass-plugin', config.llvm_new_pm_plugin]

extra_args_typed_legacy = vc_extra_args_legacy_pm+[config.opaque_pointers_disable_opt]
extra_args_opaque_legacy = vc_extra_args_legacy_pm+[config.opaque_pointers_enable_opt]
extra_args_default = vc_extra_args_legacy_pm+[config.opaque_pointers_default_arg_opt]
extra_args_typed_new_pm = vc_extra_args_new_pm+[config.opaque_pointers_disable_opt]
extra_args_opaque_new_pm = vc_extra_args_new_pm+[config.opaque_pointers_enable_opt]

if int(config.llvm_version) >= 16:
  command_opt_legacy = 'true ||'
  command_opt_new_pm = FindTool('opt')
  command_not_legacy = 'true ||'
  command_not_new_pm = FindTool('not')
else:
  command_opt_legacy = FindTool('opt')
  command_opt_new_pm = 'true ||'
  command_not_legacy = FindTool('not')
  command_not_new_pm = 'true ||'

command_opt = FindTool('opt')

if int(config.llvm_version) >= 16:
    command_opt_default = command_opt_new_pm
else:
    command_opt_default = command_opt_legacy

# Use one of the %opt version explicitly to override the default setting in the
# course of LITs' migration to opaque pointers.

opt_tool_typed_ptrs = ToolSubst('%opt_typed_ptrs', extra_args=extra_args_typed_legacy, command=command_opt)
opt_tool_opaque_ptrs = ToolSubst('%opt_opaque_ptrs', extra_args=extra_args_opaque_legacy, command=command_opt)

opt_tool_legacy_typed = ToolSubst('%opt_legacy_typed', extra_args=extra_args_typed_legacy, command=command_opt_legacy)
opt_tool_legacy_opaque = ToolSubst('%opt_legacy_opaque', extra_args=extra_args_opaque_legacy, command=command_opt_legacy)

opt_tool_new_pm_typed = ToolSubst('%opt_new_pm_typed', extra_args=extra_args_typed_new_pm, command=command_opt_new_pm)
opt_tool_new_pm_opaque = ToolSubst('%opt_new_pm_opaque', extra_args=extra_args_opaque_new_pm, command=command_opt_new_pm)

opt_tool_not_legacy = ToolSubst('%not_legacy', command=command_not_legacy)
opt_tool_not_new_pm = ToolSubst('%not_new_pm', command=command_not_new_pm)

llc_tool_typed_ptrs = ToolSubst('%llc_typed_ptrs', extra_args=extra_args_typed_legacy, command=FindTool('llc'))
llc_tool_opaque_ptrs = ToolSubst('%llc_opaque_ptrs', extra_args=extra_args_opaque_legacy, command=FindTool('llc'))

opt_tool_old_pm = ToolSubst('%opt', extra_args=extra_args_default, command=command_opt_default)

tools = [ToolSubst('not'),
         opt_tool_old_pm,
         opt_tool_not_legacy,
         opt_tool_not_new_pm,
         opt_tool_typed_ptrs,
         opt_tool_opaque_ptrs,
         opt_tool_legacy_opaque,
         opt_tool_legacy_typed,
         opt_tool_new_pm_opaque,
         opt_tool_new_pm_typed,
         llc_tool_typed_ptrs,
         llc_tool_opaque_ptrs,
         ToolSubst('llc', extra_args=vc_extra_args_legacy_pm+[config.opaque_pointers_default_arg_opt]),
         ToolSubst('oneapi-readelf', unresolved='ignore'),
         ToolSubst('llvm-dwarfdump'),
         ToolSubst('%igc-lld', command=FindTool('ld.lld'))]

if int(config.llvm_version) < 11:
  config.substitutions.append(('%not_for_vc_diag%', 'not'))
  config.substitutions.append(('%use_old_pass_manager%', ''))
else:
  config.substitutions.append(('%not_for_vc_diag%', 'not --crash'))
  config.substitutions.append(('%use_old_pass_manager%', '-enable-new-pm=0'))

if int(config.llvm_version) < 12:
  config.available_features.add('llvm_11_or_less')
else:
  config.available_features.add('llvm_12_or_greater')

if int(config.llvm_version) >= 16:
  config.available_features.add('llvm_16_or_greater')

if config.oneapi_readelf_dir:
  config.available_features.add('oneapi-readelf')

llvm_config.add_tool_substitutions(tools, tool_dirs)

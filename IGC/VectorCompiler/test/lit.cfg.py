# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
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
config.substitutions.append(('%OCL_GENERIC_BIF%', config.ocl_generic_bif))
config.substitutions.append(('%VC_PRITF_OCL_BIF%', config.vc_printf_ocl_bif))
config.substitutions.append(('%VC_SPIRV_OCL_BIF%', config.vc_spirv_ocl_bif))

platforms = config.vc_platform_list.split(";")
for platform in platforms:
  emulib_path = '{}_{}.vccg.bc'.format(config.vc_emulation_bif_prefix, platform)
  config.substitutions.append(('%VC_EMULATION_BIF_{}%'.format(platform), emulib_path))

if config.use_khronos_spirv_translator_in_sc == "1":
  config.substitutions.append(('%SPV_CHECK_PREFIX%', 'CHECK-KHR'))
else:
  config.substitutions.append(('%SPV_CHECK_PREFIX%', 'CHECK-LEGACY'))

tool_dirs = [
  config.cm_opt_bin_dir,
  config.oneapi_readelf_dir,
  config.llvm_tools_dir]

# Add extra args for opt to remove boilerplate from tests.
vc_extra_args = ['-load', config.llvm_plugin]
tools = [ToolSubst('not'),
         ToolSubst('opt', extra_args=vc_extra_args+[config.llvm_dependent_opt_flags]),
         ToolSubst('llc', extra_args=vc_extra_args),
         ToolSubst('oneapi-readelf'),
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

llvm_config.add_tool_substitutions(tools, tool_dirs)

# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import lit.formats
import lit.util

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst
from lit.llvm.subst import FindTool

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = 'OfflineCompilationTests'

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.cl', '.ll', '.spvasm']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.test_run_dir, 'test_output')

llvm_config.use_default_substitutions()

llvm_config.with_environment('LD_LIBRARY_PATH', [config.ocloc_lib_dir,
                                                 config.igc_lib_dir,
                                                 config.cclang_lib_dir], append_path=True)


tool_dirs = [config.ocloc_dir, config.llvm_tools_dir, config.spirv_as_dir]

if llvm_config.add_tool_substitutions([ToolSubst('ocloc', unresolved='break')], tool_dirs) is False:
  lit_config.note('Did not find ocloc in %s, ocloc will be used from system paths' % tool_dirs)

llvm_config.add_tool_substitutions([ToolSubst('llvm-dwarfdump')], tool_dirs)
llvm_config.add_tool_substitutions([ToolSubst('opt')], tool_dirs)
llvm_config.add_tool_substitutions([ToolSubst('not')], tool_dirs)

if not config.regkeys_disabled:
  config.available_features.add('regkeys')

if config.spirv_as_enabled:
  config.available_features.add('spirv-as')
  llvm_config.add_tool_substitutions([ToolSubst('spirv-as', unresolved='fatal')], tool_dirs)

if config.is32b == "1":
  config.available_features.add('sys32')

if config.debug_build:
  config.available_features.add('debug')

if config.use_khronos_spirv_translator_in_sc == "1":
  config.available_features.add('khronos-translator')
  config.available_features.add('khronos-translator-' + config.llvm_version_major)
  config.substitutions.append(('%SPV_CHECK_PREFIX%', 'CHECK-KHR'))
else:
  config.available_features.add('legacy-translator')
  config.substitutions.append(('%SPV_CHECK_PREFIX%', 'CHECK-LEGACY'))

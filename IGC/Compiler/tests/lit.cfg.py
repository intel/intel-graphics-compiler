# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
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
config.name = 'IGC'

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

llvm_config.use_default_substitutions()

config.substitutions.append(('%PATH%', config.environment['PATH']))

tool_dirs = [config.igc_opt_dir, config.llvm_tools_dir]
tools = [ToolSubst('not'), ToolSubst('split-file'), ToolSubst('igc_opt')]

llvm_config.add_tool_substitutions(tools, tool_dirs)


llvm_version = int(config.llvm_version)

config.substitutions.append(('%LLVM_DEPENDENT_CHECK_PREFIX%', f'CHECK-LLVM-{llvm_version}'))

if llvm_version >= 14:
  config.available_features.add('llvm-14-plus')

if llvm_version <= 15:
  config.available_features.add('llvm-15-or-older')

if llvm_version >= 15:
  config.available_features.add('llvm-15-plus')

if llvm_version >= 16:
  config.available_features.add('llvm-16-plus')

if not config.regkeys_disabled:
  config.available_features.add('regkeys')
if config.opaque_pointers == '1':
  config.available_features.add('opaque-ptr-fix')

  config.substitutions.append(('%RT_CHECK_PREFIX%', 'CHECK-RT'))


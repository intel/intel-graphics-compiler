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
tools = [ToolSubst('igc_opt')]

llvm_config.add_tool_substitutions(tools, tool_dirs)

# Add substitutions for pass configuration options to account for
# opt CLI changes between LLVM 10 and 11.
# FIXME: Remove once older-than-11 LLVM versions go out of use.
if int(config.llvm_version) < 11:
  config.substitutions.append(('%enable-basic-aa%', '-basicaa'))
else:
  config.substitutions.append(('%enable-basic-aa%', '--basic-aa'))
# Add LLVM version-dependent check prefixes.
# FIXME: Remove altogether after unifying all supported LLVM versions at 14+.
if int(config.llvm_version) < 14:
  config.substitutions.append(('%LLVM_DEPENDENT_CHECK_PREFIX%', 'CHECK-PRE-LLVM-14'))
else:
  config.substitutions.append(('%LLVM_DEPENDENT_CHECK_PREFIX%', 'CHECK-LLVM-14-PLUS'))
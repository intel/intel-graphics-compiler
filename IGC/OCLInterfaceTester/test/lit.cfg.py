# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import os
import lit.formats
import lit.util

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst

# name: The name of this test suite.
config.name = 'interface-tests'

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.test']

# excludes: A list of directories  and files to exclude from the testsuite.
config.excludes = ['CMakeLists.txt']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests shoul_d be run.
config.test_exec_root = os.path.join(config.test_run_dir, 'test_output')

# Standard substitutions: %s, %t, FileCheck, etc.
llvm_config.use_default_substitutions()

# dlopen finds libigc.so (+ deps) at runtime: PATH on Windows, LD_LIBRARY_PATH on Linux.
shared_lib_env = 'PATH' if 'system-windows' in config.available_features else 'LD_LIBRARY_PATH'

llvm_config.with_environment(shared_lib_env,
                             [config.igc_lib_dir],
                             append_path=True)

# regkeys feature gate (only needed if a .test uses `REQUIRES: regkeys`)
if not config.regkeys_disabled:
    config.available_features.add('regkeys')

# define tool dirs for tool substitutions
tool_dirs = [
    config.interface_tester_dir,
    config.llvm_tools_dir]

# In an ASan build the ASan runtime must load first, or it prints
# "==PID==ASan runtime does not come first ..." to stderr
# Matches ocloc_tests
asan_runtime_lib = getattr(config, 'asan_runtime_lib', '') if 'system-windows' not in config.available_features else ''

tester_path = lit.util.which('IGCOCLInterfaceTester', config.interface_tester_dir)
if asan_runtime_lib and tester_path:
    tester_tool = ToolSubst('IGCOCLInterfaceTester', command='env',
                            extra_args=['LD_PRELOAD={}'.format(asan_runtime_lib), tester_path],
                            unresolved='break')
else:
    tester_tool = ToolSubst('IGCOCLInterfaceTester')

# add substitutions for tools used by the tests
llvm_config.add_tool_substitutions([
    ToolSubst('not'),
    tester_tool],
    tool_dirs)

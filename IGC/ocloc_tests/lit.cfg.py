# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import subprocess
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


tool_dirs = [config.ocloc_dir, config.llvm_tools_dir, config.spirv_as_dir, config.llvm_spirv_dir]

if llvm_config.add_tool_substitutions([ToolSubst('ocloc', command=FindTool(config.ocloc_name), unresolved='break')], tool_dirs) :
  ocloc_path = llvm_config.config.substitutions[-1][1]
else :
  lit_config.note('Did not find %s in %s, ocloc will be used from system paths' % (config.ocloc_name, tool_dirs))
  ocloc_path = 'ocloc'

llvm_config.add_tool_substitutions([ToolSubst('llvm-dwarfdump')], tool_dirs)
llvm_config.add_tool_substitutions([ToolSubst('llvm-as')], tool_dirs)
llvm_config.add_tool_substitutions([ToolSubst('not')], tool_dirs)
llvm_config.add_tool_substitutions([ToolSubst('split-file')], tool_dirs)


def get_available_devices(tool_path, ld_path):
    set_of_devices = set()
    # ocloc executable is expected to be present for these tests
    ocloc_cmd = subprocess.Popen(
        [tool_path, 'compile', '--help'], stdout=subprocess.PIPE, env={'LD_LIBRARY_PATH': ld_path})
    ocloc_out = ocloc_cmd.stdout.read().decode()
    ocloc_cmd.wait()
    for line in ocloc_out.split('\n') :
        if '<device_type> can be:' in line :
            fields = line.strip().split(':')
            for dev in fields[1].split(','):
               acronym = dev.strip()
               if acronym and not 'ip version' in acronym :
                  set_of_devices.add(acronym + '-supported')
    return set_of_devices

# Get supported acronyms for ocloc
devices = get_available_devices(ocloc_path, config.environment['LD_LIBRARY_PATH'])
lit_config.note('Supported device acronyms(to be used with REQUIRES:): \n%s' % ' '.join(sorted(devices)))
config.available_features.update(devices)

if not config.regkeys_disabled:
  config.available_features.add('regkeys')

if config.has_vc != "1":
  config.excludes = ['VC']

if config.spirv_as_enabled:
  config.available_features.add('spirv-as')
  llvm_config.add_tool_substitutions([ToolSubst('spirv-as', unresolved='fatal')], tool_dirs)

if int(config.llvm_version_major) <= 15:
  config.available_features.add('llvm-15-or-older')

if int(config.llvm_version_major) >= 15:
  config.available_features.add('llvm-15-plus')

if int(config.llvm_version_major) >= 16:
  config.available_features.add('llvm-16-plus')

if int(config.llvm_version_major) >= 17:
  config.available_features.add('llvm-17-plus')

if config.llvm_spirv_enabled:
  config.available_features.add('llvm-spirv')
  llvm_config.add_tool_substitutions([ToolSubst('llvm-spirv', unresolved='fatal')], tool_dirs)

if config.is32b == "1":
  config.available_features.add('sys32')

if config.debug_build:
  config.available_features.add('debug')


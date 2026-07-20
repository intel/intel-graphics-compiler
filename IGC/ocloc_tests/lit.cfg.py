# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# -*- Python -*-

import os
import subprocess
import sys
import lit.formats
import lit.util

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst

if config.igc_lit_common_dir:
    sys.path.append(config.igc_lit_common_dir)
    from igc_lit_helpers import VerboseUnsupportedShTest
else:
    VerboseUnsupportedShTest = lit.formats.ShTest

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = 'OfflineCompilationTests'

# testFormat: The test format to use to interpret tests.
config.test_format = VerboseUnsupportedShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.cl', '.ll', '.spvasm', '.spv-test']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: where tests run. Overridable so a re-run pass
# (e.g. check-ocloc-igc-clang) writes to its own dir.
test_output_subdir = lit_config.params.get('test_output_subdir', 'test_output')
config.test_exec_root = os.path.join(config.test_run_dir, test_output_subdir)

shared_library_path_env = 'PATH' if 'system-windows' in config.available_features else 'LD_LIBRARY_PATH'

llvm_config.use_default_substitutions()

lib_dirs = [config.ocloc_lib_dir, config.igc_lib_dir, config.cclang_lib_dir]
# Optional igc-clang prebuild directory, used by the IGC_LibClangOverride tests.
igc_clang_lib_dir = getattr(config, 'igc_clang_lib_dir', '')
if igc_clang_lib_dir:
    lib_dirs.append(igc_clang_lib_dir)

llvm_config.with_environment(shared_library_path_env, lib_dirs, append_path=True)

# For check-ocloc-igc-clang - force FCL to load igc-clang via
# IGC_LibClangOverride env variable. Default = clear the flag.
igc_libclang_override = lit_config.params.get('igc_libclang_override', '')
if igc_libclang_override:
    llvm_config.with_environment('IGC_LibClangOverride', igc_libclang_override)
    config.name += '-igc-clang'
    # Tests known to fail under the igc-clang library are tagged
    # with "UNSUPPORTED: lib-igc-clang".
    config.available_features.add('lib-igc-clang')


tool_dirs = [config.ocloc_dir, config.llvm_tools_dir]

asan_runtime_lib = getattr(config, 'asan_runtime_lib', '') if 'system-windows' not in config.available_features else ''

ocloc_path = lit.util.which(config.ocloc_name, os.pathsep.join(tool_dirs))
if ocloc_path is None:
  lit_config.note('Did not find %s in %s, ocloc will be used from system paths' % (config.ocloc_name, tool_dirs))
  ocloc_path = 'ocloc'

if asan_runtime_lib:
  config.environment['NEO_OCLOC_DisableDeepBind'] = '1'
  ocloc_tool = ToolSubst('ocloc', command='env', extra_args=['LD_PRELOAD={}'.format(asan_runtime_lib), ocloc_path], unresolved='break')
else:
  ocloc_tool = ToolSubst('ocloc', command=ocloc_path, unresolved='break')

llvm_config.add_tool_substitutions([
  ocloc_tool,
  ToolSubst('llvm-dwarfdump'),
  ToolSubst('llvm-as'),
  ToolSubst('not'),
  ToolSubst('split-file'),
], tool_dirs)


def _get_ocloc_output(*args):
  env = dict(config.environment)
  if asan_runtime_lib:
    env['LD_PRELOAD'] = asan_runtime_lib
  ocloc_cmd = subprocess.Popen([ocloc_path, *args], stdout=subprocess.PIPE, env=env)
  ocloc_out = ocloc_cmd.stdout.read().decode()
  ocloc_cmd.wait()
  return ocloc_out


def get_available_devices():
    set_of_devices = set()
    # ocloc executable is expected to be present for these tests
    for line in _get_ocloc_output('compile', '--help').split('\n') :
        if '<device_type> can be:' in line :
            fields = line.strip().split(':')
            for dev in fields[1].split(','):
               acronym = dev.strip()
               if acronym and not 'ip version' in acronym :
                  set_of_devices.add(acronym + '-supported')
    return set_of_devices

# Get supported acronyms for ocloc
devices = get_available_devices()
lit_config.note('Supported device acronyms(to be used with REQUIRES:): \n%s' % ' '.join(sorted(devices)))
config.available_features.update(devices)

if not config.regkeys_disabled:
  config.available_features.add('regkeys')

if config.has_vc != "1":
  config.excludes = ['VC']

llvm_ver = int(config.llvm_version_major)

if config.spirv_as_enabled:
  config.available_features.add('spirv-as')
  llvm_config.add_tool_substitutions([ToolSubst('spirv-as', unresolved='fatal')], config.spirv_as_dir)

if llvm_ver <= 15:
  config.available_features.add('llvm-15-or-older')

if llvm_ver >= 15:
  config.available_features.add('llvm-15-plus')

if llvm_ver >= 16 and config.igc_llvm_opaque_pointers == '1':
  config.available_features.add('llvm-16-plus')

if llvm_ver >= 17:
  config.available_features.add('llvm-17-plus')

if llvm_ver >= 22:
  config.available_features.add('llvm-22-plus')

# On LLVM 17 tools like llvm-as do not have "opaque-pointers" flag, so in order to keep tests working on all LLVMs
# on 17 tools we just provide empty string
if llvm_ver >= 17:
  config.substitutions.append(('%OPAQUE_PTR_FLAG%', ''))
  config.substitutions.append(('%TYPED_PTR_FLAG%', ''))
else:
  config.substitutions.append(('%OPAQUE_PTR_FLAG%', '-opaque-pointers=1'))
  config.substitutions.append(('%TYPED_PTR_FLAG%', '-opaque-pointers=0'))

config.substitutions.append(('%LLVM_DEPENDENT_CHECK_PREFIX%', f'CHECK-LLVM-{config.llvm_version_major}'))

if config.llvm_spirv_enabled:
  config.available_features.add('llvm-spirv')
  llvm_config.add_tool_substitutions([ToolSubst('llvm-spirv', unresolved='fatal')], config.llvm_spirv_dir)

if config.is32b == "1":
  config.available_features.add('sys32')

if config.debug_build:
  config.available_features.add('debug')

if getattr(config, 'release_build', False):
  config.available_features.add('release')


# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import argparse
import os
from functools import total_ordering
from subprocess import check_call

parser = argparse.ArgumentParser(description='Apply LLVM patches for IGC')
parser.add_argument('--llvm-version', required=True,
                    help='LLVM version for patching')
parser.add_argument('--llvm-project-dir', required=True,
                    help='LLVM project copied sources')
parser.add_argument('--patches-dir', required=True,
                    help='Directory with patches')
parser.add_argument('--patch-executable', required=True,
                    help='Path to patch program')
parser.add_argument('--patch-disable', required=True,
                    help='Patch to disable')
parser.add_argument('--dry-run', action='store_true',
                    help='Only print list of patches that will be applied')

args = parser.parse_args()

def get_dir_patches(ver_dir):
    """Return list of patches from given version directory.

    Collect patches from 'patches' subdirectory and return them
    as a list of DirEntry objects.
    """
    patches_dir = os.path.join(args.patches_dir, ver_dir)
    patches = []
    ext_patches = os.path.join(patches_dir, 'patches_external')
    if os.path.exists(ext_patches):
        patches.extend(os.scandir(ext_patches))
    return patches

def apply_patch(patch_path):
    """Apply patch to llvm project."""
    rel_path = os.path.relpath(patch_path.path, args.patches_dir)
    print('Applying {} file'.format(rel_path))
    if args.dry_run:
        return
    check_call([args.patch_executable,
                '-t', '-s', '-N',
                '-d', args.llvm_project_dir,
                '-p1', '-i', patch_path])

def split_ver_str(ver_str):
    """Split version string into numeric components.

    Return list of components as numbers additionally checking
    that all components are correct (i.e. can be converted to numbers).
    """
    ver_list = []
    for c in ver_str.split('.')[0:3]:
        if not c.isdecimal():
            raise RuntimeError("Malformed version string '{}'. "
                               "Component '{}' is not an integer."
                               .format(ver_str, c))
        ver_list.append(int(c))

    return ver_list

def get_ver_component(ver_list, idx):
    """Get version component from components list.

    Return 0 for components out of range as default.
    """
    if idx < len(ver_list):
        return ver_list[idx]
    return 0

@total_ordering
class Version:
    """Simple wrapper around three-component version.

    This class provides convenient accessors to version components
    represented by decimal numbers. Suitable for LLVM version in IGC.
    """

    def __init__(self, ver_str):
        ver_list = split_ver_str(ver_str)
        ver_tuple = tuple(get_ver_component(ver_list, idx) for idx in (0, 1, 2))
        self.major = ver_tuple[0]
        self.minor = ver_tuple[1]
        self.patch = ver_tuple[2]

    def as_tuple(self):
        return (self.major, self.minor, self.patch)

    def __repr__(self):
        return '{}.{}.{}'.format(self.major, self.minor, self.patch)

    def __eq__(self, other):
        return self.as_tuple() == other.as_tuple()

    def __lt__(self, other):
        return self.as_tuple() < other.as_tuple()

required_ver = Version(args.llvm_version)

# Create mapping of version to directory with suitable
# version range sorted in descending order.
# All directories with same major version as required are suitable.
patches_dir = os.listdir(args.patches_dir)
ver_to_dir = [(Version(v), v) for v in patches_dir]
ver_to_dir = filter(lambda tpl: tpl[0].major == required_ver.major, ver_to_dir)
ver_to_dir = filter(lambda tpl: tpl[0] <= required_ver, ver_to_dir)
ver_to_dir = sorted(ver_to_dir, key=lambda tpl: tpl[0], reverse=True)

# Merge patches from suitable directories taking only patches from
# newest versions if same patch is present in several directries.
patches = {}
for _, d in ver_to_dir:
    dir_patches = get_dir_patches(d)
    dir_patches = {d.name : d for d in dir_patches}
    dir_patches.update(patches)
    patches = dir_patches
patches = list(patches.values())
patches.sort(key=lambda p: p.name)

checkDisabledPatch = False
if args.patch_disable != "None":
    checkDisabledPatch = True

for patch in patches:
    if checkDisabledPatch:
        if args.patch_disable in patch.name:
            continue
    apply_patch(patch)

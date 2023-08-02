# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import sys
import argparse
from Intrinsic_utils import file_path, dir_path
from Intrinsic_definition_translation import generate_type_definitions_from_modules
from Intrinsic_generator import generate_intrinsic_defintion_files

if __name__ == '__main__':
    def main(args):
        parser = argparse.ArgumentParser(description='Generates IGC intrinsic files.')
        parser.add_argument("inputs", nargs='+', help="the source path to the file with intrinsic defintions (current IGC format)",
                        type=file_path)
        parser.add_argument("--use_comments", action='store_true')
        parser.add_argument("--output", help="the directory for the files with intrinsic definitions",
                        type=dir_path)

        args = parser.parse_args(args[1:])

        intrinsic_definitions = generate_type_definitions_from_modules(args.inputs)
        generate_intrinsic_defintion_files(intrinsic_definitions, args.output, args.use_comments)

    main(sys.argv)
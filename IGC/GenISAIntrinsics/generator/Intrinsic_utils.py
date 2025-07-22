# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import argparse
import re
import subprocess
import traceback
import sys

from pathlib import Path
from mako.template import Template
from mako.lookup import TemplateLookup

def is_windows():
    return hasattr(sys, 'getwindowsversion')

def safe_open(path, mode = 'w'):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    return open(path, mode)

def remove_all_whitespace_at_line_end(text):
    """Removes all whitespace at the ends of lines
    Args:
        text:   An input text.
    Returns:
        The formatted text without trailing whitespaces at the ends of lines"""
    text = re.sub("[ \t]+(\r?)\n", '\g<1>\n', text, flags=re.MULTILINE)
    return text

def set_windows_line_ends(text):
    """Removes all whitespace at the ends of lines
    Args:
        text:   An input text.
    Returns:
        The formatted text without trailing whitespaces at the ends of lines"""
    text = re.sub("(?<!\r)\n", "\r\n", text, flags=re.MULTILINE)
    return text

def write_to_file_using_template(file_path, template, use_clang_format=True, **kwargs):
    """Gets the formatted text with intendation which corresponds to 4 spaces.
    Args:
        file_path:   output path,
        template:    recipe for a new file,
        kwargs:      arguments for template."""
    try:
        print(" --- Generating %s" % os.path.basename(file_path))
        content = template.render(file_name=os.path.basename(file_path), **kwargs)
        content = remove_all_whitespace_at_line_end(content)
        content = set_windows_line_ends(content)
        if use_clang_format:
            content = format_with_clang_format(content, style_path())
        if os.path.isfile(file_path):
            os.chmod(file_path, 0o666)
        with safe_open(file_path, 'wb') as f:
            f.write(content.encode())
        print(" --- Successfully generated in the following location: \n    %s"%(os.path.abspath(file_path)))
        os.chmod(file_path, 0o444)
    except IOError as e:
        print(" --- The file is not generated in the following location: \n    %s"%(os.path.abspath(file_path)))
        print("I/O error: %s" % e.strerror)
    except AssertionError as e:
        print(" --- The file is not generated in the following location: \n    %s"%(os.path.abspath(file_path)))
        print(e)
        print(traceback.format_exc())
    except Exception as e:
        print(" --- The file is not generated in the following location: \n    %s"%(os.path.abspath(file_path)))
        print(e.text_error_template().render())
        print(traceback.format_exc())

def style_path():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    return os.path.join(root_dir, '.clang-format')

def clang_fmt_path():
    if "CLANG_FORMAT_PATH" in os.environ:
        return os.environ["CLANG_FORMAT_PATH"]
    return "clang-format"

def format_with_clang_format(content, style_file=""):
    try:
        if style_file:
            assert(os.path.isfile(style_file))
            style = f"--style=file:{os.path.abspath(style_file)}"
        else:
            style = "--style=llvm"
        proc = subprocess.run(
            [clang_fmt_path(), style],
            input=content.encode(),
            stdout=subprocess.PIPE,
            check=True
        )
        result = proc.stdout.decode()
        return result
    except FileNotFoundError as e:
        print(" --- clang-format failed")
        print("Executable not found:  %s" % e)
    return content

def from_template_name_to_destination_name(template_filename : str):
    return Path(template_filename).name.replace(".mako", "")

def file_path(path):
    if os.path.isfile(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"{path} is not a valid path to file")

def dir_path(path):
    if os.path.isdir(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"{path} is not a valid path to dir")

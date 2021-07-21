# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# import argparse
# import os
# def concat(file1,file2,new_file):
#     f1,f2 = "",""
#     if os.path.exists(file1):
#         f1 = open(file1).read()
#     if os.path.exists(file2):
#         f2 = open(file2).read()
#     with open(new_file,"wb") as f_concat:
#         f_concat.write(f1+f2)
#         f_concat.write("\0")
# parser = argparse.ArgumentParser()
# parser.add_argument('-top', help='top file for concat')
# parser.add_argument('-bottom', help='bottom file for concat')
# parser.add_argument('-new', help='new file for concat')
# args = parser.parse_args()
# concat(args.top,args.bottom,args.new)

import os
import sys

def Usage () :
  print (r'Usage: concat.py -new <outfile> <f0> <f1> ...')
  print (r'  To concatenate files in order into a new file <outfile>.')
  print (r'  Input files are <f0> <f1>, and they are at least two.')

if len(sys.argv) < 5 :
  Usage()
  sys.exit("Error: not enough argments to concat.py")

if sys.argv[1] != '-new':
  Usage()
  sys.exit("Error: concat.py - the 1st argument must be -new <outfile>")

outfilename = sys.argv[2]
infilenames = sys.argv[3:]

with open(outfilename, 'wb') as outfile:
  for fname in infilenames:
    if not os.path.exists(fname):
      sys.exit("Error: no such file: " + fname)
    with open(fname, "rb") as infile:
      outfile.write(infile.read())
  outfile.write(b"\0")

#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================

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

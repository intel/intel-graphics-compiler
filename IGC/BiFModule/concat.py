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

import argparse
import os
def concat(file1,file2,new_file):
	f1,f2 = "",""
	if os.path.exists(file1):
		f1 = open(file1, "br").read()
	if os.path.exists(file2):
		f2 = open(file2, "br").read()
	with open(new_file,"wb") as f_concat:
		f_concat.write(f1+f2)
		f_concat.write(b"\0")
parser = argparse.ArgumentParser()
parser.add_argument('-top', help='top file for concat')
parser.add_argument('-bottom', help='bottom file for concat')
parser.add_argument('-new', help='new file for concat')
args = parser.parse_args()
concat(args.top,args.bottom,args.new)

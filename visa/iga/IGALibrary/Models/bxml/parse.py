#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

file1 = open('Model12P5.hpp', 'r')
Lines = file1.readlines()

new_file = open('Model12P5_new.hpp', 'w')

count = 0
# read old lines
for line in Lines:
    if "ENCODED(" not in line:
        new_file.writelines(line)
    else:
        # remove everything after "ENCODED("
        en_idx = line.find('ENCODED(')
        nstr = line[:en_idx-1]
        # remove '/' and '*' and ' '
        nstr = nstr.replace('/','')
        nstr = nstr.replace('*','')
        #nstr = nstr.replace(' ','')
        # count number of spaces
        n_spaces = 0
        for i in range(0, len(nstr)):
            if nstr[i] != ' ':
                # find first non-space, break
                break
            else:
                n_spaces = n_spaces + 1
        # replace last space before string to "
        nstr = nstr[:n_spaces-1] + "\"" + nstr[n_spaces:]
        # replace last space in string to "
        nstr = nstr[:len(nstr)-1] + "\""

        # add ',' at the end
        nstr = nstr + ",\n"
        new_file.writelines(nstr)




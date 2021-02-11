#!/usr/bin/env python

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

import os
import sys
import errno
from typing import List, NamedTuple

class DeclHeader(NamedTuple):
    # line contains the entire string of the line the decl was found on
    line: str
    # declName is just the identifier name
    declName: str

# usage: autogen.py <path_to_MDFrameWork.h> <path_to_MDNodeFuncs.gen>
__MDFrameWorkFile__ = sys.argv[1]
__genFile__         = sys.argv[2]

__genDir__ = os.path.dirname(__genFile__)
if not os.path.exists(__genDir__):
    try:
        os.makedirs(__genDir__)
    except OSError as err:
        # In case of a race to create this directory...
        if err.errno != errno.EEXIST:
            raise

output = open(__genFile__, 'w')
inputFile = open(__MDFrameWorkFile__, 'r')

enumNames: List[DeclHeader] = []
structureNames: List[DeclHeader] = []
structDataMembers = []

with inputFile as file:
    insideIGCNameSpace = False
    for line in file:
        line = line.split("//")[0]
        if line.find("namespace IGC") != -1:
            line = line.split("//")[0]
            while line.find("{") == -1:
                line = next(file, None)
            insideIGCNameSpace = True
        if insideIGCNameSpace:
            line = line.split("//")[0]
            if line.find("struct") != -1:
                words = line.split()
                structureNames.append(DeclHeader(line, words[1]))
                while line.find("};") == -1:
                    line = next(file, None)
                    line = line.split("//")[0]
                line = next(file, None)
            if line.find("enum") != -1:
                words = line.split()
                nameIdx = 2 if 'class' in words else 1
                enumNames.append(DeclHeader(line, words[nameIdx]))
                while line.find("};") == -1:
                    line = next(file, None)
                    line = line.split("//")[0]
                line = next(file, None)
                line = line.split("//")[0]
            if line.find("};") != -1:
                insideIGCNameSpace = False

def skipLine(line):
    return False

def storeVars(line):
    vars = line.split()
    for i in range(1, len(vars)):
        res = vars[i].split(",")
        for j in range(0, len(res)):
            if(res[j] != '' and res[j]!= ';' and res[j].find("ModuleMetaData") == -1 and res[j].find("FunctionMetaData") == -1):
                if(i != len(vars)-1):
                    structDataMembers.append(res[j] + ';')
                else:
                    if(j != len(res)-1):
                        structDataMembers.append(res[j] + ";")
                    else:
                        structDataMembers.append(res[j])


def extractVars(line):
    vars = line.split()
    if(line.find("std::vector") != -1 or line.find("std::map") != -1 or line.find("std::array") != -1):
        structDataMembers.append(vars[len(vars)-1])
        return
    if line.find("=") != -1:
        structDataMembers.append(vars[vars.index("=")-1]+";")
    elif(line.find(",")) == -1:
        if len(vars) == 2:
            structDataMembers.append(vars[1])
        else:
            if(len(vars) == 3):
                structDataMembers.append(vars[len(vars)-1])
    else:
        storeVars(line)

def extractEnumVal(line):
    vars = line.split()
    if (len(vars) == 0) or (line.find("{") != -1):
        return

    val = vars[0]
    if val[-1] == ',':
        val = val[:-1]

    structDataMembers.append(val)

def printCalls(structName):
    output.write("    Metadata* v[] = \n")
    output.write("    { \n")
    output.write("        MDString::get(module->getContext(), name),\n")
    for item in structDataMembers:
        item = item[:-1]
        p =  '"{}"'.format(item)
        output.write("        CreateNode(" + structName + "Var" + "." + item + ", module, IGC_MANGLE("+ p + ")),\n")
    output.write("    };\n")
    output.write("    MDNode* node = MDNode::get(module->getContext(), v);\n")
    output.write("    return node;\n")


def printEnumCalls(EnumName):
    output.write("    StringRef enumName;\n")
    output.write("    switch("+ EnumName+ "Var)\n")
    output.write("    {\n")
    for item in structDataMembers:
        output.write("        case IGC::" + EnumName + "::" + item + ":\n"  )
        output.write("            enumName = IGC_MANGLE(\"" + item + "\");\n" )
        output.write("            break;\n" )
    output.write("    }\n")
    output.write("    Metadata* v[] = \n")
    output.write("    { \n")
    output.write("        MDString::get(module->getContext(), name),\n")
    output.write("        MDString::get(module->getContext(), enumName),\n")
    output.write("    };\n")
    output.write("    MDNode* node = MDNode::get(module->getContext(), v);\n")
    output.write("    return node;\n")


def printReadCalls(structName):
     for item in structDataMembers:
        item = item[:-1]
        p =  '"{}"'.format(item)
        output.write("    readNode(" + structName + "Var" + "." + item + ", node , IGC_MANGLE(" + p + "));\n")

def printEnumReadCalls(enumName):
    output.write("    StringRef s = cast<MDString>(node->getOperand(1))->getString();\n")
    output.write("    std::string str = s.str();\n")
    output.write("    "+ enumName + "Var = (IGC::" + enumName + ")(0);\n")

    for item in structDataMembers:
        output.write("    if((str.size() == sizeof(\""+ item + "\")-1) && (::memcmp(str.c_str(),IGC_MANGLE(\""+ item + "\"),str.size())==0))\n")
        output.write("    {\n")
        output.write("            "+ enumName + "Var = IGC::" + enumName + "::" + item + ";\n")
        output.write("    } else\n")

    output.write("    {\n")
    output.write("            "+ enumName + "Var = (IGC::" + enumName + ")(0);\n")
    output.write("    }\n")

def emitCodeBlock(names: List[DeclHeader], declType, fmtFn, extractFn, printFn):
    for item in names:
        foundStruct = False
        insideStruct = False
        findStructDecl = item.line
        inputFile = open(__MDFrameWorkFile__, 'r')
        with inputFile as file:
            for line in file:
                line = line.split("//")[0]
                if line.find(findStructDecl) != -1:
                    foundStruct = True
                    output.write(fmtFn(item.declName))
                    output.write("{\n")
                if foundStruct:
                    line = line.split("//")[0]
                    while line.find("{") == -1:
                        line = next(file, None)
                        line = line.split("//")[0]
                    insideStruct = True
                if insideStruct:
                    line = line.split("//")[0]
                    if line.find("};") != -1:
                        inputFile.close()
                        continue
                    else:
                        if not skipLine(line):
                            line = line.split("//")[0]
                            while line.find("};") == -1:
                                extractFn(line)
                                line = next(file, None)
                                while skipLine(line):
                                    line = next(file, None)
                                line = line.split("//")[0]
                            printFn(item.declName)
                            del structDataMembers[:]
                            foundStruct = False
                            insideStruct = False
        output.write("}\n\n")

def emitEnumCreateNode():
    def fmtFn(item):
        return f"MDNode* CreateNode(IGC::{item} {item}Var, Module* module, StringRef name)\n"
    emitCodeBlock(enumNames, "enum", fmtFn, extractEnumVal, printEnumCalls)

def emitStructCreateNode():
    def fmtFn(item):
        return "MDNode* CreateNode(const IGC::" + item + "& " + item + "Var" +", Module* module, StringRef name)\n"
    emitCodeBlock(structureNames, "struct", fmtFn, extractVars, printCalls)

def emitEnumReadNode():
    def fmtFn(item):
        return "void readNode( IGC::" + item + " &" + item + "Var," + " MDNode* node)\n"
    emitCodeBlock(enumNames, "enum", fmtFn, extractEnumVal, printEnumReadCalls)

def emitStructReadNode():
    def fmtFn(item):
        return "void readNode( IGC::" + item + " &" + item + "Var," + " MDNode* node)\n"
    emitCodeBlock(structureNames, "struct", fmtFn, extractVars, printReadCalls)

def genCode():
    emitEnumCreateNode()
    emitStructCreateNode()
    emitEnumReadNode()
    emitStructReadNode()

if __name__ == '__main__':
    genCode()

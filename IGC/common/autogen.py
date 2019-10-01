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

enumNames = []
structureNames = []
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
                structureNames.append(words[1])
                while line.find("};") == -1:
                    line = next(file, None)
                    line = line.split("//")[0]
                line = next(file, None)
            if line.find("enum") != -1:
                words = line.split()
                enumNames.append(words[1])
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
        output.write("        CreateNode(" + structName + "Var" + "." + item + ", module, "+ p + "),\n")
    output.write("    };\n")
    output.write("    MDNode* node = MDNode::get(module->getContext(), v);\n")
    output.write("    return node;\n")


def printEnumCalls(EnumName):
    output.write("    StringRef enumName;\n")
    output.write("    switch("+ EnumName+ "Var)\n")
    output.write("    {\n")
    for item in structDataMembers:
        output.write("        case IGC::"+ item + ":\n"  )
        output.write("            enumName = \"" + item + "\";\n" )
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
        output.write("    readNode(" + structName + "Var" + "." + item + ", node ," + p + ");\n")

def printEnumReadCalls(enumName):
    output.write("    StringRef s = cast<MDString>(node->getOperand(1))->getString();\n")
    output.write("    "+ enumName + "Var = StringSwitch<IGC::" + enumName + ">(s)\n")
    for item in structDataMembers:
        output.write("        .Case(\""+ item + "\", IGC::"+ item + ")\n")
    output.write("        .Default((IGC::" + enumName + ")(0));\n")

def emitCodeBlock(names, declType, fmtFn, extractFn, printFn):
    for item in names:
        foundStruct = False
        insideStruct = False
        findStructDecl = "{} {}".format(declType, item)
        inputFile = open(__MDFrameWorkFile__, 'r')
        with inputFile as file:
            for line in file:
                line = line.split("//")[0]
                if line.find(findStructDecl) != -1:
                    foundStruct = True
                    output.write(fmtFn(item))
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
                            printFn(item)
                            del structDataMembers[:]
                            foundStruct = False
                            insideStruct = False
        output.write("}\n\n")

def emitEnumCreateNode():
    def fmtFn(item):
        return "MDNode* CreateNode(IGC::" + item + " " + item + "Var" +", Module* module, StringRef name)\n"
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

genCode()

# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import sys
import errno
import re
from typing import List


class DeclHeader:
    # line contains the entire string of the line the decl was found on
    line: str
    # declName is just the identifier name
    declName: str
    # fields contains a list of all the names of the fields in the structure
    fields: List[str]

    def __init__(self, line_p, declName_p, fields_p):
        self.line = line_p
        self.declName = declName_p
        self.fields = fields_p

enumNames: List[DeclHeader] = []
structureNames: List[DeclHeader] = []


def parseCmdArgs():
    if (len(sys.argv) != 3):
        sys.exit("usage: autogen.py <path_to_MDFrameWork.h> <path_to_MDNodeFuncs.gen>")

    # usage: autogen.py <path_to_MDFrameWork.h> <path_to_MDNodeFuncs.gen>
    __MDFrameWorkFile__ = sys.argv[1]
    __genFile__         = sys.argv[2]

    if not os.path.isfile(__MDFrameWorkFile__):
        sys.exit("Could not find the file " + __MDFrameWorkFile__)

    __genDir__ = os.path.dirname(__genFile__)
    # Try to create the path to MDNodeFuncs.gen if it doesn't already exist.
    if not os.path.exists(__genDir__):
        try:
            os.makedirs(__genDir__)
        except OSError as err:
            # In case of a failure to create this directory, and the directory doesn't already exist.
            if err.errno != errno.EEXIST:
                sys.exit("Failed to create the directory " + __genDir__)

    return __MDFrameWorkFile__ , __genFile__


def storeVars(line, declHeader):
    vars = line.split()
    for i in range(1, len(vars)):
        res = vars[i].split(",")
        for j in range(0, len(res)):
            if(res[j] != '' and res[j]!= ';' and res[j].find("ModuleMetaData") == -1 and res[j].find("FunctionMetaData") == -1):
                if(i != len(vars)-1):
                    declHeader.fields.append(res[j] + ';')
                else:
                    if(j != len(res)-1):
                        declHeader.fields.append(res[j] + ";")
                    else:
                        declHeader.fields.append(res[j])


def extractVars(line, declHeader):
    vars = line.split()
    if any(ty in line for ty in ("std::vector", "std::map", "std::array", "MapVector")):
        declHeader.fields.append(vars[len(vars)-1])
        return
    if line.find("=") != -1:
        declHeader.fields.append(vars[vars.index("=")-1]+";")
    elif(line.find(",")) == -1:
        if len(vars) == 2:
            declHeader.fields.append(vars[1])
        else:
            if(len(vars) == 3):
                declHeader.fields.append(vars[len(vars)-1])
    else:
        storeVars(line, declHeader)


def extractEnumVal(line, declHeader):
    vars = line.split()
    if (len(vars) == 0) or (line.find("{") != -1):
        return

    val = vars[0]
    if val[-1] == ',':
        val = val[:-1]

    declHeader.fields.append(val)


def parseFile(fileName, insideIGCNameSpace):
    inputFile = None
    try:
        inputFile = open(fileName, 'r')
    except:
        sys.exit("Failed to open the file " + fileName)

    # with statement automatically closes the file
    with inputFile as file:
        pcount = 0
        for line in file:
            line = line.split("//")[0]

            if line.find("namespace IGC") != -1:
                while line.find("{") == -1:
                    line = next(file, None)
                insideIGCNameSpace = True
                pcount += 1
                #
            if insideIGCNameSpace:
                blockType = re.search("struct|enum", line)
                if blockType:
                    words = line.split()
                    idx = 2 if 'class' in words else 1
                    foundDecl = DeclHeader(line, words[idx], [])
                    opcount = pcount
                    namesList = structureNames
                    extractFunc = extractVars
                    if blockType[0] == 'enum':
                        namesList = enumNames
                        extractFunc = extractEnumVal
                    while True:
                        line = next(file, None)
                        assert line, "EOF reached with unclosed enum or struct, check " + fileName + " formatting"
                        assert re.search("/\*", line) == None, "Multi-line comments are not supported yet"
                        line = line.split("//")[0]
                        pcount += line.count("{") - line.count("}")
                        if pcount <= opcount:
                            break
                        extractFunc(re.sub("{|}","", line), foundDecl)
                    assert pcount == opcount, "Unexpected struct/enum ending, check " + fileName + " formatting"
                    namesList.append(foundDecl)
                elif line.lstrip().startswith("#include") == True:
                    words = line.split()
                    parent_dir = os.path.dirname(fileName)
                    include_file = words[1][1:-1]  # cut off the "" or <> surrounding the file name
                    include_file_path = os.path.join(parent_dir, include_file)
                    parseFile(include_file_path, True)
                elif line.find("}") != -1 and line.find("};") == -1:
                    insideIGCNameSpace = False
                    pcount -= 1
        assert pcount == 0, "EOF reached, with unclosed IGC namespace, check " + fileName + " formatting"

def printStructCalls(structDecl, outputFile):
    outputFile.write("    Metadata* v[] = \n")
    outputFile.write("    { \n")
    outputFile.write("        MDString::get(module->getContext(), name),\n")
    for item in structDecl.fields:
        item = item[:-1]
        p =  '"{}"'.format(item)
        outputFile.write("        CreateNode(" + structDecl.declName + "Var" + "." + item + ", module, ")
        outputFile.write(p)
        outputFile.write("),\n")
    outputFile.write("    };\n")
    outputFile.write("    MDNode* node = MDNode::get(module->getContext(), v);\n")
    outputFile.write("    return node;\n")


def printEnumCalls(enumDecl, outputFile):
    outputFile.write("    StringRef enumName;\n")
    outputFile.write("    switch("+ enumDecl.declName + "Var)\n")
    outputFile.write("    {\n")
    for item in enumDecl.fields:
        outputFile.write("        case IGC::" + enumDecl.declName + "::" + item + ":\n"  )
        outputFile.write("            enumName = ")
        outputFile.write("\"" + item + "\"")
        outputFile.write(";\n")
        outputFile.write("            break;\n" )
    outputFile.write("    }\n")
    outputFile.write("    Metadata* v[] = \n")
    outputFile.write("    { \n")
    outputFile.write("        MDString::get(module->getContext(), name),\n")
    outputFile.write("        MDString::get(module->getContext(), enumName),\n")
    outputFile.write("    };\n")
    outputFile.write("    MDNode* node = MDNode::get(module->getContext(), v);\n")
    outputFile.write("    return node;\n")


def printStructReadCalls(structDecl, outputFile):
     for item in structDecl.fields:
        item = item[:-1]
        p =  '"{}"'.format(item)
        outputFile.write("    readNode(" + structDecl.declName + "Var" + "." + item + ", node , ")
        outputFile.write(p)
        outputFile.write(");\n")

def printEnumReadCalls(enumDecl, outputFile):
    outputFile.write("    StringRef s = cast<MDString>(node->getOperand(1))->getString();\n")
    outputFile.write("    std::string str = s.str();\n")
    outputFile.write("    "+ enumDecl.declName + "Var = (IGC::" + enumDecl.declName + ")(0);\n")

    for item in enumDecl.fields:
        outputFile.write("    if((str.size() == sizeof(\""+ item + "\")-1) && (::memcmp(str.c_str(),")
        outputFile.write("\"" + item + "\"")
        outputFile.write(",str.size())==0))\n")
        outputFile.write("    {\n")
        outputFile.write("            "+ enumDecl.declName + "Var = IGC::" + enumDecl.declName + "::" + item + ";\n")
        outputFile.write("    } else\n")

    outputFile.write("    {\n")
    outputFile.write("            "+ enumDecl.declName + "Var = (IGC::" + enumDecl.declName + ")(0);\n")
    outputFile.write("    }\n")


def emitCodeBlock(names: List[DeclHeader], declType, fmtFn, printFn, outputFile):
    for item in names:
        outputFile.write(fmtFn(item.declName))
        outputFile.write("{\n")
        printFn(item, outputFile)
        outputFile.write("}\n\n")


def emitEnumCreateNode(outputFile):
    def fmtFn(item):
        return f"MDNode* CreateNode(IGC::{item} {item}Var, Module* module, StringRef name)\n"
    emitCodeBlock(enumNames, "enum", fmtFn, printEnumCalls, outputFile)

def emitStructCreateNode(outputFile):
    def fmtFn(item):
        return "MDNode* CreateNode(const IGC::" + item + "& " + item + "Var" +", Module* module, StringRef name)\n"
    emitCodeBlock(structureNames, "struct", fmtFn, printStructCalls, outputFile)

def emitEnumReadNode(outputFile):
    def fmtFn(item):
        return "void readNode( IGC::" + item + " &" + item + "Var," + " MDNode* node)\n"
    emitCodeBlock(enumNames, "enum", fmtFn, printEnumReadCalls, outputFile)

def emitStructReadNode(outputFile):
    def fmtFn(item):
        return "void readNode( IGC::" + item + " &" + item + "Var," + " MDNode* node)\n"
    emitCodeBlock(structureNames, "struct", fmtFn, printStructReadCalls, outputFile)


def genCode(fileName):
    outputFile = None
    try:
        outputFile = open(fileName, 'w')
    except:
        sys.exit("Failed to open the file " + fileName)

    emitEnumCreateNode(outputFile)
    emitStructCreateNode(outputFile)
    emitEnumReadNode(outputFile)
    emitStructReadNode(outputFile)

    outputFile.close()


if __name__ == '__main__':
    __MDFrameWorkFile__ , __genFile__ = parseCmdArgs()
    parseFile(__MDFrameWorkFile__, False)
    genCode(__genFile__)

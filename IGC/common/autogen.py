# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import sys
import errno
import re
from typing import List, Tuple, TextIO, Callable, Generator

class DeclHeader:
    # line contains the entire string of the line the decl was found on
    line: str
    # declName is just the identifier name
    declName: str
    # declName is just the identifier name
    namespace: str
    # fields contains a list of all the names of the fields in the structure
    fields: List[str]

    def __init__(self, line: str, namespace: str, declName: str, fields: List[str]):
        self.line      = line
        self.namespace = namespace
        self.declName  = declName
        self.fields    = fields

enumNames: List[DeclHeader] = []
structureNames: List[DeclHeader] = []

def parseCmdArgs() -> Tuple[str, str]:
    if (len(sys.argv) != 3):
        sys.exit("usage: autogen.py <path_to_MDFrameWork.h> <path_to_MDNodeFuncs.gen>")

    __MDFrameWorkFile__ = sys.argv[1]
    __genFile__         = sys.argv[2]

    if not os.path.isfile(__MDFrameWorkFile__):
        sys.exit(f"Could not find the file {__MDFrameWorkFile__}")

    __genDir__ = os.path.dirname(__genFile__)
    if not os.path.exists(__genDir__):
        try:
            os.makedirs(__genDir__)
        except OSError as err:
            if err.errno != errno.EEXIST:
                sys.exit(f"Failed to create the directory {__genDir__}")

    return __MDFrameWorkFile__ , __genFile__

def extractStructField(line: str, declHeader: DeclHeader):
    if line.strip() == '':
        return
    vars = line.split()
    if "=" in line:
        declHeader.fields.append(vars[vars.index("=") - 1] + ";")
    else:
        declHeader.fields.append(vars[-1])

def extractEnumVal(line: str, declHeader: DeclHeader):
    vars = line.split()
    if len(vars) == 0 or "{" in line:
        return

    val = vars[0]
    if val[-1] == ',':
        val = val[:-1]

    declHeader.fields.append(val)

def lines(s: str) -> Generator[str, None, None]:
    for line in s.split('\n'):
        yield line

def parseHeader(fileContents: str):
    namespace = ""
    pcount = 0
    file = lines(fileContents)
    for line in file:
        line = line.split("//")[0]
        if "namespace" in line:
            words = line.split()
            while "{" not in line:
                line = next(file, None)
                if line is None:
                    sys.exit('missing opening brace!')
            namespace = words[1] if len(words) > 1 else ""
            pcount += 1
        blockType = re.search("struct|enum", line)
        if blockType:
            words = line.split()
            idx = 2 if blockType[0] == 'enum' and ('class' in words or 'struct' in words) else 1
            foundDecl = DeclHeader(line, namespace, words[idx], [])
            opcount = pcount
            namesList = structureNames
            extractFunc = extractStructField
            if blockType[0] == 'enum':
                namesList = enumNames
                extractFunc = extractEnumVal
            while True:
                line = next(file, None)
                if line is None:
                    sys.exit(f"EOF reached with unclosed enum or struct, check formatting")
                line = line.split("//")[0]
                pcount += line.count("{") - line.count("}")
                if pcount <= opcount:
                    break
                extractFunc(re.sub("{|}","", line), foundDecl)
            assert pcount == opcount, f"Unexpected struct/enum ending, check formatting"
            namesList.append(foundDecl)
        elif "}" in line and "};" not in line:
            insideIGCNameSpace = False
            pcount -= 1
    assert pcount == 0, f"EOF reached, with unclosed namespace, check formatting"

def stripBlockComments(text: str) -> str:
    return re.sub(r'/\*(.|\s)*?\*/', '', text)

def expandIncludes(fileName: str) -> str:
    try:
        file = open(fileName, 'r')
    except:
        sys.exit(f"Failed to open the file {fileName}")

    text = file.read()
    while True:
        # look for includes of the form: #include "myinclude.h" // ^MDFramework^
        includes: List[Tuple[str, str]] = []
        for m in re.finditer(r'#include\s+"(\S+)"\s*//\s*\^MDFramework\^:\s*(\S+)', text):
            include_file = os.path.basename(m.group(1))
            relative_path = m.group(2)
            parent_dir = os.path.dirname(fileName)
            include_file_path = os.path.normpath(
                os.path.join(parent_dir, relative_path, include_file))
            includes.append((m.group(0), include_file_path))

        if len(includes) == 0:
            break

        for (include_string, include_path) in includes:
            try:
                file = open(include_path, 'r')
            except:
                sys.exit(f"Failed to open the file {include_path}")
            include_contents = file.read()
            text = text.replace(include_string, include_contents)

    return text

def printStructCalls(structDecl: DeclHeader, outputFile: TextIO):
    outputFile.write("    Metadata* v[] = \n")
    outputFile.write("    { \n")
    outputFile.write("        MDString::get(module->getContext(), name),\n")
    for item in structDecl.fields:
        item = item[:-1]
        outputFile.write(f"        CreateNode({structDecl.declName}Var.{item}, module, ")
        outputFile.write(f'"{item}"')
        outputFile.write("),\n")
    outputFile.write("    };\n")
    outputFile.write("    return MDNode::get(module->getContext(), v);\n")

def printEnumCalls(enumDecl: DeclHeader, outputFile: TextIO):
    outputFile.write("    StringRef enumName;\n")
    outputFile.write(f"    switch({enumDecl.declName}Var)\n")
    outputFile.write("    {\n")
    for item in enumDecl.fields:
        outputFile.write(f"        case {enumDecl.namespace}::{enumDecl.declName}::{item}:\n")
        outputFile.write("            enumName = ")
        outputFile.write(f'"{item}"')
        outputFile.write(";\n")
        outputFile.write("            break;\n" )
    outputFile.write("    }\n")
    outputFile.write("    Metadata* v[] = \n")
    outputFile.write("    { \n")
    outputFile.write("        MDString::get(module->getContext(), name),\n")
    outputFile.write("        MDString::get(module->getContext(), enumName),\n")
    outputFile.write("    };\n")
    outputFile.write("    return MDNode::get(module->getContext(), v);\n")

def printStructReadCalls(structDecl: DeclHeader, outputFile: TextIO):
     for item in structDecl.fields:
        item = item[:-1]
        outputFile.write(f"    readNode({structDecl.declName}Var.{item}, node, ")
        outputFile.write(f'"{item}"')
        outputFile.write(");\n")

def printEnumReadCalls(enumDecl: DeclHeader, outputFile: TextIO):
    outputFile.write("    StringRef s = cast<MDString>(node->getOperand(1))->getString();\n")

    first = True
    for item in enumDecl.fields:
        outputFile.write(f'    {"" if first else "else "}if(s.compare(')
        outputFile.write(f'"{item}"')
        outputFile.write(") == 0)\n")
        outputFile.write("    {\n")
        outputFile.write(f"        {enumDecl.declName}Var = {enumDecl.namespace}::{enumDecl.declName}::{item};\n")
        outputFile.write("    }\n")
        first = False

    outputFile.write("    else\n")
    outputFile.write("    {\n")
    outputFile.write(f"        {enumDecl.declName}Var = ({enumDecl.namespace}::{enumDecl.declName})(0);\n")
    outputFile.write("    }\n")


def emitCodeBlock(names: List[DeclHeader], fmtFn: Callable[[str, str], str], printFn: Callable[[DeclHeader, TextIO], None], outputFile: TextIO):
    for item in names:
        outputFile.write(fmtFn(item.namespace, item.declName))
        outputFile.write("{\n")
        printFn(item, outputFile)
        outputFile.write("}\n\n")

def emitEnumCreateNode(outputFile: TextIO):
    def fmtFn(namespace:str, item: str):
        return f"MDNode* CreateNode({namespace}::{item} {item}Var, Module* module, StringRef name)\n"
    emitCodeBlock(enumNames, fmtFn, printEnumCalls, outputFile)

def emitStructCreateNode(outputFile: TextIO):
    def fmtFn(namespace:str, item: str):
        return f"MDNode* CreateNode(const {namespace}::{item}& {item}Var, Module* module, StringRef name)\n"
    emitCodeBlock(structureNames, fmtFn, printStructCalls, outputFile)

def emitEnumReadNode(outputFile: TextIO):
    def fmtFn(namespace:str, item: str):
        return f"void readNode({namespace}::{item}& {item}Var, MDNode* node)\n"
    emitCodeBlock(enumNames, fmtFn, printEnumReadCalls, outputFile)

def emitStructReadNode(outputFile: TextIO):
    def fmtFn(namespace:str, item: str):
        return f"void readNode({namespace}::{item}& {item}Var, MDNode* node)\n"
    emitCodeBlock(structureNames, fmtFn, printStructReadCalls, outputFile)

def genCode(fileName: str):
    try:
        outputFile = open(fileName, 'w')
    except:
        sys.exit(f"Failed to open the file {fileName}")

    emitEnumCreateNode(outputFile)
    emitStructCreateNode(outputFile)
    emitEnumReadNode(outputFile)
    emitStructReadNode(outputFile)

if __name__ == '__main__':
    __MDFrameWorkFile__ , __genFile__ = parseCmdArgs()
    expansion = expandIncludes(__MDFrameWorkFile__)
    expansion = stripBlockComments(expansion)
    parseHeader(expansion)
    genCode(__genFile__)

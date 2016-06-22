#!/usr/bin/env python
from __future__ import print_function, division

import re
import argparse
from os.path import abspath, dirname, join as path_join, exists as file_exists
import sys

mydir = abspath(dirname(__file__))
darma_root = dirname(mydir)
oo_macros = path_join(darma_root, "src", "include", "darma", "impl", "oo", "macros.h")
if not file_exists(oo_macros):
    print("Can't find OO macros file {}".format(oo_macros), file=sys.stderr)
    exit(1)

parser = argparse.ArgumentParser(
    description='Generate a "regular" C++ header file from DARMA object oriented'
                ' macros for easier debugging'
)
parser.add_argument('-t', '--tag', metavar='TAG', type=str, nargs=1,
                    help='Generate an expansion of DARMA_OO_DEFINE_TAG for TAG')
parser.add_argument('-c', '--cls', metavar='CLASS', type=str, nargs=1,
                    help='Generate an expansion of DARMA_OO_DECLARE_CLASS for CLS')
parser.add_argument('-o', '--output', metavar='FILE', type=str, nargs=1,
                    help='Path of the file to generate.  Defaults to ootag_<TAG>.generated.h'
                         ' if tag is given, oocls_<CLS>.generated.h if class is given,'
                         ' and oo.generated.h if neither or both are given.')
parser.add_argument('-f', '--folder', metavar='PATH', type=str, nargs=1,
                    help='Folder to put the generated files in.  Defaults to {}'.format(
                        darma_root
                    ))
parser.add_argument('-v', '--verbose', type=bool, help="Be verbose")

args = parser.parse_args()

if not args.output:
    if args.tag and args.cls:
        ofilename = "oo.generated.h"
    elif args.tag:
        ofilename = "ootag_{}.generated.h".format(args.tag[0])
    elif args.cls:
        ofilename = "oocls_{}.generated.h".format(args.cls[0])
    else:
        ofilename = "oo.generated.h"
    ofile = open(path_join(
        args.folder[0] if args.folder else "./",
        ofilename),
        "w+"
    )
else:
    ofilename = args.output[0]
    ofile = open(args.output, "w+")

macros_file = open(oo_macros, "r")
macros_text = macros_file.read()
macros = dict()
macro_args = dict()

for m in re.finditer(r'#define (\w+)\((.+)\)\s+\\\s*\n((?:[^\n]+\n)+)', macros_text, re.M):
    macros[m.group(1)] = m.group(3)
    macro_args[m.group(1)] = [a.strip() for a in m.group(2).split(",")]

#for m, val in macros.items():
#    print(m + "(" + ",".join(macro_args[m]) + ")")

file_text = """
#include <darma.h>
"""
if args.tag:
    for tag in args.tag:
        file_text += "DARMA_OO_DEFINE_TAG(" + tag + ");\n"
if args.cls:
    for cls in args.cls:
        file_text += "DARMA_OO_DECLARE_CLASS(" + cls + ");\n"

if(args.verbose):
    print("Generating file {} macro expansion of:\n{}".format(ofilename, file_text))

m = True
while m:
    to_replace = None
    mstart = mend = 0
    for macro in macros:
        m = re.search(macro + "\(([^\)]+)\)", file_text)
        if m:
            mstart = m.start()
            mend = m.end()
            to_replace = macro
            arg_replacements = [a.strip() for a in m.group(1).split(",")]
            assert(to_replace in macros)
            break
    if to_replace:
        assert(len(macro_args[to_replace]) == len(arg_replacements))
        replacement = " " + macros[to_replace] + " "
        for i, (arg, repl) in enumerate(zip(macro_args[to_replace], arg_replacements)):
            assert(all(arg not in r for r in arg_replacements[:i]))
            replacement = re.sub(r'(\W|\n)' + arg + r'(\W|\n)',
                                 r'\1' + repl + r'\2', replacement)
        replacement.strip()
        file_text = file_text[:mstart] + replacement + file_text[mend:]

file_text = re.sub(r'\\\s*\n', r'\n', file_text)
file_text = re.sub(r'##', r'', file_text)
# Don't do this for now
file_text = re.sub(r'#(?!include)(\w+)', r'"\1"', file_text)

print(file_text, file=ofile)





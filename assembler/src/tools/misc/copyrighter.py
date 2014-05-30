#!/usr/bin/env python

############################################################################
# Copyright (c) 2011-2014 Saint-Petersburg Academic University
# All Rights Reserved
# See file LICENSE for details.
############################################################################


import os
import shutil
import sys

script_comment = [
    '############################################################################',
    '# Copyright (c) 2011-2014 Saint-Petersburg Academic University',
    '# All Rights Reserved',
    '# See file LICENSE for details.',
    '############################################################################', 
    '']

code_comment = [
    '//***************************************************************************',
    '//* Copyright (c) 2011-2014 Saint-Petersburg Academic University',
    '//* All Rights Reserved',
    '//* See file LICENSE for details.',
    '//****************************************************************************',
    '']

def insert_in_script(filename):
    lines = open(filename).readlines()
    if (script_comment[1] + '\n') in lines:
        return

    modified = open(filename, 'w')
    if len(lines) and lines[0].startswith('#!'):
        modified.write(lines[0])
        modified.write('\n')
        lines = lines[1:]
    
    for com_line in script_comment:
        modified.write(com_line + '\n')
    
    for line in lines:
        modified.write(line)

    modified.close() 

def insert_in_code(filename):       
    lines = open(filename).readlines()
    if (code_comment[1] + '\n') in lines:
        return

    modified = open(filename, 'w')
        
    for com_line in code_comment:
        modified.write(com_line + '\n')
    
    for line in lines:
        modified.write(line)

    modified.close()

def visit(arg, dirname, names):
    for name in names:
        path = os.path.join(dirname, name)
        if not os.path.isfile(path):
            continue
        ext = os.path.splitext(name)[1]
        if arg and ext != arg:
            continue
        if (ext in ['.py', '.sh']) or name.lower().startswith('cmake'):
            insert_in_script(path)
        elif ext in ['.hpp', '.cpp', '.h', '.c']:
            insert_in_code(path)

if len(sys.argv) < 2:
    print ("Usage: " + sys.argv[0] + " <src folder> [.ext -- only file with this extension will be modified]")
    sys.exit(1)

start_dir = sys.argv[1]
if not os.path.isdir(start_dir):
    print("Error! " + start_dir + " is not a directory!")
    sys.exit(1)   

arg = None
if len(sys.argv) == 3:
    arg = sys.argv[2]
os.path.walk(start_dir, visit, arg)

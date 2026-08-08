#!/bin/bash
# Example script that uses modules of rpgc separately,
# like running each end as separate processes

set -xe
AST=".temp/ast.txt"
AST_OPT=".temp/ast_opt.txt"
ASM=".temp/nasm.s"
OBJ=".temp/obj.o"
STDLIB_ASM="stdlib.s"
STDLIB_OBJ=".temp/stdlib.o"

./bin/rpgc-frontend $1 -o $AST
./bin/rpgc-middleend $AST -o $AST_OPT
./bin/rpgc-backend -s $AST_OPT -o $ASM 
nasm -f elf64 $ASM -wno-number-overflow -o $OBJ
nasm -f elf64 $STDLIB_ASM -wno-number-overflow -o $STDLIB_OBJ
ld $OBJ $STDLIB_OBJ -o $2

#!/bin/bash
# example of a script that uses rpgc,
# which directly transforms RPGLang into nasm files

set -xe
ASM=".temp/asm.s"
OBJ=".temp/obj.o"
STDLIB_ASM="stdlib.s"
STDLIB_OBJ=".temp/stdlib.o"

./bin/rpgc $1 -o $ASM
nasm -f elf64 $ASM -wno-number-overflow -o $OBJ
nasm -f elf64 $STDLIB_ASM -wno-number-overflow -o $STDLIB_OBJ
ld $OBJ $STDLIB_OBJ -o $2

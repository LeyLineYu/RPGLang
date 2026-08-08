#!/bin/bash
# example of a script that uses rpgc,
# which directly transforms RPGLang into nasm files
# (-s flag means "stop at nasm", so you can call nasm yourself after if you want)

set -xe
ASM=".temp/nasm.s"
OBJ=".temp/obj.o"
STDLIB_ASM="stdlib.s"
STDLIB_OBJ=".temp/stdlib.o"

./bin/rpgc -s $1 -o $ASM
nasm -f elf64 $ASM -wno-number-overflow -o $OBJ
nasm -f elf64 $STDLIB_ASM -wno-number-overflow -o $STDLIB_OBJ
ld $OBJ $STDLIB_OBJ -o $2

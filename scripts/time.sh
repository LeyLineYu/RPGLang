#!/bin/bash
# A really simple script to (roughly) time 
# how long it takes to compile a bunch of test programs
# Mainly used to compare debug and release version of the compiler
# the --nasm flag makes sure we stop at nasm stage and wont call nasm or ld (which adds a heavy time load)
# Recommended use: "time time.sh"

#set -x
COMPILER="./bin/rpgc"
TESTS_DIR="tests/backend"
TARGETS=$(find $TESTS_DIR -type f -name "*.rpg")
#TARGETS=$'tests/backend/01.rpg\ntests/backend/02.rpg\ntests/backend/03.rpg\ntests/backend/04.rpg\ntests/backend/05.rpg\ntests/backend/06.rpg\ntests/backend/07.rpg\ntests/backend/08.rpg\ntests/backend/09.rpg\ntests/backend/10.rpg\ntests/backend/11.rpg\ntests/backend/12.rpg\ntests/backend/13.rpg'
OUTPUT="a.out"

for file in $TARGETS
do
  $COMPILER $file -o "a.out" -D easy -O false --nasm
done

rm $OUTPUT

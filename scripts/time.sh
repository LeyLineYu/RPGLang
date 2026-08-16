#!/bin/bash
# A really simple script to (roughly) time 
# how long it takes to compile a bunch of test programs
# Mainly used to compare debug and release version of the compiler
# Recommended use: "time time.sh"

# set -x
COMPILER="./bin/rpgc"
TESTS_DIR="tests/backend"
TARGETS=$(find $TESTS_DIR -type f -name "*.rpg")
OUTPUT="a.out"

for file in $TARGETS
do
  $COMPILER $file -o "a.out" -D easy -O false
done

rm $OUTPUT

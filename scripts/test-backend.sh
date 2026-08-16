#!/bin/bash
# A script to compile test programs and test them out
# Warning: all the test programs are compiled in easy difficulty (-D easy)
# which means the compiler does not care for the core mechanic of this language: class-specific-keywords

# set -x
COMPILER="./bin/rpgc"
TESTS_DIR="tests/backend"
OUTPUT_DIR=".temp/tests"
TARGETS=$(find $TESTS_DIR -type f -name "*.rpg")

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

failed_count=0
total_count=0
for file in $TARGETS
do
  let "total_count+=1"
  filename_noext="${file%.*}"
  filename_stripped=$(basename -- $filename_noext)
  executable="$OUTPUT_DIR/$filename_stripped"
  output="$executable.txt"
  diff_output="$executable.diff"
  input="$filename_noext.in"
  answer="$filename_noext.ans"

  $COMPILER $file -o $executable -D easy -O false
  
  if [ -e $input ]; then
    ./$executable < $input > $output
  else
    ./$executable >> $output
  fi

  diff -N $answer $output >> $diff_output
  if [ $? -ne 0 ]; then
    let "failed_count+=1"
    >&2 echo "[FAIL] $file"
    >&2 cat $diff_output
  else 
    echo "[ OK ] $file"
  fi
  # remove .diff file if the diff is empty (-s means "size")
  if [ ! -s $diff_output ]; then
    rm $diff_output
  fi
done

if [ $failed_count -eq 0 ]; then
  echo "All tests have run successfully. Cleaning up..."
  rm -r $OUTPUT_DIR
else
  echo "Some tests ($failed_count/$total_count) have failed"
fi

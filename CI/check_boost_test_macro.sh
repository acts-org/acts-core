#!/bin/bash

files="$@"

if [ -z "$files" ]; then
    files=$(find Tests -name "*.hpp" -or -name "*.cpp" -or -name "*.ipp")
fi

test_string="BOOST_TEST("

ec=0
for file in $files; do
  grep -n "$test_string" "$file"
  status=$?
  if [ $status -ne 1 ]; then
    echo "Found occurrences of '$test_string' in '$file'"
    ec=1
  fi
done

exit $ec

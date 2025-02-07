#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

RET=0
ERRORS=0

FILES=$(find  Core/include/ -name "*.hpp" | grep -v "/detail/")
N_FILES=$(echo "$FILES" | wc -l)
echo "Check $N_FILES files"

ITER=0

for file in $(find  Core/include/ -name "*.hpp" | grep -v "/detail/"); do
    ITER=$((ITER+1))
    echo "$(date +%H:%M:%S)    $((100*ITER/N_FILES))%   check $file"
    out=$(printf "#include <${file:13}>\nint main() { return 0; }" | clang++ -std=c++20 -O0 -c -I "Core/include" -I "/usr/include/eigen3" -x c++ - 2>&1)
    if [[ "$?" -ne "0" ]]; then
        echo "------------------------------------"
        echo "$out"
        echo "------------------------------------"
        RET=1
        ERRORS=$((ERRORS+1))
    fi
done

echo "Total errors: $ERRORS"
exit $RET

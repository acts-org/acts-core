#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

ec=0

files="$@"

if [ -z "$files" ]; then
    files=$(find Core Examples Tests Plugins -name "*.hpp")
fi

for file in $files; do
    res=$(grep -e "^[[:space:]]*#pragma once" $file)
    if [[ "$res" != "#pragma once" ]]; then
        ec=1
        echo "'#pragma once' missing in '$file'"
    fi
done

exit $ec

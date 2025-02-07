#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set -e

output_dir=$1
shift
build_dir=$1
shift

mkdir -p $output_dir
output_dir=$(realpath $output_dir)

pushd $build_dir
NINJA_STATUS="[ninja] [%f/%t] " ninja $@ | tee $output_dir/ninja.log
popd

# grep fails if it does not find anything
set +e
rm $output_dir/clang-tidy.log
cat $output_dir/ninja.log | grep -v '\[ninja\]' > $output_dir/clang-tidy.log
set -e

if [ ! -f $output_dir/clang-tidy.log ]; then
  exit 1
fi

exit 0

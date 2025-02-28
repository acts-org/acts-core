#!/bin/bash
set -e
set -u

PYTHON_VERSION=3.13

input=$1
input_abs=$(realpath "$input")
input_rel=$(basename "$input_abs")
dir=$(dirname "$input_abs")
output=requirements.txt

pushd "$dir"
uv pip compile \
  --python-version $PYTHON_VERSION \
  "$input_rel" \
  -o "$output"
popd

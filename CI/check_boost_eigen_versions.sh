#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

## This script to be used only when building our own boost & eigen
## Also, this relies on Tests/DownstreamProject/ShowActsVersion.cpp

set -u
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

SHOWACTSVERSION=$1

BOOSTVER=$(< "$SCRIPT_DIR/../CMakeLists.txt" sed -n 's/^set(_acts_boost_recommended_version \(.*\))$/\1/p')
EIGENVER=$(< "$SCRIPT_DIR/../CMakeLists.txt" sed -n 's/^set(_acts_eigen3_version \(.*\))$/\1/p')

if ! $SHOWACTSVERSION | grep -q "Using Boost version $BOOSTVER"; then
    echo "Boost version mismatch!"
    exit 1
fi

if ! $SHOWACTSVERSION | grep -q "Using Eigen version $EIGENVER"; then
    echo "Eigen version mismatch!"
    exit 1
fi

exit 0

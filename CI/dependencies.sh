#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

function run() {
    set -x
    "$@"
    { set +x;  } 2> /dev/null
}

function set_env {
  key="$1"
  value="$2"

  echo "=> ${key}=${value}"

  if [ -n "${GITHUB_ACTIONS:-}" ]; then
    echo "${key}=${value}" >> $GITHUB_ENV
  else
    export ${key}=${value}
  fi
}

url=${1:-${DEPENDENCY_URL:-}}

if [ -n "${GITHUB_ACTIONS:-}" ]; then
    destination="${GITHUB_WORKSPACE}/dependencies"
elif [ -n "${GITLAB_CI:-}" ];then
    destination="${CI_PROJECT_DIR}/dependencies"
else
    destination=${2}
fi

set_env DEPENDENCY_DIR "${destination}"

if [ -z "${url}" ]; then
    echo "url is not set"
    exit 1
fi

echo "URL: $url"
echo "DESTINATION: $destination"

# check curl location
CURL=$(command -v curl)
if [ -z "$CURL" ]; then
    echo "curl is not available"
    exit 1
fi

UNZSTD=$(command -v unzstd)
if [ -z "$UNZSTD" ]; then
    echo "unzstd is not available"
    exit 1
fi

TAR=$(command -v tar)
if [ -z "$TAR" ]; then
    echo "tar is not available"
    exit 1
fi

run mkdir -p "${destination}"

run $CURL \
  --retry 5 \
  --connect-timeout 2 \
  --location $url \
  | unzstd \
  | tar \
    -x \
    --strip-components=1 \
    --directory "${destination}"

# Patch up geant4-config data install script
out=$(${destination}/bin/geant4-config --datasets)
line=$(echo "$out" | head -n1)
orig_share=$(echo "$line" | perl -pe 's|.*?(\/.*)\/share.*|\1|')
orig_share_escaped=$(echo $orig_share|perl -pe 's|/|\\/|g')
destination_escaped=$(echo "$destination"|perl -pe 's|/|\\/|g')
perl -pi.bak -e "s/$orig_share_escaped/$destination_escaped/g" ${destination}/bin/geant4-config

if [ -n "${GITHUB_ACTIONS:-}" ]; then
  echo "Running in GitHub Actions"
  venv="${GITHUB_WORKSPACE}/venv"
fi

if [ -n "${GITLAB_CI:-}" ];then
  echo "Running in GitLab CI"
  venv="${CI_PROJECT_DIR}/venv"
fi

if [ -n "${CI:-}" ];then
  run "${destination}/bin/python3" -m venv "${venv}"
  run "${venv}/bin/python3" -m pip install pyyaml jinja2
  set_env PATH "${venv}/bin:${destination}/bin/:${PATH}"
fi

set_env CMAKE_PREFIX_PATH "${destination}"
set_env LD_LIBRARY_PATH "${destination}/lib"
set_env ROOT_INCLUDE_PATH "${destination}/include"
# Geant4 puts CLHEP in a subdirectory
set_env ROOT_INCLUDE_PATH "${destination}/include/Geant4"
# Pythia8 looks for settings in this directory
set_env PYTHIA8DATA "${destination}/share/Pythia8/xmldoc"

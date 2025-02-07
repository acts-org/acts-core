#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set -e
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export GIT_CLIFF_CONFIG=$SCRIPT_DIR/cliff.toml

# helper function to selectively print and run commands without a subshell
function run() {
    set -x
    "$@"
    # save exit code
    { rec=$?; } 2> /dev/null
    { set +x;   } 2> /dev/null
    # restore exit code
    (exit $rec)
}

export run

version=$(git cliff --bumped-version)
echo "Bumped version will be: $version"

zenodo=$(cat .zenodo.json)
echo "$zenodo" \
  | jq --arg version "$version" '.version = $version' \
  | jq --arg version "$version" '.title = "acts-project/acts: \($version)"' \
  > .zenodo.json
echo "- Updated .zenodo.json"

citation=$(cat CITATION.cff)
echo "$citation" \
  | sed "s/^version: .*/version: $version/" \
  > CITATION.cff
echo "- Updated CITATION.cff"

echo "$version" | sed 's/^v//g' > version_number
echo "- Updated version_bumber"

run git add .zenodo.json CITATION.cff version_number
run git commit -n -m"Bump version to $version"
CI=${CI:-}
if [ -n "$CI" ]; then
  run git push
fi

run git cliff --tag "$version" --latest --unreleased -o release.md

RELEASE_TARGET=${RELEASE_TARGET:-$(git rev-parse HEAD)}

set +e
! gh release view "$version" > /dev/null 2>&1
release_exists=$?
set -e
if [[ $release_exists == 1 ]]; then
  echo "Release $version exists"
  run gh release edit $version \
    --notes-file release.md \
    --target $RELEASE_TARGET \
    --draft
else
  echo "Release $version does not exist"
  run gh release create $version \
    --title "$version" \
    --notes-file release.md \
    --target $RELEASE_TARGET \
    --draft
fi

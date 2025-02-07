# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# setup latest, supported LCG release via cvmfs

if test -n "$BASH_SOURCE"; then
  this_script=$BASH_SOURCE
elif test -n "$ZSH_VERSION"; then
  setopt function_argzero
  this_script=$0
else
  echo "Unsupported shell. Please use bash or zsh." 1>&2
  return
fi

dir="$( cd "$( dirname "${this_script}" )" && pwd )"
source $dir/setup_cvmfs_lcg105.sh

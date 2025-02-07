# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# setup appropriate LCG release via cvmfs

if test -e /etc/centos-release && grep 'CentOS Linux release 7' /etc/centos-release; then
  lcg_os=centos7
elif test -e /etc/centos-release && grep 'CentOS Stream release 8' /etc/centos-release; then
  lcg_os=centos8
# not centos. Check for RHEL
elif test -e /etc/redhat-release && grep 'Linux release 9' /etc/redhat-release; then
  lcg_os=el9
else
  echo "Unsupported system" 1>&2
  return
fi

lcg_release=LCG_105
lcg_compiler=gcc13
lcg_platform=x86_64-${lcg_os}-${lcg_compiler}-opt
lcg_view=/cvmfs/sft.cern.ch/lcg/views/${lcg_release}/${lcg_platform}

source ${lcg_view}/setup.sh
# extra variables required to build acts
export DD4hep_DIR=${lcg_view}

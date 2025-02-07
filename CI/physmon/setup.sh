# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}"  )" &> /dev/null && pwd  )
export PYTHONPATH+=:$(realpath "${SCRIPT_DIR}/../../Examples/Scripts/Python/")
export PYTHONPATH+=":${SCRIPT_DIR}"

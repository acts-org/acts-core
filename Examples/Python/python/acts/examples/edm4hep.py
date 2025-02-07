# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from acts._adapter import _patch_config
from acts import ActsPythonBindingsEDM4hep

_patch_config(ActsPythonBindingsEDM4hep)

from acts.ActsPythonBindingsEDM4hep import *

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from acts._adapter import _patch_config
from acts import ActsPythonBindings

if not hasattr(ActsPythonBindings._examples, "_onnx"):
    raise ImportError("ActsPythonBindings._examples._onnx not found")

_patch_config(ActsPythonBindings._examples._onnx)

from acts.ActsPythonBindings._examples._onnx import *

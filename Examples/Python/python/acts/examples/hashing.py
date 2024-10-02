from acts._adapter import _patch_config
from acts import ActsPythonBindings

if not hasattr(ActsPythonBindings._examples, "_hashing"):
    raise ImportError("ActsPythonBindings._examples._hashing not found")

_patch_config(ActsPythonBindings._examples._hashing)

from acts.ActsPythonBindings._examples._hashing import *

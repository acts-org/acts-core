# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
from typing import Union
import os
import warnings


from .ActsPythonBindings import *
from .ActsPythonBindings import __version__
from . import ActsPythonBindings
from ._adapter import _patch_config

if (
    "ACTS_LOG_FAILURE_THRESHOLD" in os.environ
    and os.environ["ACTS_LOG_FAILURE_THRESHOLD"] != logging.getFailureThreshold().name
):
    error = (
        "Runtime log failure threshold is given in environment variable "
        f"`ACTS_LOG_FAILURE_THRESHOLD={os.environ['ACTS_LOG_FAILURE_THRESHOLD']}`"
        "However, a compile-time value is set via CMake, i.e. "
        f"`ACTS_LOG_FAILURE_THRESHOLD={logging.getFailureThreshold().name}`. "
        "or `ACTS_ENABLE_LOG_FAILURE_THRESHOLD=OFF`, which disables runtime thresholds."
    )
    if "PYTEST_CURRENT_TEST" in os.environ:
        # test environment, fail hard
        raise RuntimeError(error)
    else:
        warnings.warn(error + "\nThe compile-time threshold will be used in this case!")


def Propagator(stepper, navigator, loglevel=ActsPythonBindings.logging.INFO):
    for prefix in ("Eigen", "Atlas", "StraightLine"):
        _stepper = getattr(ActsPythonBindings, f"{prefix}Stepper")
        if isinstance(stepper, _stepper):
            _detectorNavigator = getattr(ActsPythonBindings, "DetectorNavigator")
            if isinstance(navigator, _detectorNavigator):
                return getattr(
                    ActsPythonBindings._propagator, f"{prefix}DetectorPropagator"
                )(stepper, navigator, loglevel)
            return getattr(ActsPythonBindings._propagator, f"{prefix}Propagator")(
                stepper, navigator, loglevel
            )
    raise TypeError(f"Unknown stepper {type(stepper).__name__}")


_patch_config(ActsPythonBindings)


@staticmethod
def _decoratorFromFile(file: Union[str, Path], **kwargs):
    if isinstance(file, str):
        file = Path(file)

    kwargs.setdefault("level", ActsPythonBindings.logging.INFO)

    if file.suffix in (".json", ".cbor"):
        c = ActsPythonBindings.MaterialMapJsonConverter.Config()
        for k in kwargs.keys():
            if hasattr(c, k):
                setattr(c, k, kwargs.pop(k))

        return ActsPythonBindings.JsonMaterialDecorator(
            jFileName=str(file), rConfig=c, **kwargs
        )
    elif file.suffix == ".root":
        return ActsPythonBindings._examples.RootMaterialDecorator(
            fileName=str(file), **kwargs
        )
    else:
        raise ValueError(f"Unknown file type {file.suffix}")


ActsPythonBindings.IMaterialDecorator.fromFile = _decoratorFromFile

import os
import shutil
from typing import List, Union
import contextlib

import acts
from acts.examples import IAlgorithm

geant4Enabled = (
    any(v.startswith("G4") for v in os.environ.keys())
    or "GEANT4_DATA_DIR" in os.environ
)
if geant4Enabled:
    try:
        import acts.examples.geant4
    except ImportError:
        geant4Enabled = False

try:
    import ROOT

    rootEnabled = True
except ImportError:
    rootEnabled = False

    if "ROOTSYS" in os.environ:  # ROOT seems to be set up, but no PyROOT
        import warnings

        warnings.warn(
            "ROOT likely built without/with incompatible PyROOT. Skipping tests that need ROOT"
        )

dd4hepEnabled = "DD4hep_DIR" in os.environ
if dd4hepEnabled:
    try:
        import acts.examples.dd4hep
    except ImportError:
        dd4hepEnabled = False

try:
    import acts.examples.hepmc3

    hepmc3Enabled = True
except ImportError:
    hepmc3Enabled = False

try:
    import acts.examples.edm4hep

    edm4hepEnabled = True
except ImportError:
    edm4hepEnabled = False

try:
    import acts.examples.onnx

    onnxEnabled = True
except ImportError:
    onnxEnabled = False

try:
    from acts import covfie

    covfieEnabled = True
except ImportError:
    covfieEnabled = False


try:
    import acts.examples

    pythia8Enabled = hasattr(acts.examples, "pythia8")
except ImportError:
    pythia8Enabled = False

try:
    import acts.examples

    hashingSeedingEnabled = hasattr(acts.examples, "hashing")
except ImportError:
    hashingSeedingEnabled = False


exatrkxEnabled = shutil.which("nvidia-smi") is not None
if exatrkxEnabled:
    try:
        from acts.examples import TrackFindingAlgorithmExaTrkX
    except ImportError:
        exatrkxEnabled = False

try:
    import podio

    podioEnabled = True
except ModuleNotFoundError:
    podioEnabled = False

isCI = os.environ.get("CI", "false") == "true"

if isCI:
    for k, v in dict(locals()).items():
        if k.endswith("Enabled"):
            locals()[k] = True


class AssertCollectionExistsAlg(IAlgorithm):
    events_seen = 0
    collections: List[str]

    def __init__(
        self,
        collections: Union[List[str], str],
        name="check_alg",
        level=acts.logging.INFO,
        *args,
        **kwargs,
    ):
        if isinstance(collections, str):
            self.collections = [collections]
        else:
            self.collections = collections
        IAlgorithm.__init__(self, name=name, level=level, *args, **kwargs)

    def execute(self, ctx):
        for collection in self.collections:
            assert ctx.eventStore.exists(collection), f"{collection} does not exist"
        self.events_seen += 1
        return acts.examples.ProcessCode.SUCCESS


doHashChecks = os.environ.get("ROOT_HASH_CHECKS", "") != "" or "CI" in os.environ


@contextlib.contextmanager
def failure_threshold(level: acts.logging.Level, enabled: bool = True):
    prev = acts.logging.getFailureThreshold()
    if enabled and prev != level:
        try:
            acts.logging.setFailureThreshold(level)
        except RuntimeError:
            # Repackage with different error string
            raise RuntimeError(
                "Runtime log failure threshold could not be set. "
                "Compile-time value is probably set via CMake, i.e. "
                f"`ACTS_LOG_FAILURE_THRESHOLD={acts.logging.getFailureThreshold().name}` is set, "
                "or `ACTS_ENABLE_LOG_FAILURE_THRESHOLD=OFF`. "
                "The pytest test-suite will not work in this configuration."
            )

        yield
        acts.logging.setFailureThreshold(prev)
    else:
        yield

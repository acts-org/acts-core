import os
import sys
import math
from collections import namedtuple
from pathlib import Path
from typing import Optional
import acts
import acts.examples


def getOpenDataDetectorDirectory():
    odd_dir = os.environ.get("ODD_PATH")
    if odd_dir is None:
        raise RuntimeError("ODD_PATH environment variable not set")
    odd_dir = Path(odd_dir)
    return odd_dir


def getOpenDataDetector(
    mdecorator=None,
    odd_dir: Optional[Path] = None,
    logLevel=acts.logging.INFO,
):
    """This function sets up the open data detector. Requires DD4hep.
    Parameters
    ----------
    mdecorator: Material Decorator, take RootMaterialDecorator if non is given
    odd_dir: if not given, try to get via ODD_PATH environment variable
    logLevel: logging level
    """
    import acts.examples.dd4hep

    customLogLevel = acts.examples.defaultLogging(logLevel=logLevel)

    if odd_dir is None:
        odd_dir = getOpenDataDetectorDirectory()
    if not odd_dir.exists():
        raise RuntimeError(f"OpenDataDetector not found at {odd_dir}")

    odd_xml = odd_dir / "xml" / "OpenDataDetector.xml"
    if not odd_xml.exists():
        raise RuntimeError(f"OpenDataDetector.xml not found at {odd_xml}")

    env_vars = []
    map_name = "libOpenDataDetector.components"
    lib_name = None
    if sys.platform == "linux":
        env_vars = ["LD_LIBRARY_PATH"]
        lib_name = "libOpenDataDetector.so"
    elif sys.platform == "darwin":
        env_vars = ["DYLD_LIBRARY_PATH", "DD4HEP_LIBRARY_PATH"]
        lib_name = "libOpenDataDetector.dylib"

    if lib_name is not None and len(env_vars) > 0:
        found = False
        for env_var in env_vars:
            for lib_dir in os.environ.get(env_var, "").split(":"):
                lib_dir = Path(lib_dir)
                if (lib_dir / map_name).exists() and (lib_dir / lib_name).exists():
                    found = True
                    break
        if not found:
            msg = (
                "Unable to find OpenDataDetector factory library. "
                f"You might need to point {'/'.join(env_vars)} to build/thirdparty/OpenDataDetector/factory or other ODD install location"
            )
            raise RuntimeError(msg)

    volumeRadiusCutsMap = {
        28: [850.0],  # LStrip negative z
        30: [850.0],  # LStrip positive z
        23: [400.0, 550.0],  # SStrip negative z
        25: [400.0, 550.0],  # SStrip positive z
        16: [100.0],  # Pixels negative z
        18: [100.0],  # Pixels positive z
    }

    def geoid_hook(geoid, surface):
        gctx = acts.GeometryContext()
        if geoid.volume() in volumeRadiusCutsMap:
            r = math.sqrt(surface.center(gctx)[0] ** 2 + surface.center(gctx)[1] ** 2)

            geoid.setExtra(1)
            for cut in volumeRadiusCutsMap[geoid.volume()]:
                if r > cut:
                    geoid.setExtra(geoid.extra() + 1)

        return geoid

    dd4hepConfig = acts.examples.dd4hep.DD4hepGeometryService.Config(
        xmlFileNames=[str(odd_xml)],
        logLevel=customLogLevel(),
        dd4hepLogLevel=customLogLevel(),
        geometryIdentifierHook=acts.GeometryIdentifierHook(geoid_hook),
    )
    detector = acts.examples.dd4hep.DD4hepDetector()

    if mdecorator is None:
        mdecorator = acts.examples.RootMaterialDecorator(
            fileName=str(odd_dir / "data/odd-material-maps.root"),
            level=customLogLevel(minLevel=acts.logging.WARNING),
        )

    trackingGeometry, decorators = detector.finalize(dd4hepConfig, mdecorator)

    OpenDataDetector = namedtuple(
        "OpenDataDetector", ["detector", "trackingGeometry", "decorators"]
    )

    class OpenDataDetectorContextManager(OpenDataDetector):
        def __new__(cls, detector, trackingGeometry, decorators):
            return super(OpenDataDetectorContextManager, cls).__new__(
                cls, detector, trackingGeometry, decorators
            )

        def __enter__(self):
            return self

        def __exit__(self, *args):
            self.detector.drop()

    return OpenDataDetectorContextManager(detector, trackingGeometry, decorators)

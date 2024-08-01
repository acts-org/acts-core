import collections
import argparse
from pathlib import Path

import acts
from acts.examples.odd import getOpenDataDetector

PhysmonSetup = collections.namedtuple(
    "Setup",
    [
        "detector",
        "trackingGeometry",
        "decorators",
        "field",
        "digiConfig",
        "geoSel",
        "outdir",
    ],
)


def makeSetup(useGeometricConfig=False) -> PhysmonSetup:
    u = acts.UnitConstants
    srcdir = Path(__file__).resolve().parent.parent.parent

    parser = argparse.ArgumentParser()
    parser.add_argument("outdir")

    args = parser.parse_args()

    matDeco = acts.IMaterialDecorator.fromFile(
        srcdir / "thirdparty/OpenDataDetector/data/odd-material-maps.root",
        level=acts.logging.INFO,
    )

    detector, trackingGeometry, decorators = getOpenDataDetector(matDeco)
    setup = PhysmonSetup(
        detector=detector,
        trackingGeometry=trackingGeometry,
        decorators=decorators,
        digiConfig=srcdir / "Examples/Algorithms/Digitization/share/odd-digi-geometric-config.json" if useGeometricConfig else srcdir / "thirdparty/OpenDataDetector/config/odd-digi-smearing-config.json",
        geoSel=srcdir / "thirdparty/OpenDataDetector/config/odd-seeding-config.json",
        field=acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T)),
        outdir=Path(args.outdir),
    )

    setup.outdir.mkdir(exist_ok=True)

    return setup

import pytest

import acts

mm = acts.UnitConstants.mm
m = acts.UnitConstants.m
degree = acts.UnitConstants.degree

bv = acts.AxisDirection

gctx = acts.GeometryContext()
logLevel = acts.logging.VERBOSE


def test_zdirection_container_blueprint(tmp_path):

    def write(root: acts.BlueprintNode, stage: int):
        gz = tmp_path / f"blueprint_{stage}.dot"
        print(gz)
        with gz.open("w") as fh:
            root.graphviz(fh)

    base = acts.Transform3.Identity()

    root = acts.Blueprint(envelope=acts.ExtentEnvelope(r=[10 * mm, 10 * mm]))
    assert root.depth == 0

    barrel = root.addCylinderContainer("Barrel", direction=bv.AxisR)

    assert barrel.depth == 1

    r = 25 * mm
    for i in range(1, 3):
        r += 50 * mm
        bounds = acts.CylinderVolumeBounds(r, r + 20 * mm, 200 * mm)
        vol = barrel.addStaticVolume(base, bounds, name=f"Barrel_{i}")
        assert vol.depth == 2

    write(barrel, 1)

    root.clearChildren()

    assert barrel.depth == 0

    det = root.addCylinderContainer("Detector", direction=bv.AxisZ)

    assert det.depth == 1

    with det.CylinderContainer("nEC", direction=bv.AxisZ) as ec:
        assert ec.depth == 2
        z = -200
        for i in range(1, 3):
            z -= 200 * mm
            bounds = acts.CylinderVolumeBounds(100 * mm, 150 * mm, 50 * mm)

            trf = base * acts.Translation3(acts.Vector3(0, 0, z))

            vol = ec.addStaticVolume(trf, bounds, name=f"nEC_{i}")
            assert vol.depth == 3

        write(ec, 2)

    det.addChild(barrel)
    assert barrel.depth == 2

    write(det, 3)

    with det.CylinderContainer("pEC", direction=bv.AxisZ) as ec:
        assert ec.depth == 2
        z = 200
        for i in range(1, 3):
            z += 200 * mm
            bounds = acts.CylinderVolumeBounds(100 * mm, 150 * mm, 50 * mm)

            trf = base * acts.Translation3(acts.Vector3(0, 0, z))

            vol = ec.addStaticVolume(trf, bounds, name=f"pEC_{i}")
            assert vol.depth == 3

    write(root, 4)

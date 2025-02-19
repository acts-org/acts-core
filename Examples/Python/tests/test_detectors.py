import pytest
from pathlib import Path

from helpers import dd4hepEnabled

import acts.examples
from acts.examples.odd import getOpenDataDetector


def count_surfaces(geo):
    __tracebackhide__ = True
    nSurfaces = 0

    def visit(srf):
        nonlocal nSurfaces
        nSurfaces += 1

    geo.visitSurfaces(visit)

    return nSurfaces


def check_extra_odd(srf):
    if srf.geometryId.volume in [28, 30, 23, 25, 16, 18]:
        assert srf.geometryId.extra != 0
    return


def test_generic_geometry():
    detector = acts.examples.GenericDetector()
    trackingGeometry = detector.trackingGeometry()
    contextDecorators = detector.contextDecorators()
    assert detector is not None
    assert trackingGeometry is not None
    assert contextDecorators is not None

    assert count_surfaces(trackingGeometry) == 18728


def test_telescope_geometry():
    n_surfaces = 10

    config = acts.examples.TelescopeDetector.Config(
        bounds=[100, 100],
        positions=[10 * i for i in range(n_surfaces)],
        stereos=[0] * n_surfaces,
        binValue=0,
    )
    detector = acts.examples.TelescopeDetector(config)
    trackingGeometry = detector.trackingGeometry()
    contextDecorators = detector.contextDecorators()

    assert detector is not None
    assert trackingGeometry is not None
    assert contextDecorators is not None

    assert count_surfaces(trackingGeometry) == n_surfaces


@pytest.mark.skipif(not dd4hepEnabled, reason="DD4hep is not set up")
def test_odd():
    with getOpenDataDetector() as detector:
        trackingGeometry = detector.trackingGeometry()

        trackingGeometry.visitSurfaces(check_extra_odd)

        assert count_surfaces(trackingGeometry) == 18824


def test_aligned_detector():
    detector = acts.examples.AlignedDetector()
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    assert detector is not None
    assert trackingGeometry is not None
    assert decorators is not None

    assert count_surfaces(trackingGeometry) == 18728


import itertools


def test_tgeo_config_triplet(monkeypatch):
    from acts.examples import TGeoDetector, Interval

    # monkeypatch the comparison operator
    def eq(self, other):
        return self.lower == other.lower and self.upper == other.upper

    monkeypatch.setattr(Interval, "__eq__", eq)

    LayerTriplet = TGeoDetector.Config.LayerTriplet
    c = TGeoDetector.Config

    def assert_combinations(value, _type):
        t = LayerTriplet(value)
        assert t.negative == value and t.central == value and t.positive == value
        assert isinstance(t, _type)

        keys = ["negative", "central", "positive"]

        combinations = (
            [(k,) for k in keys] + list(itertools.combinations(keys, 2)) + [keys]
        )

        for c in combinations:
            d = {k: value for k in c}

            t = LayerTriplet(**d)
            assert isinstance(t, _type)
            for k in c:
                assert getattr(t, k) == value

    v = ["Some::SensorName"]
    assert_combinations(v, c.LayerTripletVectorString)

    with pytest.raises(TypeError):
        LayerTriplet(["Some::SensorName", 848])

    with pytest.raises(TypeError):
        LayerTriplet(("Some::SensorName", 848))

    for v in (True, False):
        assert_combinations(v, c.LayerTripletBool)

    assert_combinations("hallo", c.LayerTripletString)

    assert_combinations(5.3, c.LayerTripletDouble)

    assert_combinations(Interval(5.0, 9.0), c.LayerTripletInterval)

    with pytest.raises(TypeError):
        LayerTriplet(("a", 9))

    v = (4.4, 2.2)
    t = LayerTriplet(v)
    assert t.negative == Interval(*v)
    assert t.central == Interval(*v)
    assert t.positive == Interval(*v)


def test_tgeo_config_volume(monkeypatch):
    from acts.examples import TGeoDetector, Interval

    # monkeypatch the comparison operator
    def eq(self, other):
        return self.lower == other.lower and self.upper == other.upper

    monkeypatch.setattr(Interval, "__eq__", eq)

    Volume = TGeoDetector.Config.Volume

    v = Volume(name="blubb")
    assert v

    for key in ("binToleranceR", "binToleranceZ", "binTolerancePhi"):
        v = Volume(**{key: Interval(4, 5)})
        assert getattr(v, key) == Interval(4, 5)

        v = Volume(**{key: (4, 5)})
        assert getattr(v, key) == Interval(4, 5)

        v = Volume(**{key: (None, 5)})
        assert getattr(v, key) == Interval(None, 5)

        v = Volume(**{key: (4, None)})
        assert getattr(v, key) == Interval(4, None)


def test_coordinate_converter(trk_geo):
    digiCfg = acts.examples.DigitizationAlgorithm.Config(
        digitizationConfigs=acts.examples.readDigiConfigFromJson(
            str(
                Path(__file__).parent.parent.parent.parent
                / "Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
            )
        ),
        surfaceByIdentifier=trk_geo.geoIdSurfaceMap(),
    )
    converter = acts.examples.DigitizationCoordinatesConverter(digiCfg)

    def test_surface(surface):
        gctx = acts.GeometryContext()
        geo_id = surface.geometryId.value
        geo_center = surface.center(gctx)
        x, y, z = geo_center[0], geo_center[1], geo_center[2]

        # test if surface center can be reproduced
        assert converter.globalToLocal(geo_id, x, y, z) == (0, 0)
        assert converter.localToGlobal(geo_id, 0, 0) == (x, y, z)

        # test if we can get back to the same local coordinates
        global_shifted = converter.localToGlobal(geo_id, 5, 5)
        local_shifted = converter.globalToLocal(geo_id, *global_shifted)
        assert abs(local_shifted[0] - 5) / 5 < 1e-6
        assert abs(local_shifted[1] - 5) / 5 < 1e-6

    trk_geo.visitSurfaces(test_surface)

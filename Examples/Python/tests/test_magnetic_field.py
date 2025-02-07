# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import pytest
import random

import acts
import acts.examples

u = acts.UnitConstants


def test_null_bfield():
    nb = acts.NullBField()
    assert nb

    ct = acts.MagneticFieldContext()
    assert ct

    fc = nb.makeCache(ct)
    assert fc

    for i in range(100):
        x = random.uniform(-10000.0, 10000.0)
        y = random.uniform(-10000.0, 10000.0)
        z = random.uniform(-10000.0, 10000.0)

        rv = nb.getField(acts.Vector3(x, y, z), fc)

        assert rv[0] == pytest.approx(0.0)
        assert rv[1] == pytest.approx(0.0)
        assert rv[2] == pytest.approx(0.0)


def test_constant_bfield():
    with pytest.raises(TypeError):
        acts.ConstantBField()

    v = acts.Vector3(1, 2, 3)
    cb = acts.ConstantBField(v)
    assert cb

    ct = acts.MagneticFieldContext()
    assert ct

    fc = cb.makeCache(ct)
    assert fc

    for i in range(100):
        x = random.uniform(-10000.0, 10000.0)
        y = random.uniform(-10000.0, 10000.0)
        z = random.uniform(-10000.0, 10000.0)

        rv = cb.getField(acts.Vector3(x, y, z), fc)

        assert rv[0] == pytest.approx(1.0)
        assert rv[1] == pytest.approx(2.0)
        assert rv[2] == pytest.approx(3.0)


def test_solenoid(conf_const):
    solenoid = conf_const(
        acts.SolenoidBField,
        radius=1200 * u.mm,
        length=6000 * u.mm,
        bMagCenter=2 * u.T,
        nCoils=1194,
    )

    field = acts.solenoidFieldMap(
        rlim=(0, 1200 * u.mm),
        zlim=(-5000 * u.mm, 5000 * u.mm),
        nbins=(10, 10),
        field=solenoid,
    )

    assert isinstance(field, acts.examples.InterpolatedMagneticField2)


def test_multiregion_bfield():
    with pytest.raises(TypeError):
        acts.MultiRangeBField()

    rs = [
        (acts.RangeXDDim3((0, 3), (0, 3), (0, 3)), acts.Vector3(0.0, 0.0, 2.0)),
        (acts.RangeXDDim3((1, 2), (1, 2), (1, 10)), acts.Vector3(2.0, 0.0, 0.0)),
    ]
    f = acts.MultiRangeBField(rs)
    assert f

    ctx = acts.MagneticFieldContext()
    assert ctx

    fc = f.makeCache(ctx)
    assert fc

    rv = f.getField(acts.Vector3(0.5, 0.5, 0.5), fc)
    assert rv[0] == pytest.approx(0.0)
    assert rv[1] == pytest.approx(0.0)
    assert rv[2] == pytest.approx(2.0)

    rv = f.getField(acts.Vector3(1.5, 1.5, 5.0), fc)
    assert rv[0] == pytest.approx(2.0)
    assert rv[1] == pytest.approx(0.0)
    assert rv[2] == pytest.approx(0.0)

    rv = f.getField(acts.Vector3(2.5, 2.5, 2.5), fc)
    assert rv[0] == pytest.approx(0.0)
    assert rv[1] == pytest.approx(0.0)
    assert rv[2] == pytest.approx(2.0)

    rv = f.getField(acts.Vector3(1.5, 1.5, 1.5), fc)
    assert rv[0] == pytest.approx(2.0)
    assert rv[1] == pytest.approx(0.0)
    assert rv[2] == pytest.approx(0.0)

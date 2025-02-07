# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import sys
import os
import re
from pathlib import Path

import pytest

import acts
import acts.examples

pytestmark = [
    pytest.mark.skipif(
        sys.platform != "linux",
        reason="FPE monitoring currently only supported on Linux",
    ),
    pytest.mark.skipif(
        "ACTS_SEQUENCER_DISABLE_FPEMON" in os.environ,
        reason="Sequencer is configured to disable FPE monitoring",
    ),
]


_names = {
    acts.FpeType.FLTDIV: "DivByZero",
    acts.FpeType.FLTOVF: "Overflow",
    acts.FpeType.FLTINV: "Invalid",
}


_types = [
    pytest.param(acts.FpeType.FLTDIV, id="FLTDIV"),
    pytest.param(acts.FpeType.FLTOVF, id="FLTOVF"),
    pytest.param(acts.FpeType.FLTINV, id="FLTINV"),
]

_src = (Path(__file__).parent / "../src/Framework.cpp").resolve()
_locs = {}
with _src.open() as fh:
    _name_to_type = {v.lower(): k for k, v in _names.items()}
    for i, line in enumerate(fh):
        m = re.match(r".*// ?MARK: (.*)", line)
        if m is None:
            continue
        (name,) = m.groups()
        _locs[_name_to_type[name]] = (str(_src), (i + 1, i + 2))


class FpeMaker(acts.examples.IAlgorithm):
    def __init__(self, name):
        acts.examples.IAlgorithm.__init__(self, name, acts.logging.INFO)

    def execute(self, context):
        i = context.eventNumber % 4

        if i == 0 or i == 1:
            acts.FpeMonitor._trigger_divbyzero()
        elif i == 2:
            acts.FpeMonitor._trigger_overflow()
        elif i == 3:
            acts.FpeMonitor._trigger_invalid()

        return acts.examples.ProcessCode.SUCCESS


class FuncAlg(acts.examples.IAlgorithm):
    def __init__(self, name, func):
        acts.examples.IAlgorithm.__init__(self, name, acts.logging.INFO)
        self.func = func

    def execute(self, context):
        self.func(context)
        return acts.examples.ProcessCode.SUCCESS


@pytest.fixture(autouse=True)
def disable_log_threshold():
    prev = acts.logging.getFailureThreshold()
    acts.logging.setFailureThreshold(acts.logging.MAX)
    yield
    acts.logging.setFailureThreshold(prev)


def test_notrackfpe():
    s = acts.examples.Sequencer(
        events=3 * 100,
        trackFpes=False,
    )
    s.addAlgorithm(FpeMaker("FpeMaker"))

    s.run()

    res = s.fpeResult

    for x in acts.FpeType.values:
        assert res.count(x) == 0


@pytest.fixture(params=_types)
def fpe_type(request):
    yield request.param


def test_fpe_single_fail_at_end(fpe_type):
    s = acts.examples.Sequencer(
        events=10,
        failOnFirstFpe=False,
    )

    s.addAlgorithm(
        FuncAlg(
            _names[fpe_type],
            lambda _: getattr(
                acts.FpeMonitor, f"_trigger_{_names[fpe_type].lower()}"
            )(),
        )
    )
    with pytest.raises(RuntimeError):
        s.run()
    # fails, but will have run all 10 events
    res = s.fpeResult
    for x in acts.FpeType.values:
        assert res.count(x) == (s.config.events if x == fpe_type else 0)


def test_fpe_single_fail_immediately(fpe_type):
    s = acts.examples.Sequencer(
        events=10,
        failOnFirstFpe=True,
        numThreads=1,
    )

    s.addAlgorithm(
        FuncAlg(
            _names[fpe_type],
            lambda _: getattr(
                acts.FpeMonitor, f"_trigger_{_names[fpe_type].lower()}"
            )(),
        )
    )

    with pytest.raises(acts.FpeFailure):
        s.run()

    res = s.fpeResult
    for x in acts.FpeType.values:
        assert res.count(x) == (1 if x == fpe_type else 0)


def test_fpe_nocontext():
    class Alg(acts.examples.IAlgorithm):
        def __init__(self):
            acts.examples.IAlgorithm.__init__(self, "Alg", acts.logging.INFO)

        def execute(self, context):
            assert context.fpeMonitor is None
            return acts.examples.ProcessCode.SUCCESS

    s = acts.examples.Sequencer(
        events=10,
        trackFpes=False,
        numThreads=-1,
    )
    s.addAlgorithm(Alg())
    s.run()


def test_fpe_rearm(fpe_type):
    trigger = getattr(acts.FpeMonitor, f"_trigger_{_names[fpe_type].lower()}")

    class Alg(acts.examples.IAlgorithm):
        def __init__(self):
            acts.examples.IAlgorithm.__init__(self, "Alg", acts.logging.INFO)

        def execute(self, context):
            assert context.fpeMonitor is not None
            trigger()
            context.fpeMonitor.rearm()
            trigger()
            return acts.examples.ProcessCode.SUCCESS

    s = acts.examples.Sequencer(
        events=10,
        failOnFirstFpe=False,
        numThreads=-1,
    )
    s.addAlgorithm(Alg())
    with pytest.raises(RuntimeError):
        s.run()

    res = s.fpeResult
    for x in acts.FpeType.values:
        assert res.count(x) == (s.config.events * 2 if x == fpe_type else 0)


def test_fpe_masking_single(fpe_type):
    trigger = getattr(acts.FpeMonitor, f"_trigger_{_names[fpe_type].lower()}")

    def func(context):
        trigger()
        assert context.fpeMonitor is not None
        context.fpeMonitor.rearm()
        trigger()

    # Mask, but it's below threshold

    s = acts.examples.Sequencer(
        events=10,
        numThreads=-1,
        failOnFirstFpe=True,
        fpeMasks=[
            acts.examples.Sequencer.FpeMask(*_locs[fpe_type], fpe_type, 1),
        ],
    )

    s.addAlgorithm(FuncAlg("Alg", func))

    with pytest.raises(acts.FpeFailure):
        s.run()

    # Mask

    s = acts.examples.Sequencer(
        events=10,
        numThreads=-1,
        failOnFirstFpe=True,
        fpeMasks=[
            acts.examples.Sequencer.FpeMask(*_locs[fpe_type], fpe_type, 3),
        ],
    )

    s.addAlgorithm(FuncAlg("Alg", func))

    s.run()

    res = s.fpeResult
    for x in acts.FpeType.values:
        assert res.count(x) == (s.config.events * 2 if x == fpe_type else 0)


def test_masking_load_yaml(fpe_type, tmp_path, monkeypatch):
    def eq(self, other):
        return (
            self.file == other.file
            and self.lines == other.lines
            and self.type == other.type
            and self.count == other.count
        )

    monkeypatch.setattr(acts.examples.Sequencer.FpeMask, "__eq__", eq)

    import yaml

    masks = [
        acts.examples.Sequencer.FpeMask(*_locs[fpe_type], fpe_type, 1),
    ]
    file = tmp_path / "fpe_mask.yml"
    with file.open("w") as fh:
        yaml.dump(acts.examples.Sequencer.FpeMask.toDict(masks), fh)

    masks2 = acts.examples.Sequencer.FpeMask.fromFile(file)

    assert masks2 == masks


def test_fpe_context(fpe_type):
    trigger = getattr(acts.FpeMonitor, f"_trigger_{_names[fpe_type].lower()}")
    trigger()

    with acts.FpeMonitor.context() as fpe:
        trigger()

        print(fpe.result)


def test_buffer_sufficient():
    s = acts.examples.Sequencer(
        events=10000,
        failOnFirstFpe=False,
    )

    s.addAlgorithm(FuncAlg("Invalid", lambda _: acts.FpeMonitor._trigger_invalid()))
    with pytest.raises(RuntimeError):
        s.run()

    res = s.fpeResult
    for x in acts.FpeType.values:
        assert res.count(x) == (s.config.events if x == acts.FpeType.FLTINV else 0)

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import pytest

import acts

import acts.examples


def test_version():
    assert hasattr(acts, "__version__")
    assert hasattr(acts, "version")
    assert hasattr(acts.version, "major")
    assert hasattr(acts.version, "minor")
    assert hasattr(acts.version, "patch")
    assert hasattr(acts.version, "commit_hash")
    assert hasattr(acts.version, "commit_hash_short")


def test_logging():
    for l in ("VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"):
        assert hasattr(acts.logging, l)
        assert hasattr(acts.logging.Level, l)


def test_pgd_particle():
    assert len(acts.PdgParticle.__members__) == 19


def test_algebra():
    v3 = acts.Vector3(1, 2, 3)
    with pytest.raises(TypeError):
        acts.Vector3(1, 2, 3, 4)
    with pytest.raises(TypeError):
        acts.Vector3(1, 2)

    v3 = acts.Vector3([1, 2, 3])
    with pytest.raises(TypeError):
        acts.Vector3([1, 2, 3, 4])
    with pytest.raises(TypeError):
        acts.Vector3([1, 2])
    with pytest.raises(TypeError):
        acts.Vector3()

    v4 = acts.Vector4(1, 2, 3, 4)
    with pytest.raises(TypeError):
        v4 = acts.Vector4(1, 2, 3)
    v4 = acts.Vector4([1, 2, 3, 4])
    with pytest.raises(TypeError):
        acts.Vector4([1, 2, 3])
    with pytest.raises(TypeError):
        acts.Vector4()


def test_empty_sequencer(conf_const):
    s = acts.examples.Sequencer()
    with pytest.raises(RuntimeError):
        s.run()

    s = conf_const(acts.examples.Sequencer, events=1)
    s.run()


def test_sequencer_single_threaded(ptcl_gun, capfd):
    s = acts.examples.Sequencer(numThreads=1, events=2)
    ptcl_gun(s)
    s.run()
    cap = capfd.readouterr()
    assert cap.err == ""
    assert "Create Sequencer (single-threaded)" in cap.out
    assert "Processed 2 events" in cap.out


def test_sequencer_multi_threaded(ptcl_gun, capfd):
    # This test can use 2 threads (for the 2 events),
    # but could be run single-threaded if threading is not available.
    s = acts.examples.Sequencer(numThreads=-1, events=2)
    ptcl_gun(s)
    s.run()
    cap = capfd.readouterr()
    assert cap.err == ""
    assert "Create Sequencer" in cap.out
    assert "Processed 2 events" in cap.out


def test_random_number():
    rnd = acts.examples.RandomNumbers(seed=42)


def test_constructors():
    s1 = acts.examples.Sequencer()
    print(s1)
    s2 = acts.examples.Sequencer()
    print(s2)

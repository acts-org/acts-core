import os
import inspect
from pathlib import Path
import shutil
import math
import sys
import tempfile

import pytest

from helpers import (
    dd4hepEnabled,
    hepmc3Enabled,
    geant4Enabled,
    edm4hepEnabled,
    podioEnabled,
    AssertCollectionExistsAlg,
)

import acts
from acts import UnitConstants as u
from acts.examples import (
    ObjPropagationStepsWriter,
    TrackFinderPerformanceWriter,
    SeedingPerformanceWriter,
    RootPropagationStepsWriter,
    RootParticleWriter,
    RootTrackParameterWriter,
    RootMaterialTrackWriter,
    RootMaterialWriter,
    RootSimHitWriter,
    RootTrackStatesWriter,
    RootTrackSummaryWriter,
    VertexPerformanceWriter,
    RootMeasurementWriter,
    CsvParticleWriter,
    CsvSimHitWriter,
    CsvTrackWriter,
    CsvTrackingGeometryWriter,
    CsvMeasurementWriter,
    JsonMaterialWriter,
    JsonFormat,
    Sequencer,
    GenericDetector,
)
from acts.examples.odd import getOpenDataDetectorDirectory


@pytest.mark.obj
def test_obj_propagation_step_writer(tmp_path, trk_geo, conf_const, basic_prop_seq):
    with pytest.raises(TypeError):
        ObjPropagationStepsWriter()

    obj = tmp_path / "obj"
    obj.mkdir()

    s, alg = basic_prop_seq(trk_geo)
    w = conf_const(
        ObjPropagationStepsWriter,
        acts.logging.INFO,
        collection=alg.config.outputSummaryCollection,
        outputDir=str(obj),
    )

    s.addWriter(w)

    s.run()

    assert len([f for f in obj.iterdir() if f.is_file()]) == s.config.events
    for f in obj.iterdir():
        assert f.stat().st_size > 1024


@pytest.mark.csv
def test_csv_particle_writer(tmp_path, conf_const, ptcl_gun):
    s = Sequencer(numThreads=1, events=10)
    evGen = ptcl_gun(s)

    out = tmp_path / "csv"

    out.mkdir()

    s.addWriter(
        conf_const(
            CsvParticleWriter,
            acts.logging.INFO,
            inputParticles=evGen.config.outputParticles,
            outputStem="particle",
            outputDir=str(out),
        )
    )

    s.run()

    assert len([f for f in out.iterdir() if f.is_file()]) == s.config.events
    assert all(f.stat().st_size > 200 for f in out.iterdir())


@pytest.mark.root
def test_root_prop_step_writer(
    tmp_path, trk_geo, conf_const, basic_prop_seq, assert_root_hash
):
    with pytest.raises(TypeError):
        RootPropagationStepsWriter()

    file = tmp_path / "prop_steps.root"
    assert not file.exists()

    s, alg = basic_prop_seq(trk_geo)
    w = conf_const(
        RootPropagationStepsWriter,
        acts.logging.INFO,
        collection=alg.config.outputSummaryCollection,
        filePath=str(file),
    )

    s.addWriter(w)

    s.run()

    assert file.exists()
    assert file.stat().st_size > 2**10 * 50
    assert_root_hash(file.name, file)


@pytest.mark.root
def test_root_particle_writer(tmp_path, conf_const, ptcl_gun, assert_root_hash):
    s = Sequencer(numThreads=1, events=10)
    evGen = ptcl_gun(s)

    file = tmp_path / "particles.root"

    assert not file.exists()

    s.addWriter(
        conf_const(
            RootParticleWriter,
            acts.logging.INFO,
            inputParticles=evGen.config.outputParticles,
            filePath=str(file),
        )
    )

    s.run()

    assert file.exists()
    assert file.stat().st_size > 1024 * 10
    assert_root_hash(file.name, file)


@pytest.mark.root
def test_root_meas_writer(tmp_path, fatras, trk_geo, assert_root_hash):
    s = Sequencer(numThreads=1, events=10)
    evGen, simAlg, digiAlg = fatras(s)

    out = tmp_path / "meas.root"

    assert not out.exists()

    config = RootMeasurementWriter.Config(
        inputMeasurements=digiAlg.config.outputMeasurements,
        inputClusters=digiAlg.config.outputClusters,
        inputSimHits=simAlg.config.outputSimHits,
        inputMeasurementSimHitsMap=digiAlg.config.outputMeasurementSimHitsMap,
        filePath=str(out),
        surfaceByIdentifier=trk_geo.geoIdSurfaceMap(),
    )
    config.addBoundIndicesFromDigiConfig(digiAlg.config)
    s.addWriter(RootMeasurementWriter(level=acts.logging.INFO, config=config))
    s.run()

    assert out.exists()
    assert out.stat().st_size > 40000
    assert_root_hash(out.name, out)


@pytest.mark.root
def test_root_simhits_writer(tmp_path, fatras, conf_const, assert_root_hash):
    s = Sequencer(numThreads=1, events=10)
    evGen, simAlg, digiAlg = fatras(s)

    out = tmp_path / "meas.root"

    assert not out.exists()

    s.addWriter(
        conf_const(
            RootSimHitWriter,
            level=acts.logging.INFO,
            inputSimHits=simAlg.config.outputSimHits,
            filePath=str(out),
        )
    )

    s.run()
    assert out.exists()
    assert out.stat().st_size > 2e4
    assert_root_hash(out.name, out)


@pytest.mark.csv
def test_csv_meas_writer(tmp_path, fatras, trk_geo, conf_const):
    s = Sequencer(numThreads=1, events=10)
    evGen, simAlg, digiAlg = fatras(s)

    out = tmp_path / "csv"
    out.mkdir()

    s.addWriter(
        conf_const(
            CsvMeasurementWriter,
            level=acts.logging.INFO,
            inputMeasurements=digiAlg.config.outputMeasurements,
            inputClusters=digiAlg.config.outputClusters,
            inputMeasurementSimHitsMap=digiAlg.config.outputMeasurementSimHitsMap,
            outputDir=str(out),
        )
    )
    s.run()

    assert len([f for f in out.iterdir() if f.is_file()]) == s.config.events * 3
    assert all(f.stat().st_size > 10 for f in out.iterdir())


@pytest.mark.csv
def test_csv_simhits_writer(tmp_path, fatras, conf_const):
    s = Sequencer(numThreads=1, events=10)
    evGen, simAlg, digiAlg = fatras(s)

    out = tmp_path / "csv"
    out.mkdir()

    s.addWriter(
        conf_const(
            CsvSimHitWriter,
            level=acts.logging.INFO,
            inputSimHits=simAlg.config.outputSimHits,
            outputDir=str(out),
            outputStem="hits",
        )
    )

    s.run()
    assert len([f for f in out.iterdir() if f.is_file()]) == s.config.events
    assert all(f.stat().st_size > 200 for f in out.iterdir())


@pytest.mark.parametrize(
    "writer",
    [
        RootPropagationStepsWriter,
        RootParticleWriter,
        TrackFinderPerformanceWriter,
        SeedingPerformanceWriter,
        RootTrackParameterWriter,
        RootMaterialTrackWriter,
        RootMeasurementWriter,
        RootMaterialWriter,
        RootSimHitWriter,
        RootTrackStatesWriter,
        RootTrackSummaryWriter,
        VertexPerformanceWriter,
        SeedingPerformanceWriter,
    ],
)
@pytest.mark.root
def test_root_writer_interface(writer, conf_const, tmp_path, trk_geo):
    assert hasattr(writer, "Config")

    config = writer.Config

    assert hasattr(config, "filePath")
    assert hasattr(config, "fileMode")

    f = tmp_path / "target.root"
    assert not f.exists()

    kw = {"level": acts.logging.INFO, "filePath": str(f)}

    for k, _ in inspect.getmembers(config):
        if k.startswith("input"):
            kw[k] = "collection"
        if k == "surfaceByIdentifier":
            kw[k] = trk_geo.geoIdSurfaceMap()

    assert conf_const(writer, **kw)

    assert f.exists()


@pytest.mark.parametrize(
    "writer",
    [
        CsvParticleWriter,
        CsvMeasurementWriter,
        CsvSimHitWriter,
        CsvTrackWriter,
        CsvTrackingGeometryWriter,
    ],
)
@pytest.mark.csv
def test_csv_writer_interface(writer, conf_const, tmp_path, trk_geo):
    assert hasattr(writer, "Config")

    config = writer.Config

    assert hasattr(config, "outputDir")

    kw = {"level": acts.logging.INFO, "outputDir": str(tmp_path)}

    for k, _ in inspect.getmembers(config):
        if k.startswith("input"):
            kw[k] = "collection"
        if k == "trackingGeometry":
            kw[k] = trk_geo
        if k == "outputStem":
            kw[k] = "stem"

    assert conf_const(writer, **kw)


@pytest.mark.root
@pytest.mark.odd
@pytest.mark.skipif(not dd4hepEnabled, reason="DD4hep not set up")
def test_root_material_writer(tmp_path, assert_root_hash):
    from acts.examples.dd4hep import DD4hepDetector

    detector, trackingGeometry, _ = DD4hepDetector.create(
        xmlFileNames=[str(getOpenDataDetectorDirectory() / "xml/OpenDataDetector.xml")]
    )

    out = tmp_path / "material.root"

    assert not out.exists()

    rmw = RootMaterialWriter(level=acts.logging.WARNING, filePath=str(out))
    assert out.exists()
    assert out.stat().st_size > 0 and out.stat().st_size < 500
    rmw.write(trackingGeometry)

    assert out.stat().st_size > 1000
    assert_root_hash(out.name, out)


@pytest.mark.json
@pytest.mark.odd
@pytest.mark.parametrize("fmt", [JsonFormat.Json, JsonFormat.Cbor])
@pytest.mark.skipif(not dd4hepEnabled, reason="DD4hep not set up")
def test_json_material_writer(tmp_path, fmt):
    from acts.examples.dd4hep import DD4hepDetector

    detector, trackingGeometry, _ = DD4hepDetector.create(
        xmlFileNames=[str(getOpenDataDetectorDirectory() / "xml/OpenDataDetector.xml")]
    )

    out = (tmp_path / "material").with_suffix("." + fmt.name.lower())

    assert not out.exists()

    jmw = JsonMaterialWriter(
        level=acts.logging.WARNING, fileName=str(out.with_suffix("")), writeFormat=fmt
    )
    assert not out.exists()
    jmw.write(trackingGeometry)

    assert out.stat().st_size > 1000


@pytest.mark.csv
def test_csv_multitrajectory_writer(tmp_path):
    detector, trackingGeometry, decorators = GenericDetector.create()
    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    from truth_tracking_kalman import runTruthTrackingKalman

    s = Sequencer(numThreads=1, events=10)
    runTruthTrackingKalman(
        trackingGeometry,
        field,
        digiConfigFile=Path(
            str(
                Path(__file__).parent.parent.parent.parent
                / "Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
            )
        ),
        outputDir=tmp_path,
        s=s,
    )

    csv_dir = tmp_path / "csv"
    csv_dir.mkdir()
    s.addWriter(
        CsvTrackWriter(
            level=acts.logging.INFO,
            inputTracks="tracks",
            inputMeasurementParticlesMap="measurement_particles_map",
            outputDir=str(csv_dir),
        )
    )
    s.run()
    assert len([f for f in csv_dir.iterdir() if f.is_file()]) == 10
    assert all(f.stat().st_size > 20 for f in csv_dir.iterdir())


@pytest.fixture(scope="session")
def hepmc_data_impl(tmp_path_factory):
    import subprocess

    script = (
        Path(__file__).parent.parent.parent.parent
        / "Examples"
        / "Scripts"
        / "Python"
        / "event_recording.py"
    )
    assert script.exists()

    with tempfile.TemporaryDirectory() as tmp_path:
        env = os.environ.copy()
        env["NEVENTS"] = "1"
        subprocess.check_call([sys.executable, str(script)], cwd=tmp_path, env=env)

        outfile = Path(tmp_path) / "hepmc3/event000000000-events.hepmc3"
        # fake = Path("/scratch/pagessin/acts/hepmc3/event000000000-events.hepmc3")

        # outfile.parent.mkdir()
        # shutil.copy(fake, outfile)

        assert outfile.exists()

        yield outfile


@pytest.fixture
def hepmc_data(hepmc_data_impl: Path, tmp_path):
    dest = tmp_path / hepmc_data_impl.name
    shutil.copy(hepmc_data_impl, dest)

    return dest


@pytest.mark.skipif(not hepmc3Enabled, reason="HepMC3 plugin not available")
@pytest.mark.skipif(not dd4hepEnabled, reason="DD4hep not set up")
@pytest.mark.skipif(not geant4Enabled, reason="Geant4 not set up")
@pytest.mark.odd
@pytest.mark.slow
def test_hepmc3_histogram(hepmc_data, tmp_path):
    from acts.examples.hepmc3 import (
        HepMC3AsciiReader,
        HepMCProcessExtractor,
    )

    s = Sequencer(numThreads=1)

    s.addReader(
        HepMC3AsciiReader(
            level=acts.logging.INFO,
            inputDir=str(hepmc_data.parent),
            inputStem="events",
            outputEvents="hepmc-events",
        )
    )

    s.addAlgorithm(
        HepMCProcessExtractor(
            level=acts.logging.INFO,
            inputEvents="hepmc-events",
            extractionProcess="Inelastic",
        )
    )

    # This segfaults, see https://github.com/acts-project/acts/issues/914
    # s.addWriter(
    #     RootNuclearInteractionParametersWriter(
    #         level=acts.logging.INFO, inputSimulationProcesses="event-fraction"
    #     )
    # )

    alg = AssertCollectionExistsAlg(
        "hepmc-events", name="check_alg", level=acts.logging.INFO
    )
    s.addAlgorithm(alg)

    s.run()


@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_measurement_writer(tmp_path, fatras):
    from acts.examples.edm4hep import EDM4hepMeasurementWriter

    s = Sequencer(numThreads=1, events=10)
    _, simAlg, digiAlg = fatras(s)

    out = tmp_path / "measurements_edm4hep.root"

    s.addWriter(
        EDM4hepMeasurementWriter(
            level=acts.logging.VERBOSE,
            inputMeasurements=digiAlg.config.outputMeasurements,
            inputClusters=digiAlg.config.outputClusters,
            outputPath=str(out),
        )
    )

    s.run()

    assert os.path.isfile(out)
    assert os.stat(out).st_size > 10


@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_simhit_writer(tmp_path, fatras, conf_const):
    from acts.examples.edm4hep import EDM4hepSimHitWriter

    s = Sequencer(numThreads=1, events=10)
    _, simAlg, _ = fatras(s)

    out = tmp_path / "simhits_edm4hep.root"

    s.addWriter(
        conf_const(
            EDM4hepSimHitWriter,
            level=acts.logging.INFO,
            inputSimHits=simAlg.config.outputSimHits,
            outputPath=str(out),
        )
    )

    s.run()

    assert os.path.isfile(out)
    assert os.stat(out).st_size > 200


@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_particle_writer(tmp_path, conf_const, ptcl_gun):
    from acts.examples.edm4hep import EDM4hepParticleWriter

    s = Sequencer(numThreads=1, events=10)
    evGen = ptcl_gun(s)

    out = tmp_path / "particles_edm4hep.root"

    out.mkdir()

    s.addWriter(
        conf_const(
            EDM4hepParticleWriter,
            acts.logging.INFO,
            inputParticles=evGen.config.outputParticles,
            outputPath=str(out),
        )
    )

    s.run()

    assert os.path.isfile(out)
    assert os.stat(out).st_size > 200


@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_multitrajectory_writer(tmp_path):
    from acts.examples.edm4hep import EDM4hepMultiTrajectoryWriter

    detector, trackingGeometry, decorators = GenericDetector.create()
    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    from truth_tracking_kalman import runTruthTrackingKalman

    s = Sequencer(numThreads=1, events=10)
    runTruthTrackingKalman(
        trackingGeometry,
        field,
        digiConfigFile=Path(
            str(
                Path(__file__).parent.parent.parent.parent
                / "Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
            )
        ),
        outputDir=tmp_path,
        s=s,
    )

    s.addAlgorithm(
        acts.examples.TracksToTrajectories(
            level=acts.logging.INFO,
            inputTracks="tracks",
            outputTrajectories="trajectories",
        )
    )

    out = tmp_path / "trajectories_edm4hep.root"

    s.addWriter(
        EDM4hepMultiTrajectoryWriter(
            level=acts.logging.VERBOSE,
            inputTrajectories="trajectories",
            inputMeasurementParticlesMap="measurement_particles_map",
            outputPath=str(out),
        )
    )

    s.run()

    assert os.path.isfile(out)
    assert os.stat(out).st_size > 200


@pytest.mark.edm4hep
@pytest.mark.skipif(not edm4hepEnabled, reason="EDM4hep is not set up")
def test_edm4hep_tracks_writer(tmp_path):
    from acts.examples.edm4hep import EDM4hepTrackWriter

    detector, trackingGeometry, decorators = GenericDetector.create()
    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    from truth_tracking_kalman import runTruthTrackingKalman

    s = Sequencer(numThreads=1, events=10)
    runTruthTrackingKalman(
        trackingGeometry,
        field,
        digiConfigFile=Path(
            str(
                Path(__file__).parent.parent.parent.parent
                / "Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
            )
        ),
        outputDir=tmp_path,
        s=s,
    )

    out = tmp_path / "tracks_edm4hep.root"

    s.addWriter(
        EDM4hepTrackWriter(
            level=acts.logging.VERBOSE,
            inputTracks="kf_tracks",
            outputPath=str(out),
            Bz=2 * u.T,
        )
    )

    s.run()

    assert os.path.isfile(out)
    assert os.stat(out).st_size > 200

    if not podioEnabled:
        import warnings

        warnings.warn(
            "edm4hep output checks were skipped, because podio was not on the python path"
        )
        return

    from podio.root_io import Reader
    import cppyy

    reader = Reader(str(out))

    actual = []

    for frame in reader.get("events"):
        tracks = frame.get("ActsTracks")
        for track in tracks:
            actual.append(
                (track.getChi2(), track.getNdf(), len(track.getTrackStates()))
            )

            locs = []

            perigee = None
            for ts in track.getTrackStates():
                if ts.location == cppyy.gbl.edm4hep.TrackState.AtIP:
                    perigee = ts
                    continue
                locs.append(ts.location)

                rp = ts.referencePoint
                r = math.sqrt(rp.x**2 + rp.y**2)
                assert r > 25

            assert locs[0] == cppyy.gbl.edm4hep.TrackState.AtLastHit
            assert locs[-1] == cppyy.gbl.edm4hep.TrackState.AtFirstHit

            assert perigee is not None
            rp = perigee.referencePoint
            assert rp.x == 0.0
            assert rp.y == 0.0
            assert rp.z == 0.0
            assert abs(perigee.D0) < 1e0
            assert abs(perigee.Z0) < 1e1

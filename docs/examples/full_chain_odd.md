<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Walkthrough of the OpenDataDetector full chain example

The OpenDataDetector (ODD) is fictitious silicon detector which resides in its own repository on [GitLab](https://gitlab.cern.ch/acts/OpenDataDetector). It is used for testing and as a performance baseline in ACTS.

Our full chain ODD example is written in Python and can be found [here](https://github.com/acts-project/acts/blob/main/Examples/Scripts/Python/full_chain_odd.py).

The first step is to load the ODD detector description and to construct the detector. `getOpenDataDetectorDirectory` gives us the ODD folder within the `thirdparty` directory in Acts. We load our preferred material map and provide it to the detector construction `getOpenDataDetector`.

```python
oddDir = getOpenDataDetectorDirectory()

oddMaterialMap = oddDir / "data/odd-material-maps.root"
oddMaterialDeco = acts.IMaterialDecorator.fromFile(oddMaterialMap)

detector = getOpenDataDetector(mdecorator=oddMaterialDeco)
trackingGeometry = detector.trackingGeometry()
decorators = detector.contextDecorators()
```

In our simple example we assume a homogeneous magnetic field along the beam axis with 2 T. The magnetic field is passed to all the different algorithms in our simulation and the reconstruction pipeline.

```python
field = acts.ConstantBField(acts.Vector3(0.0, 0.0, 2.0 * u.T))
```

The simulation step involves random processes and in order to get the same results across different executions we provide our preferred random number generator with an arbitrary seed.

```python
rnd = acts.examples.RandomNumbers(seed=42)
```

All simulation and reconstruction pipelines in ACTS begin with a `Sequencer`. It controls the execution of the different algorithms in the chain. We provide the number of events, the number of threads to use (`-1` to use all the machine's cores) and the desired log level.

```python
s = acts.examples.Sequencer(events=100, numThreads=-1, logLevel=acts.logging.INFO)
```

Our first simulation step is the particle gun.
It spawns particles and their initial parameters, like position and momentum, inside our detector.

In our simple example we generate a single muon with random charge (i.e. muon or anti-muon) with 1-10 GeV with uniform pseudorapidity from -3 to 3.

Relativistic muons are hardly deflected in the detector and will keep most of their energy which makes them almost ideal for track reconstruction.

```python
addParticleGun(
    s,
    MomentumConfig(1.0 * u.GeV, 10.0 * u.GeV, transverse=True),
    EtaConfig(-3.0, 3.0, uniform=True),
    ParticleConfig(1, acts.PdgParticle.eMuon, randomizeCharge=True),
    rnd=rnd,
)
```

These newly created particles now need to be propagated through our detector. Fatras will do this for us if we provide it the detector geometry and the magnetic field.

Learn more about Fatras [here](/fatras/fatras).

```python
addFatras(
    s,
    trackingGeometry,
    field,
    outputDirRoot=outputDir,
    rnd=rnd,
)
```

The last step in the simulation is the digitization. Here we simulate the readout of the detector after a charged particle interacted with active detector material.

In the simplest case we use a gaussian smearing of the true hit and displace it slightly.

```python
oddDigiConfig = oddDir / "config/odd-digi-smearing-config.json"

addDigitization(
    s,
    trackingGeometry,
    field,
    digiConfigFile=oddDigiConfig,
    outputDirRoot=outputDir,
    rnd=rnd,
)
```

With the last step we completed the simulation and switch the focus to the actual topic: reconstruction.

The first step in our reconstruction is the track seeding. Here we try to find tracks and estimate their parameters.

```python
oddSeedingSel = oddDir / "config/odd-seeding-config.json"

addSeeding(
    s,
    trackingGeometry,
    field,
    geoSelectionConfigFile=oddSeedingSel,
    outputDirRoot=outputDir,
)
```

The Combinatorial Kalman Filter (CKF) will use the seeds to propagate the trajectory forward and backward in time with the idea to find more measurements along the way.
It combines (i.e. smooths) these measurements and outputs reconstructed tracks which include smoothed track parameters for each measurement.

```python
addCKFTracks(
    s,
    trackingGeometry,
    field,
    outputDirRoot=outputDir,
)
```

Our very last step in the reconstruction is the vertexing. In this step we try to find the origin of our tracks which are usually in the beam pipe at center of our detector.

```python
addVertexFitting(
    s,
    field,
    vertexFinder=VertexFinder.AMVF,
    outputDirRoot=outputDir,
)
```

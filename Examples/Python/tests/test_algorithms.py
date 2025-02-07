# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import pytest

from acts.examples import (
    AdaptiveMultiVertexFinderAlgorithm,
    VertexFitterAlgorithm,
    IterativeVertexFinderAlgorithm,
    SpacePointMaker,
    TrackFindingAlgorithm,
    SeedingAlgorithm,
    TrackParamsEstimationAlgorithm,
    EventGenerator,
    FatrasSimulation,
    MaterialMapping,
    TruthTrackFinder,
    ParticleSelector,
    TruthVertexFinder,
    TrackParameterSmearing,
    TrackSelectorAlgorithm,
    TrackFittingAlgorithm,
    ParticlesPrinter,
    TrackParametersPrinter,
    PropagationAlgorithm,
    DigitizationAlgorithm,
)


from helpers import geant4Enabled, hepmc3Enabled


@pytest.mark.parametrize(
    "alg",
    [
        AdaptiveMultiVertexFinderAlgorithm,
        VertexFitterAlgorithm,
        IterativeVertexFinderAlgorithm,
        SpacePointMaker,
        TrackFindingAlgorithm,
        SeedingAlgorithm,
        TrackParamsEstimationAlgorithm,
        EventGenerator,
        FatrasSimulation,
        MaterialMapping,
        TruthTrackFinder,
        ParticleSelector,
        TruthVertexFinder,
        TrackParameterSmearing,
        TrackSelectorAlgorithm,
        TrackFittingAlgorithm,
        ParticlesPrinter,
        TrackParametersPrinter,
        PropagationAlgorithm,
        # GeantinoRecording,
        # EventRecording,
    ],
)
def test_algorithm_interface(alg):
    assert hasattr(alg, "Config")


@pytest.mark.skipif(not geant4Enabled, reason="Geant4 not set up")
@pytest.mark.skipif(not hepmc3Enabled, reason="HepMC3 not set up")
def test_g4_algorithms():
    from acts.examples.geant4.hepmc3 import EventRecording
    from acts.examples.geant4 import Geant4Simulation

    for alg in (EventRecording, Geant4Simulation):
        assert hasattr(alg, "Config")


@pytest.mark.skipif(not hepmc3Enabled, reason="HepMC3 not set up")
def test_hepmc_algorithms():
    from acts.examples.hepmc3 import HepMCProcessExtractor

    assert hasattr(HepMCProcessExtractor, "Config")


def test_special_algorithm_interfaces():
    # just assert they exists
    assert DigitizationAlgorithm

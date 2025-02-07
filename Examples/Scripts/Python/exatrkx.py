#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
import os
import sys

import acts.examples
import acts
from acts.examples.reconstruction import addExaTrkX, ExaTrkXBackend
from acts import UnitConstants as u

from digitization import runDigitization


def runGNNTrackFinding(
    trackingGeometry,
    field,
    outputDir,
    digiConfigFile,
    geometrySelection,
    backend,
    modelDir,
    outputRoot=False,
    outputCsv=False,
    s=None,
):
    s = runDigitization(
        trackingGeometry,
        field,
        outputDir,
        digiConfigFile=digiConfigFile,
        particlesInput=None,
        outputRoot=outputRoot,
        outputCsv=outputCsv,
        s=s,
    )

    addExaTrkX(
        s,
        trackingGeometry,
        geometrySelection,
        modelDir,
        backend=backend,
        outputDirRoot=outputDir if outputRoot else None,
    )

    s.run()


if "__main__" == __name__:

    backend = ExaTrkXBackend.Torch

    if "onnx" in sys.argv:
        backend = ExaTrkXBackend.Onnx
    if "torch" in sys.argv:
        backend = ExaTrkXBackend.Torch

    detector = acts.examples.GenericDetector()
    trackingGeometry = detector.trackingGeometry()

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    srcdir = Path(__file__).resolve().parent.parent.parent.parent

    geometrySelection = (
        srcdir
        / "Examples/Algorithms/TrackFinding/share/geoSelection-genericDetector.json"
    )
    assert geometrySelection.exists()

    digiConfigFile = (
        srcdir
        / "Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
    )
    assert digiConfigFile.exists()

    if backend == ExaTrkXBackend.Torch:
        modelDir = Path.cwd() / "torchscript_models"
        assert (modelDir / "embed.pt").exists()
        assert (modelDir / "filter.pt").exists()
        assert (modelDir / "gnn.pt").exists()
    else:
        modelDir = Path.cwd() / "onnx_models"
        assert (modelDir / "embedding.onnx").exists()
        assert (modelDir / "filtering.onnx").exists()
        assert (modelDir / "gnn.onnx").exists()

    s = acts.examples.Sequencer(events=2, numThreads=1)
    s.config.logLevel = acts.logging.INFO

    rnd = acts.examples.RandomNumbers()
    outputDir = Path(os.getcwd())

    runGNNTrackFinding(
        trackingGeometry,
        field,
        outputDir,
        digiConfigFile,
        geometrySelection,
        backend,
        modelDir,
        outputRoot=True,
        outputCsv=False,
        s=s,
    )

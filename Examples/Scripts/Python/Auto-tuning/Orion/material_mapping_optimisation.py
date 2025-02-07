#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os
import argparse
import time
from datetime import datetime

from orion.client import build_experiment
from orion.storage.base import get_storage

import acts
from acts import (
    SurfaceMaterialMapper,
    VolumeMaterialMapper,
    Navigator,
    Propagator,
    StraightLineStepper,
    MaterialMapJsonConverter,
)
from acts.examples import (
    Sequencer,
    WhiteBoard,
    AlgorithmContext,
    ProcessCode,
    RootMaterialTrackReader,
    MaterialMapping,
    JsonMaterialWriter,
    JsonFormat,
)
from acts.examples.odd import getOpenDataDetector


def runMaterialMappingNoTrack(
    trackingGeometry,
    decorators,
    outputDir,
    inputDir,
    mapName="material-map",
    mapSurface=True,
    mapVolume=True,
    format=JsonFormat.Json,
    readCachedSurfaceInformation=False,
    s=None,
):
    """
    Implementation of the material mapping that doesn't write the material tracks.
    Used to create the material map that will then be used to compute the material variance.

    trackingGeometry : The tracking geometry
    decorators : The decorators for the tracking geometry
    outputDir : Output directory for the material map
    inputDir : Input directory containing the Material track
    mapName : Name of the material map
    mapSurface : Is material being mapped onto surfaces ?
    mapVolume : Is material being mapped onto volumes ?
    format : Json format used to write the material map (json, cbor, ...)
    readCachedSurfaceInformation : If set to true it will be assumed that the surface has already been associated with each material interaction in the input file.
    """

    s = s or Sequencer(numThreads=1)
    for decorator in decorators:
        s.addContextDecorator(decorator)
    wb = WhiteBoard(acts.logging.INFO)
    context = AlgorithmContext(0, 0, wb)
    for decorator in decorators:
        assert decorator.decorate(context) == ProcessCode.SUCCESS

    # Read material step information from a ROOT TTRee
    s.addReader(
        RootMaterialTrackReader(
            level=acts.logging.INFO,
            outputMaterialTracks="material-tracks",
            fileList=[
                os.path.join(
                    inputDir,
                    (
                        "optimised-material-map_tracks.root"
                        if readCachedSurfaceInformation
                        else "geant4_material_tracks.root"
                    ),
                )
            ],
            readCachedSurfaceInformation=readCachedSurfaceInformation,
        )
    )

    stepper = StraightLineStepper()
    mmAlgCfg = MaterialMapping.Config(context.geoContext, context.magFieldContext)
    mmAlgCfg.trackingGeometry = trackingGeometry
    mmAlgCfg.inputMaterialTracks = "material-tracks"

    if mapSurface:
        navigator = Navigator(
            trackingGeometry=trackingGeometry,
            resolveSensitive=True,
            resolveMaterial=True,
            resolvePassive=True,
        )
        propagator = Propagator(stepper, navigator)
        mapper = SurfaceMaterialMapper(level=acts.logging.INFO, propagator=propagator)
        mmAlgCfg.materialSurfaceMapper = mapper

    if mapVolume:
        navigator = Navigator(
            trackingGeometry=trackingGeometry,
        )
        propagator = Propagator(stepper, navigator)
        mapper = VolumeMaterialMapper(
            level=acts.logging.INFO, propagator=propagator, mappingStep=999
        )
        mmAlgCfg.materialVolumeMapper = mapper

    jmConverterCfg = MaterialMapJsonConverter.Config(
        processSensitives=True,
        processApproaches=True,
        processRepresenting=True,
        processBoundaries=True,
        processVolumes=True,
        context=context.geoContext,
    )

    jmw = JsonMaterialWriter(
        level=acts.logging.VERBOSE,
        converterCfg=jmConverterCfg,
        fileName=os.path.join(outputDir, mapName),
        writeFormat=format,
    )

    mmAlgCfg.materialWriters = [jmw]

    s.addAlgorithm(MaterialMapping(level=acts.logging.INFO, config=mmAlgCfg))

    return s


def runMaterialMappingVariance(
    binMap,
    events,
    job,
    inputPath,
    pathExp,
    pipeResult,
    readCachedSurfaceInformation=False,
):
    """
    Run the material mapping and compute the variance for each bin of each surface
    Return a dict with the GeometryId value of the surface as a key that stores
    a list of pairs corresponding to the variance and number of tracks associated with each bin of the surface

    binMap : Map containing the binning for each surface
    events : Number of event to use in the mapping
    job : ID of the job
    inputPath : Directory containing the input geantino track and the json geometry
    pathExp : Material mapping optimisation path
    pipeResult : Pipe to send back the score to the main python instance
    readCachedSurfaceInformation : Are surface information stored in the material track. Switch to true if the mapping was already performed to improve the speed.
    """
    print(
        datetime.now().strftime("%H:%M:%S") + "    Start mapping for job " + str(job),
        flush=True,
    )
    mapName = "material-map-" + str(job)
    mapSurface = True
    mapVolume = False

    # Create a MappingMaterialDecorator based on the tracking geometry
    matDeco = acts.IMaterialDecorator.fromFile(
        str(os.path.join(inputPath, "geometry-map.json"))
    )
    detectorTemp = getOpenDataDetector(matDeco)
    trackingGeometryTemp = detectorTemp.trackingGeometry()
    matMapDeco = acts.MappingMaterialDecorator(
        tGeometry=trackingGeometryTemp, level=acts.logging.ERROR
    )
    # Update the binning using the bin map corresponding to this trial
    matMapDeco.setBinningMap(binMap)

    # Decorate the detector with the MappingMaterialDecorator
    detector = getOpenDataDetector(matMapDeco)
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    # Sequence for the mapping, only use one thread when mapping material
    sMap = acts.examples.Sequencer(
        events=events, numThreads=1, logLevel=acts.logging.INFO
    )

    # Run the material mapping without writing the material track (as they take too much space)
    runMaterialMappingNoTrack(
        trackingGeometry,
        decorators,
        outputDir=pathExp,
        inputDir=inputPath,
        mapName=mapName,
        format=JsonFormat.Cbor,
        mapVolume=mapVolume,
        readCachedSurfaceInformation=readCachedSurfaceInformation,
        s=sMap,
    )
    sMap.run()

    # Compute the variance by rerunning the mapping
    print(
        datetime.now().strftime("%H:%M:%S")
        + "    Job "
        + str(job)
        + ": second pass to compute the variance",
        flush=True,
    )
    # Use the material map from the previous mapping as an input
    cborMap = os.path.join(pathExp, (mapName + ".cbor"))
    matDecoVar = acts.IMaterialDecorator.fromFile(cborMap)
    detectorVar = getOpenDataDetector(matDecoVar)
    trackingGeometryVar = detectorVar.trackingGeometry()
    decoratorsVar = detectorVar.contextDecorators()
    s = acts.examples.Sequencer(events=events, numThreads=1, logLevel=acts.logging.INFO)
    for decorator in decoratorsVar:
        s.addContextDecorator(decorator)
    wb = WhiteBoard(acts.logging.ERROR)
    context = AlgorithmContext(0, 0, wb)
    for decorator in decoratorsVar:
        assert decorator.decorate(context) == ProcessCode.SUCCESS

    # Read material step information from a ROOT TTRee
    reader = RootMaterialTrackReader(
        level=acts.logging.ERROR,
        outputMaterialTracks="material-tracks",
        fileList=[
            os.path.join(
                inputPath,
                (
                    "optimised-material-map_tracks.root"
                    if readCachedSurfaceInformation
                    else "geant4_material_tracks.root"
                ),
            )
        ],
        readCachedSurfaceInformation=readCachedSurfaceInformation,
    )
    s.addReader(reader)
    stepper = StraightLineStepper()
    mmAlgCfg = MaterialMapping.Config(context.geoContext, context.magFieldContext)
    mmAlgCfg.trackingGeometry = trackingGeometryVar
    mmAlgCfg.inputMaterialTracks = "material-tracks"

    if mapSurface:
        navigator = Navigator(
            trackingGeometry=trackingGeometryVar,
            resolveSensitive=True,
            resolveMaterial=True,
            resolvePassive=True,
        )
        propagator = Propagator(stepper, navigator)
        surfaceCfg = SurfaceMaterialMapper.Config(
            computeVariance=True
        )  # Don't forget to turn the `computeVariance` to true
        mapper = SurfaceMaterialMapper(
            config=surfaceCfg, level=acts.logging.ERROR, propagator=propagator
        )
        mmAlgCfg.materialSurfaceMapper = mapper

    if mapVolume:
        navigator = Navigator(
            trackingGeometry=trackingGeometryVar,
        )
        propagator = Propagator(stepper, navigator)
        mapper = VolumeMaterialMapper(
            level=acts.logging.ERROR, propagator=propagator, mappingStep=999
        )
        mmAlgCfg.materialVolumeMapper = mapper

    mapping = MaterialMapping(level=acts.logging.ERROR, config=mmAlgCfg)
    s.addAlgorithm(mapping)
    s.run()

    # Compute the scoring parameters
    score = dict()
    for key in binMap:
        objective = 0
        nonZero = 0
        binParameters = mapping.scoringParameters(key)
        # Objective : Sum of variance in all bin divided by the number of bin
        # The variance is scaled by (1.0 + 1.0/(number of hits in the bin)) to encourage larger bin at equal score
        for parameters in binParameters:
            if parameters[1] != 0:
                objective += parameters[0] * (1.0 + 1.0 / parameters[1])
                nonZero += 1
        if nonZero != 0:
            objective = objective / nonZero
        score[key] = [dict(name="surface_score", type="objective", value=objective)]
    print(
        datetime.now().strftime("%H:%M:%S")
        + "    Mapping over for job "
        + str(job)
        + " : now sending score",
        flush=True,
    )
    pipeResult.send(score)

    os.remove(cborMap)


def surfaceExperiment(key, nbJobs, pathDB, pathResult, pipeBin, pipeResult, doPloting):
    """
    This function create an experiment for a given single surface
    Due to how Orion is implemented only one DB can exist per job, this thus need to be call using pythons multiprocessing to circumvent the issue.

    key : Id of the surface corresponding to this experiment
    nbJobs : Total number of jobs to be executed simultaneously
    pathDB : Path to the databases
    pathResult : Path to write the result of the optimisation
    pipeBin : Pipe use to send the experiment binning to the main python instance
    pipeResult : Pipe to receive the result of the optimisation
    doPloting : true if we want to plot the result of the optimisation and obtain the optimal material map
    """
    # Create the database
    storage = {
        "database": {
            "name": "database_" + str(key),
            "type": "pickleddb",
            "host": os.path.join(pathDB, "database_" + str(key) + ".pkl"),
            "timeout": 43200,
        },
    }
    # Create the search space, the range of the binning can be chosen here
    # x represent X or phi depending on the type of surface
    # y represent Y, R or Z depending on the type of surface
    space = {
        "x": "uniform(1, 240, discrete=True)",
        "y": "uniform(1, 240, discrete=True)",
    }
    # Build the experiment
    experiments = build_experiment(
        "s_" + str(key),
        version="1",
        space=space,
        algorithms="random",
        storage=storage,
        max_idle_time=43200,
    )
    # Clean trial that haven't been completed
    store = get_storage()
    store.delete_trials(uid=experiments.id, where={"status": {"$ne": "completed"}})
    store.release_algorithm_lock(uid=experiments.id)

    # Suggest one binning per job and then send them via the pipe
    trials = dict()
    binMap = dict()
    for job in range(nbJobs):
        trials[job] = experiments.suggest()
        binMap[job] = (trials[job].params["x"], trials[job].params["y"])
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Binning for job "
            + str(job)
            + " and surface "
            + str(key)
            + " has been selected",
            flush=True,
        )
        pipeBin.send(binMap[job])
    print(
        datetime.now().strftime("%H:%M:%S")
        + "    All binning for surface "
        + str(key)
        + " has been sent",
        flush=True,
    )
    # Store the score resulting for the jobs in the database
    for job in range(nbJobs):
        score = pipeResult.recv()
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Received score for job "
            + str(job)
            + " and surface "
            + str(key),
            flush=True,
        )
        experiments.observe(trials[job], score)
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Score for job "
            + str(job)
            + " and surface "
            + str(key)
            + " has been written",
            flush=True,
        )

    # Create some performances plots for each surface
    if doPloting:
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    All the jobs are over. Now creating the optimisation plots",
            flush=True,
        )

        pathExpSurface = os.path.join(pathResult, "b_" + str(key))
        if not os.path.isdir(pathExpSurface):
            os.makedirs(pathExpSurface)

        regret = experiments.plot.regret()
        regret.write_html(pathExpSurface + "/regret.html")

        parallel_coordinates = experiments.plot.parallel_coordinates()
        parallel_coordinates.write_html(pathExpSurface + "/parallel_coordinates.html")

        lpi = experiments.plot.lpi()
        lpi.write_html(pathExpSurface + "/lpi.html")

        partial_dependencies = experiments.plot.partial_dependencies()
        partial_dependencies.write_html(pathExpSurface + "/partial_dependencies.html")

        # Select the optimal binning and send it via the pipe
        df = experiments.to_pandas()
        best = df.iloc[df.objective.idxmin()]
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Best score for surface "
            + str(key)
            + " : "
            + str(best),
            flush=True,
        )
        resultBinMap = (best.x, best.y)
        pipeBin.send(resultBinMap)


if "__main__" == __name__:
    print(datetime.now().strftime("%H:%M:%S") + "    Starting")
    # Optimiser arguments
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--numberOfJobs", nargs="?", default=2, type=int
    )  # number of parallele jobs
    parser.add_argument(
        "--topNumberOfEvents", nargs="?", default=100, type=int
    )  # number of events per trials
    parser.add_argument(
        "--inputPath", nargs="?", default=os.getcwd(), type=str
    )  # path to the input
    parser.add_argument(
        "--outputPath", nargs="?", default="", type=str
    )  # path to the output
    parser.add_argument(
        "--doPloting", action="store_true"
    )  # Return the optimisation plot and create the optimal material map
    parser.add_argument(
        "--readCachedSurfaceInformation", action="store_true"
    )  # Use surface information from the material track
    parser.set_defaults(doPloting=False)
    parser.set_defaults(readCachedSurfaceInformation=False)
    args = parser.parse_args()

    # Define the useful path and create them if they do not exist
    pathExp = os.path.join(args.outputPath, "Mapping")
    pathDB = os.path.join(pathExp, "Database")
    pathResult = os.path.join(pathExp, "Result")
    if not os.path.isdir(pathExp):
        os.makedirs(pathExp)
    if not os.path.isdir(pathDB):
        os.makedirs(pathDB)
    if not os.path.isdir(pathResult):
        os.makedirs(pathResult)

    # Create the tracking geometry, uses the json file to configure the proto-surfaces
    matDeco = acts.IMaterialDecorator.fromFile(
        str(os.path.join(args.inputPath, "geometry-map.json"))
    )
    detector = getOpenDataDetector(matDeco)
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    # Use the MappingMaterialDecorator to create a binning map that can be optimised
    matMapDeco = acts.MappingMaterialDecorator(
        tGeometry=trackingGeometry, level=acts.logging.WARNING
    )
    binDict = matMapDeco.binningMap()

    # Create the pipes that will be used to transfer data to/from the jobs
    from multiprocessing import Process, Pipe

    binPipes_child = dict()
    resultPipes_child = dict()
    scorePipes_child = dict()

    binPipes_parent = dict()
    resultPipes_parent = dict()
    scorePipes_parent = dict()

    expJob = dict()
    OptiJob = dict()

    # Build one experiment per surface
    # The binning of the surfaces are independent so we split
    # an optimisation problem with a large number of variable into a lot of optimisation with 2
    for key in binDict:
        binPipes_parent[key], binPipes_child[key] = Pipe()
        scorePipes_parent[key], scorePipes_child[key] = Pipe()
        expJob[key] = Process(
            target=surfaceExperiment,
            args=(
                key,
                args.numberOfJobs,
                pathDB,
                pathResult,
                binPipes_child[key],
                scorePipes_child[key],
                args.doPloting,
            ),
        )
        expJob[key].start()

    # Prepare `args.numberOfJobs` material mapping jobs
    for job in range(args.numberOfJobs):
        resultPipes_parent[job], resultPipes_child[job] = Pipe()
        binMap = dict()
        # Collect the binning for all the surfaces and create a bin map
        for key in binDict:
            binMap[key] = binPipes_parent[key].recv()
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Binning for job "
            + str(job)
            + " have been selected, now running the mapping",
            flush=True,
        )
        # Launch the material mapping with the bin map
        OptiJob[job] = Process(
            target=runMaterialMappingVariance,
            args=(
                binMap,
                args.topNumberOfEvents,
                job,
                args.inputPath,
                pathExp,
                resultPipes_child[job],
                args.readCachedSurfaceInformation,
            ),
        )
        OptiJob[job].start()

    # Collect the score from the material mapping, this pauses the script until all the jobs have been completed
    for job in range(args.numberOfJobs):
        scores = resultPipes_parent[job].recv()
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Retried score for job "
            + str(job),
            flush=True,
        )
        for key in binDict:
            score = scores[key]
            scorePipes_parent[key].send(score)
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Job number "
            + str(job)
            + " is over",
            flush=True,
        )

    if args.doPloting:
        # The optimal binning has been found.
        # Run the material mapping one last to obtain a usable material map
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Running the material mapping to obtain the optimised material map",
            flush=True,
        )
        resultBinMap = dict()
        for key in binDict:
            resultBinMap[key] = binPipes_parent[key].recv()
        matMapDeco.setBinningMap(resultBinMap)

        # Decorate the detector with the MappingMaterialDecorator
        resultDetector, resultTrackingGeometry, resultDecorators = getOpenDataDetector(
            matMapDeco
        )

        # Sequence for the mapping, only use one thread when mapping material
        rMap = acts.examples.Sequencer(
            events=args.topNumberOfEvents, numThreads=1, logLevel=acts.logging.INFO
        )

        # Run the material mapping
        from material_mapping import runMaterialMapping

        runMaterialMapping(
            resultTrackingGeometry,
            resultDecorators,
            outputDir=args.outputPath,
            inputDir=args.inputPath,
            mapName="optimised-material-map",
            format=JsonFormat.Json,
            mapVolume=False,
            s=rMap,
        )
        rMap.run()
    print(
        datetime.now().strftime("%H:%M:%S")
        + "    Waiting for all the score to have been stored",
        flush=True,
    )

    # Create a timer that will be used to terminate the Process
    deadline = time.time() + 600

    for key in binDict:
        timeout = max(deadline - time.time(), 0)
        expJob[key].join(timeout=timeout)
        expJob[key].terminate()
        print(
            datetime.now().strftime("%H:%M:%S")
            + "    Experiment for surface "
            + str(key)
            + " is over",
            flush=True,
        )

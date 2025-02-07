#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import sys
import optuna
import logging
import uproot
import matplotlib
import subprocess
import json
import pandas as pd
from pathlib import Path

matplotlib.use("pdf")

srcDir = Path(__file__).resolve().parent


def run_ckf(params, names, outDir):
    if len(params) != len(names):
        raise Exception("Length of Params must equal names")

    ckf_script = srcDir / "ckf.py"
    nevts = "--nEvents=1"
    indir = "--indir=" + str(srcDir)
    outdir = "--output=" + str(outDir)

    ret = ["python"]
    ret.append(ckf_script)
    ret.append(nevts)
    ret.append(indir)
    ret.append(outdir)

    i = 0
    for param in params:
        arg = "--sf_" + names[i] + "=" + str(param)
        ret.append(arg)
        i += 1

    # Run CKF for the given parameters
    subprocess.call(ret)


class Objective:
    def __init__(self, k_dup, k_time):
        self.res = {
            "eff": [],
            "fakerate": [],
            "duplicaterate": [],
            "runtime": [],
        }

        self.k_dup = k_dup
        self.k_time = k_time

    def __call__(self, trial, ckf_perf=True):
        params = []

        maxSeedsPerSpM = trial.suggest_int("maxSeedsPerSpM", 0, 10)
        params.append(maxSeedsPerSpM)
        cotThetaMax = trial.suggest_float("cotThetaMax", 5.0, 10.0)
        params.append(cotThetaMax)
        sigmaScattering = trial.suggest_float("sigmaScattering", 0.2, 50)
        params.append(sigmaScattering)
        radLengthPerSeed = trial.suggest_float("radLengthPerSeed", 0.001, 0.1)
        params.append(radLengthPerSeed)
        impactMax = trial.suggest_float("impactMax", 0.1, 25)
        params.append(impactMax)
        maxPtScattering = trial.suggest_float("maxPtScattering", 1, 50)
        params.append(maxPtScattering)
        deltaRMin = trial.suggest_float("deltaRMin", 0.25, 30)
        params.append(deltaRMin)
        deltaRMax = trial.suggest_float("deltaRMax", 50, 300)
        params.append(deltaRMax)
        keys = [
            "maxSeedsPerSpM",
            "cotThetaMax",
            "sigmaScattering",
            "radLengthPerSeed",
            "impactMax",
            "maxPtScattering",
            "deltaRMin",
            "deltaRMax",
        ]

        get_tracking_perf(self, ckf_perf, params, keys)

        efficiency = self.res["eff"][-1]
        penalty = (
            self.res["fakerate"][-1]
            + self.res["duplicaterate"][-1] / self.k_dup
            + self.res["runtime"][-1] / self.k_time
        )

        return efficiency - penalty


def get_tracking_perf(self, ckf_perf, params, keys):
    if ckf_perf:
        outDirName = "Output_CKF"
        outputfile = srcDir / outDirName / "performance_ckf.root"
        effContName = "particles"
        contName = "tracks"
    else:
        outDirName = "Output_Seeding"
        outputfile = srcDir / outDirName / "performance_seeding.root"
        effContName = "seeds"
        contName = "seeds"

    outputDir = Path(srcDir / outDirName)
    outputDir.mkdir(exist_ok=True)
    run_ckf(params, keys, outputDir)
    rootFile = uproot.open(outputfile)
    self.res["eff"].append(rootFile["eff_" + effContName].member("fElements")[0])
    self.res["fakerate"].append(rootFile["fakerate_" + contName].member("fElements")[0])
    self.res["duplicaterate"].append(
        rootFile["duplicaterate_" + contName].member("fElements")[0]
    )

    timingfile = srcDir / outDirName / "timing.tsv"
    timing = pd.read_csv(timingfile, sep="\t")

    if ckf_perf:
        time_ckf = float(
            timing[timing["identifier"].str.match("Algorithm:TrackFindingAlgorithm")][
                "time_perevent_s"
            ]
        )

    time_seeding = float(
        timing[timing["identifier"].str.match("Algorithm:SeedingAlgorithm")][
            "time_perevent_s"
        ]
    )
    if ckf_perf:
        self.res["runtime"].append(time_ckf + time_seeding)
    else:
        self.res["runtime"].append(time_seeding)


def main():
    k_dup = 5
    k_time = 5

    # Initializing the objective (score) function
    objective = Objective(k_dup, k_time)

    """
    start_values = {
        "maxSeedsPerSpM": 1,
        "cotThetaMax": 7.40627,
        "sigmaScattering": 50,
        "radLengthPerSeed": 0.1,
        "impactMax": 3.0,
        "maxPtScattering": 10.0,
        "deltaRMin": 1.0,
        "deltaRMax": 60.0
}

    """
    # Optuna logger
    optuna.logging.get_logger("optuna").addHandler(logging.StreamHandler(sys.stdout))
    study_name = "test_study"
    storage_name = "sqlite:///{}.db".format(study_name)

    # creating a new optuna study
    study = optuna.create_study(
        study_name=study_name,
        storage=storage_name,
        direction="maximize",
        load_if_exists=True,
    )

    # study.enqueue_trial(start_values)
    # Start Optimization
    study.optimize(objective, n_trials=3)

    # Printout the best trial values
    print("Best Trial until now", flush=True)
    for key, value in study.best_trial.params.items():
        print(f"    {key}: {value}", flush=True)

    outputDir = Path("OptunaResults")
    outputDir.mkdir(exist_ok=True)

    with open(outputDir / "results.json", "w") as fp:
        json.dump(study.best_params, fp)


if __name__ == "__main__":
    main()

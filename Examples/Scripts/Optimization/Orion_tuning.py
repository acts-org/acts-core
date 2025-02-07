#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os
import matplotlib
import subprocess
from pathlib import Path

from orion.client import build_experiment

from Optuna_tuning import get_tracking_perf, run_ckf

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

    def __call__(
        self,
        maxSeedsPerSpM,
        cotThetaMax,
        sigmaScattering,
        radLengthPerSeed,
        impactMax,
        maxPtScattering,
        deltaRMin,
        deltaRMax,
        ckf_perf=True,
    ):
        params = [
            maxSeedsPerSpM,
            cotThetaMax,
            sigmaScattering,
            radLengthPerSeed,
            impactMax,
            maxPtScattering,
            deltaRMin,
            deltaRMax,
        ]
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

        return [
            {"name": "objective", "type": "objective", "value": -(efficiency - penalty)}
        ]


def main():
    k_dup = 5
    k_time = 5

    # Initializing the objective (score) function
    objective = Objective(k_dup, k_time)

    # Defining the parameter space
    space = {
        "maxSeedsPerSpM": "uniform(0,10,discrete=True)",
        "cotThetaMax": "uniform(5.0,10.0)",
        "sigmaScattering": "uniform(0.2,50.0)",
        "radLengthPerSeed": "uniform(.001,0.1)",
        "impactMax": "uniform(0.1,25.0)",
        "maxPtScattering": "uniform(1.0, 50.0)",
        "deltaRMin": "uniform(0.25, 30.0)",
        "deltaRMax": "uniform(50.0,300.0)",
    }

    # Remove storage file if already exists (conflicts with present run if not deleted)
    if os.path.exists("./db.pkl"):
        os.remove("./db.pkl")

    # location to store metadata
    storage = {
        "type": "legacy",
        "database": {
            "type": "pickleddb",
            "host": "./db.pkl",
        },
    }

    # Build new orion experiment
    experiment = build_experiment(
        "orion_new",
        space=space,
        storage=storage,
    )

    # Start Optimization
    experiment.workon(objective, max_trials=3)

    outputDir = Path("OrionResults")
    outputDir.mkdir(exist_ok=True)

    # fetching trials in a dataframe
    df = experiment.to_pandas()
    df.to_csv(outputDir / "results.txt")

    # Getting the best parameters
    df_imp = df[
        [
            "objective",
            "maxSeedsPerSpM",
            "cotThetaMax",
            "sigmaScattering",
            "radLengthPerSeed",
            "impactMax",
            "maxPtScattering",
            "deltaRMin",
            "deltaRMax",
        ]
    ]
    df_obj = df["objective"]
    min_obj = df_obj.min()
    df_final = df_imp[df_imp["objective"] == min_obj]
    print("Best Score = %s" % (df_final["objective"]))
    print("maxSeedsPerSpM = %s" % (df_final["maxSeedsPerSpM"]))
    print("cotThetaMax = %s" % (df_final["cotThetaMax"]))
    print("sigmaScattering = %s" % (df_final["sigmaScattering"]))
    print("radLengthPerSeed = %s" % (df_final["radLengthPerSeed"]))
    print("impactMax = %s" % (df_final["impactMax"]))
    print("maxPtScattering = %s" % (df_final["maxPtScattering"]))
    print("deltaRMin = %s" % (df_final["deltaRMin"]))
    print("deltaRMax = %s" % (df_final["deltaRMax"]))


if __name__ == "__main__":
    main()

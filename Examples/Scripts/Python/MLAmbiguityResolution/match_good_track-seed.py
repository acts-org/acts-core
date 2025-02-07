# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import glob

import pandas as pd


def matchGood(seed_files: list[str], ckf_files: list[str]):
    """Read the dataset from the tracks and seeds files, then modify the seed dataset so that good seed correspond to the ones that lead to good tracks. Seed with truth id that do not lead to good tracks are considered as fake. Also create a new dataset with only truth particle associated to a good seeds."""
    """
    @param[in] Seed_files: List of files containing seeds data (1 file per events usually)
    @param[in] CKF_files: List of files containing tracks data (1 file per events usually)
    """
    data_seed = pd.DataFrame()
    data_track = pd.DataFrame()
    goodSeed = pd.DataFrame()
    # Loop over the different track files and collect the list of seed ID associated to the good tracks
    for f_ckf, f_seed in zip(ckf_files, seed_files):
        print("reading file: ", f_ckf, f_seed)
        data_track = pd.read_csv(f_ckf)
        data_track = data_track.loc[data_track["good/duplicate/fake"] == "good"]
        goodSeed = data_track["seed_id"]

        data_seed = pd.read_csv(f_seed)
        # Add a good seed column to the seed dataset
        data_seed["goodSeed"] = data_seed["seed_id"].isin(goodSeed)

        data_seed.loc[
            data_seed["good/duplicate/fake"] == "good", "good/duplicate/fake"
        ] = "duplicate"
        data_seed.loc[data_seed["goodSeed"] == True, "good/duplicate/fake"] = "good"

        cleanedData = pd.DataFrame()

        # Find the particle ID that are associated to only fake seeds
        for ID in data_seed["particleId"].unique():
            if (
                data_seed.loc[data_seed["particleId"] == ID, "goodSeed"] == False
            ).all():
                data_seed.loc[data_seed["particleId"] == ID, "good/duplicate/fake"] = (
                    "fake"
                )
            else:
                cleanedData = pd.concat(
                    [data_seed.loc[data_seed["particleId"] == ID], cleanedData]
                )

        # Save the matched dataset for future use (the matching is time consuming)
        matched = f_seed[:-4] + "_matched.csv"
        matchedData = data_seed.sort_values("seed_id")
        matchedData = matchedData.set_index("seed_id")
        matchedData = matchedData.drop(columns=["goodSeed"])
        matchedData.to_csv(matched)

        # Save the cleaned dataset for future use (the cleaning is time consuming)
        cleaned = f_seed[:-4] + "_cleaned.csv"
        cleanedData = cleanedData.sort_values("seed_id")
        cleanedData = cleanedData.set_index("seed_id")
        cleanedData = cleanedData.drop(columns=["goodSeed"])
        cleanedData.to_csv(cleaned)

    return


# Read the seed and track files and match them
# This will allow us to determine which seeds leads to the best possible tracks
seed_files = sorted(glob.glob("odd_output" + "/event*-seed.csv"))
ckf_files = sorted(glob.glob("odd_output" + "/event*-tracks_ckf.csv"))
matchGood(seed_files, ckf_files)

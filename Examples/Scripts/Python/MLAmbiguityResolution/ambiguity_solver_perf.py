# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import glob

import pandas as pd
import numpy as np

from ambiguity_solver_network import prepareDataSet


def readDataSet(CKS_files: list[str]) -> pd.DataFrame:
    """Read the dataset from the different file, remove the pure duplicate tracks and combine the datasets"""
    """
    @param[in] CKS_files: DataFrame contain the data from each track files (1 file per events usually)
    @return: combined DataFrame containing all the track, ordered by events and then by truth particle ID in each event
    """
    data = []
    for f in CKS_files:
        datafile = pd.read_csv(f)
        datafile = prepareDataSet(datafile)
        # Combine dataset
        data.append(datafile)
    return data


# ==================================================================

# CSV files to be compared, do not forget to sort them
CKF_files_track = sorted(
    glob.glob("odd_output" + "/event0000000[0-9][0-9]-tracks_ckf.csv")
)
CKF_files_resolved = sorted(
    glob.glob("odd_output" + "/event0000000[0-9][0-9]-tracks_ambi.csv")
)
ML_files_resolved = sorted(
    glob.glob("odd_output" + "/event0000000[0-9][0-9]-tracks_ambiML.csv")
)

data_track = readDataSet(CKF_files_track)
data_ML_track = readDataSet(CKF_files_track)
data_resolved = readDataSet(CKF_files_resolved)
data_ML_resolved = readDataSet(ML_files_resolved)

# Compute the algorithm performances
nb_part = 0
nb_track = 0
nb_fake = 0
nb_duplicate = 0

nb_good_match = 0
nb_reco_part = 0
nb_reco_fake = 0
nb_reco_duplicate = 0
nb_reco_track = 0

nb_good_match_ML = 0
nb_reco_part_ML = 0
nb_reco_fake_ML = 0
nb_reco_duplicate_ML = 0
nb_reco_track_ML = 0

# Compute the different efficiencies
for trackEvent, resolvedEvent in zip(data_track, data_resolved):
    nb_part += trackEvent.loc[trackEvent["good/duplicate/fake"] == "good"].shape[0]
    nb_track += trackEvent.shape[0]
    nb_fake += trackEvent.loc[trackEvent["good/duplicate/fake"] == "fake"].shape[0]
    nb_duplicate += trackEvent.loc[
        trackEvent["good/duplicate/fake"] == "duplicate"
    ].shape[0]

    # Merge two dataFrames and add indicator column
    merged = pd.merge(
        trackEvent.loc[trackEvent["good/duplicate/fake"] == "good"],
        resolvedEvent,
        on=[
            "particleId",
            "nStates",
            "nMeasurements",
            "nOutliers",
            "nHoles",
            "ndf",
            "chi2/ndf",
            "good/duplicate/fake",
        ],
        how="left",
        indicator="exists",
    )
    # Add column to show if each row in first DataFrame exists in second
    merged["exists"] = np.where(merged.exists == "both", True, False)
    merged.to_csv(path_or_buf="merged.csv")

    nb_good_match += merged.loc[merged["exists"] == True].shape[0]
    nb_reco_fake += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] == "fake"
    ].shape[0]
    nb_reco_duplicate += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] == "duplicate"
    ].shape[0]
    nb_reco_part += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] != "fake"
    ].index.nunique()
    nb_reco_track += resolvedEvent.shape[0]

# Compute the different efficiencies for ML
for trackEvent, resolvedEvent in zip(data_ML_track, data_ML_resolved):
    # Merge two dataFrames and add indicator column
    merged_ML = pd.merge(
        trackEvent.loc[trackEvent["good/duplicate/fake"] == "good"],
        resolvedEvent,
        on=[
            "particleId",
            "nStates",
            "nMeasurements",
            "nOutliers",
            "nHoles",
            "ndf",
            "chi2/ndf",
            "good/duplicate/fake",
        ],
        how="left",
        indicator="exists",
    )

    # Add column to show if each row in first DataFrame exists in second
    merged_ML["exists"] = np.where(merged_ML.exists == "both", True, False)
    merged_ML.to_csv(path_or_buf="merged_ML.csv")

    nb_good_match_ML += merged_ML.loc[merged_ML["exists"] == True].shape[0]
    nb_reco_fake_ML += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] == "fake"
    ].shape[0]
    nb_reco_duplicate_ML += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] == "duplicate"
    ].shape[0]
    nb_reco_part_ML += resolvedEvent.loc[
        resolvedEvent["good/duplicate/fake"] != "fake"
    ].index.nunique()
    nb_reco_track_ML += resolvedEvent.shape[0]

print("===Initial efficiencies===")
print("nb particles : ", nb_part)
print("nb track : ", nb_track)
print("duplicate rate: ", 100 * nb_duplicate / nb_track, " %")
print("Fake rate: ", 100 * nb_fake / nb_track, " %")

print("===computed efficiencies Greedy===")
print("nb particles : ", nb_part)
print("nb good match : ", nb_good_match)
print("nb particle reco : ", nb_reco_part)
print("nb track reco : ", nb_reco_track)
print("Efficiency (good track) : ", 100 * nb_good_match / nb_part, " %")
print("Efficiency (particle reco) : ", 100 * nb_reco_part / nb_part, " %")
print("duplicate rate: ", 100 * nb_reco_duplicate / nb_reco_track, " %")
print("Fake rate: ", 100 * nb_reco_fake / nb_reco_track, " %")

print("===computed efficiencies ML===")
print("nb particles: ", nb_part)
print("nb good match: ", nb_good_match_ML)
print("nb particle reco: ", nb_reco_part_ML)
print("nb track reco: ", nb_reco_track_ML)
print("Efficiency (good track): ", 100 * nb_good_match_ML / nb_part, " %")
print("Efficiency (particle reco): ", 100 * nb_reco_part_ML / nb_part, " %")
print("duplicate rate: ", 100 * nb_reco_duplicate_ML / nb_reco_track_ML, " %")
print("Fake rate: ", 100 * nb_reco_fake_ML / nb_reco_track_ML, " %")

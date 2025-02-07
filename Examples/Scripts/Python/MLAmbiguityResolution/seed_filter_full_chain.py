# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import glob

import pandas as pd
import numpy as np

import torch.utils

from sklearn.cluster import DBSCAN, KMeans

from sklearn.preprocessing import LabelEncoder, OrdinalEncoder
from seed_solver_network import prepareDataSet


def readDataSet(CKS_files: list[str]) -> pd.DataFrame:
    """Read the dataset from the different files, remove the pure duplicate tracks and combine the datasets"""
    """
    @param[in] CKS_files: DataFrame contain the data from each track files (1 file per events usually)
    @return: combined DataFrame containing all the track, ordered by events and then by truth particle ID in each events
    """
    data = []
    for f in CKS_files:
        datafile = pd.read_csv(f)
        datafile = prepareDataSet(datafile)
        data.append(datafile)
    return data


def prepareInferenceData(data: pd.DataFrame) -> tuple[np.ndarray, np.ndarray]:
    """Prepare the data"""
    """
    @param[in] data: input DataFrame to be prepared
    @return: array of the network input and the corresponding truth
    """
    # Remove truth and useless variable
    target_column = "good/duplicate/fake"
    # Separate the truth from the input variables
    y = LabelEncoder().fit(data[target_column]).transform(data[target_column])
    input = data.drop(
        columns=[
            target_column,
            "seed_id",
            "Hits_ID",
            "cluster",
        ]
    )
    # Prepare the input feature
    x_cat = OrdinalEncoder().fit_transform(input.select_dtypes("object"))
    x = np.concatenate((x_cat, input), axis=1)
    return x, y


def clusterSeed(
    event: pd.DataFrame, DBSCAN_eps: float = 0.03, DBSCAN_min_samples: int = 2
) -> pd.DataFrame:
    """
    Cluster together all the track that appear to belong to the same truth particle
    To cluster the tracks together, a DBSCAN is first used followed by a sub clustering based on hits shared by tracks.
    """
    """
    @param[in] event: input DataFrame that contain all track in one event
    @param[in] DBSCAN_eps: minimum radius used by the DBSCAN to cluster track together
    @param[in] DBSCAN_min_samples: minimum number of tracks needed for DBSCAN to create a cluster
    @return: DataFrame identical to the output with an added column with the cluster
    """
    # Perform the DBSCAN clustering and sort the Db by cluster ID
    trackDir = event[["eta", "phi", "vertexZ", "pT"]].to_numpy()
    trackDir[:, 2] = trackDir[:, 2] / 50
    # Perform the subclustering
    clustering = DBSCAN(eps=DBSCAN_eps, min_samples=DBSCAN_min_samples).fit(trackDir)
    clusterarray = renameCluster(clustering.labels_)
    event["cluster"] = clusterarray
    sorted = event.sort_values(["cluster"], ascending=True)
    return sorted


def renameCluster(clusterarray: np.ndarray) -> np.ndarray:
    """Rename the cluster IDs to be int starting from 0"""
    """
    @param[in] clusterarray: numpy array containing the hits IDs and the cluster ID
    @return: numpy array with updated cluster IDs
    """
    new_id = len(set(clusterarray)) - (1 if -1 in clusterarray else 0)
    for i, cluster in enumerate(clusterarray):
        if cluster == -1:
            clusterarray[i] = new_id
            new_id = new_id + 1
    return clusterarray


# ==================================================================

import time

start = time.time()

# ttbar events as test input
CKF_files = sorted(glob.glob("odd_output" + "/event0000000[0-1][0-9]-seed_matched.csv"))
data = readDataSet(CKF_files)

# Data of each event after clustering
clusteredData = []
# Data of each event after ambiguity resolution
cleanedData = []

t1 = time.time()

# Cluster tracks belonging to the same particle
for event in data:
    clustered = clusterSeed(event)
    clusteredData.append(clustered)

t2 = time.time()

duplicateClassifier = torch.load("seedduplicateClassifier.pt")

import matplotlib.pyplot as plt

# Make a copy of the data to be plotted
plotData = []
plotDF = pd.DataFrame()
for event in clusteredData:
    plotData.append(event.copy())
    plotDF = pd.concat([plotDF, event.copy()])

# Plot the distribution of the 4 variable
plotDF["eta"].hist(bins=100)
plt.xlabel("eta")
plt.ylabel("nb seed")
plt.savefig("eta.png")
plt.clf()

plotDF["phi"].hist(bins=100)
plt.xlabel("phi")
plt.ylabel("nb seed")
plt.savefig("phi.png")
plt.clf()

plotDF["vertexZ"].hist(bins=100)
plt.xlabel("vertexZ")
plt.ylabel("nb seed")
plt.savefig("vertexZ.png")
plt.clf()

plotDF["pT"].hist(bins=100, range=[0, 10])
plt.xlabel("pT")
plt.ylabel("nb seed")
plt.savefig("pT.png")
plt.clf()

plotDF.plot.scatter(x="eta", y="pT")
plt.xlabel("eta")
plt.ylabel("pT")
plt.savefig("pT_eta.png")
plt.clf()


plotDF2 = pd.DataFrame()
# Create histogram filled with the number of seeds per cluster
for event in plotData:
    event["nb_seed"] = 0
    event["nb_fake"] = 0
    event["nb_duplicate"] = 0
    event["nb_good"] = 0
    event["nb_cluster"] = 0
    event["nb_truth"] = 0
    event["nb_seed_truth"] = 0
    event["nb_seed_removed"] = 0
    event["particleId"] = event.index
    event["nb_seed"] = event.groupby(["cluster"])["cluster"].transform("size")
    # Create histogram filled with the number of fake seeds per cluster
    event.loc[event["good/duplicate/fake"] == "fake", "nb_fake"] = (
        event.loc[event["good/duplicate/fake"] == "fake"]
        .groupby(["cluster"])["cluster"]
        .transform("size")
    )
    # Create histogram filled with the number of duplicate seeds per cluster
    event.loc[event["good/duplicate/fake"] == "duplicate", "nb_duplicate"] = (
        event.loc[event["good/duplicate/fake"] == "duplicate"]
        .groupby(["cluster"])["cluster"]
        .transform("size")
    )
    # Create histogram filled with the number of good seeds per cluster
    event.loc[event["good/duplicate/fake"] == "good", "nb_good"] = (
        event.loc[event["good/duplicate/fake"] == "good"]
        .groupby(["cluster"])["cluster"]
        .transform("size")
    )

    plotDF2 = pd.concat([plotDF2, event])

plotDF2["nb_seed"].hist(bins=20, weights=1 / plotDF2["nb_seed"], range=[0, 20])
plt.xlabel("nb seed/[cluster]")
plt.ylabel("Arbitrary unit")
plt.savefig("nb_seed.png")
plt.clf()

plotDF2["nb_fake"].hist(bins=10, weights=1 / plotDF2["nb_seed"], range=[0, 10])
plt.xlabel("nb fake/[cluster]")
plt.ylabel("Arbitrary unit")
plt.savefig("nb_fake.png")
plt.clf()

plotDF2["nb_duplicate"].hist(bins=10, weights=1 / plotDF2["nb_seed"], range=[0, 10])
plt.xlabel("nb duplicate/[cluster]")
plt.ylabel("Arbitrary unit")
plt.savefig("nb_duplicate.png")
plt.clf()

plotDF2["nb_good"].hist(bins=5, weights=1 / plotDF2["nb_seed"], range=[0, 5])
plt.xlabel("nb good/[cluster]")
plt.ylabel("Arbitrary unit")
plt.savefig("nb_good.png")
plt.clf()

t3 = time.time()

# Performed the MLP based ambiguity resolution
for clusteredEvent in clusteredData:
    # Prepare the data
    x_test, y_test = prepareInferenceData(clusteredEvent)
    x = torch.tensor(x_test, dtype=torch.float32)
    output_predict = duplicateClassifier(x).detach().numpy()

    # Create an array of random value between 0 and 1 of the same size as the output
    # output_predict = np.random.rand(len(x_test))

    clusteredEvent["score"] = output_predict
    # Keep only the track in cluster of more than 1 track or with a score above 0.5
    idx = clusteredEvent["score"] > 0.1
    cleanedEvent = clusteredEvent[idx]
    # For each cluster only keep the seed with the highest score
    idx = (
        cleanedEvent.groupby(["cluster"])["score"].transform(max)
        == cleanedEvent["score"]
    )
    cleanedEvent = cleanedEvent[idx]
    # For cluster with more than 1 seed, keep the one with the smallest seed_id
    idx = (
        cleanedEvent.groupby(["cluster"])["seed_id"].transform(min)
        == cleanedEvent["seed_id"]
    )
    cleanedEvent = cleanedEvent[idx]
    cleanedData.append(cleanedEvent)

t4 = time.time()

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

for clusteredEvent, cleanedEvent in zip(clusteredData, cleanedData):
    nb_part += clusteredEvent.loc[
        clusteredEvent["good/duplicate/fake"] != "fake"
    ].index.nunique()
    nb_track += clusteredEvent.shape[0]
    nb_fake += clusteredEvent.loc[
        clusteredEvent["good/duplicate/fake"] == "fake"
    ].shape[0]
    nb_duplicate += clusteredEvent.loc[
        clusteredEvent["good/duplicate/fake"] == "duplicate"
    ].shape[0]

    nb_good_match += cleanedEvent.loc[
        cleanedEvent["good/duplicate/fake"] == "good"
    ].shape[0]
    nb_reco_fake += cleanedEvent.loc[
        cleanedEvent["good/duplicate/fake"] == "fake"
    ].shape[0]
    nb_reco_duplicate += cleanedEvent.loc[
        cleanedEvent["good/duplicate/fake"] == "duplicate"
    ].shape[0]
    nb_reco_part += cleanedEvent.loc[
        cleanedEvent["good/duplicate/fake"] != "fake"
    ].index.nunique()
    nb_reco_track += cleanedEvent.shape[0]

tend = time.time()

print("===Initial efficiencies===")
print("nb particles: ", nb_part)
print("nb track: ", nb_track)
print("duplicate rate: ", 100 * nb_duplicate / nb_track, " %")
print("Fake rate: ", 100 * nb_fake / nb_track, " %")

print("===computed efficiencies===")
print("nb particles: ", nb_part)
print("nb good match: ", nb_good_match)
print("nb particle reco: ", nb_reco_part)
print("nb track reco: ", nb_reco_track)
print("Efficiency (good track): ", 100 * nb_good_match / nb_part, " %")
print("Efficiency (particle reco): ", 100 * nb_reco_part / nb_part, " %")
print(
    "duplicate rate: ",
    100 * ((nb_good_match + nb_reco_duplicate) - nb_reco_part) / nb_reco_track,
    " %",
)
print("Fake rate: ", 100 * nb_reco_fake / nb_reco_track, " %")

print("===computed speed===")
print("Load: ", (t1 - start) * 1000 / len(CKF_files), "ms")
print("Clustering: ", (t2 - t1) * 1000 / len(CKF_files), "ms")
print("Inference: ", (t4 - t3) * 1000 / len(CKF_files), "ms")
print("Perf: ", (tend - t4) * 1000 / len(CKF_files), "ms")
print("tot: ", (t4 - start) * 1000 / len(CKF_files), "ms")
print("Seed filter: ", (t4 - t1) * 1000 / len(CKF_files), "ms")


# ==================================================================
# Plotting

# Combine the events to have a better statistic
clusteredDataPlots = pd.concat(clusteredData)

cleanedDataPlots = pd.concat(cleanedData)
# cleanedDataPlots = cleanedData[0]

import matplotlib.pyplot as plt

# Plot the average score distribution for each type of track
plt.figure()
for tag in ["good", "duplicate", "fake"]:
    weights = np.ones_like(
        cleanedDataPlots.loc[cleanedDataPlots["good/duplicate/fake"] == tag]["score"]
    ) / len(
        cleanedDataPlots.loc[cleanedDataPlots["good/duplicate/fake"] == tag]["score"]
    )
    plt.hist(
        cleanedDataPlots.loc[cleanedDataPlots["good/duplicate/fake"] == tag]["score"],
        bins=100,
        weights=weights,
        alpha=0.65,
        label=tag,
    )
plt.legend()
plt.xlabel("score")
plt.ylabel("Fraction of good/duplicate/fake tracks")
plt.title("Score distribution for each type of track")
plt.savefig("score_distribution.png")
plt.yscale("log")
plt.savefig("score_distribution_log.png")

# Average value of the score
averageCleanedDataPlots = cleanedDataPlots.loc[
    cleanedDataPlots["good/duplicate/fake"] == "good"
].groupby(
    pd.cut(
        cleanedDataPlots.loc[cleanedDataPlots["good/duplicate/fake"] == "good"]["eta"],
        np.linspace(-3, 3, 100),
    )
)
plt.figure()
plt.plot(
    np.linspace(-3, 3, 99),
    averageCleanedDataPlots["score"].mean(),
    label="average score",
)
plt.legend()
plt.xlabel("eta")
plt.ylabel("score")
plt.title("Average score for each eta bin")
plt.savefig("score_eta.png")

# Plot the pT distribution for each type of track
plt.figure()
plt.hist(
    [
        clusteredDataPlots.loc[clusteredDataPlots["good/duplicate/fake"] == "good"][
            "pT"
        ],
        clusteredDataPlots.loc[
            clusteredDataPlots["good/duplicate/fake"] == "duplicate"
        ]["pT"],
        clusteredDataPlots.loc[clusteredDataPlots["good/duplicate/fake"] == "fake"][
            "pT"
        ],
    ],
    bins=100,
    range=(0, 100),
    stacked=False,
    label=["good", "duplicate", "fake"],
)
plt.legend()
plt.xlabel("pT")
plt.ylabel("number of tracks")
plt.yscale("log")
plt.title("pT distribution for each type of track")
plt.savefig("pT_distribution.png")

# Plot the eta distribution for each type of track
plt.figure()
plt.hist(
    [
        clusteredDataPlots.loc[clusteredDataPlots["good/duplicate/fake"] == "good"][
            "eta"
        ],
        clusteredDataPlots.loc[
            clusteredDataPlots["good/duplicate/fake"] == "duplicate"
        ]["eta"],
        clusteredDataPlots.loc[clusteredDataPlots["good/duplicate/fake"] == "fake"][
            "eta"
        ],
    ],
    bins=100,
    range=(-3, 3),
    stacked=False,
    label=["good", "duplicate", "fake"],
)
plt.legend()
plt.xlabel("eta")
plt.ylabel("number of tracks")
plt.yscale("log")
plt.title("eta distribution for each type of track")
plt.savefig("eta_distribution.png")

# Average value of the score for 50 pt bins
averageCleanedDataPlots = cleanedDataPlots.loc[
    cleanedDataPlots["good/duplicate/fake"] == "good"
].groupby(
    pd.cut(
        cleanedDataPlots.loc[cleanedDataPlots["good/duplicate/fake"] == "good"]["pT"],
        np.linspace(0, 100, 50),
    )
)
plt.figure()
plt.plot(
    np.linspace(0, 100, 49),
    averageCleanedDataPlots["score"].mean(),
    label="average score",
)
plt.legend()
plt.xlabel("pT")
plt.ylabel("score")
plt.title("Average score for each eta bin")
plt.savefig("score_pt.png")

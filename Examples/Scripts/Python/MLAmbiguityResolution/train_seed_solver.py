# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import glob

import pandas as pd
import numpy as np

import torch.nn as nn
import torch.nn.functional as F
import torch.utils
from torch.utils.tensorboard import SummaryWriter

from sklearn.preprocessing import LabelEncoder, StandardScaler, OrdinalEncoder

from seed_solver_network import (
    prepareDataSet,
    DuplicateClassifier,
    Normalise,
)

avg_mean = [0] * 14
avg_sdv = [0] * 14
events = 0
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")


def readDataSet(Seed_files: list[str]) -> pd.DataFrame:
    """Read the dataset from the different files, remove the particle with only fakes and combine the datasets"""
    """
    @param[in] Seed_files: DataFrame contain the data from each seed files (1 file per events usually)
    @return: combined DataFrame containing all the seed, ordered by events and then by truth particle ID in each events
    """
    data = pd.DataFrame()
    for f in Seed_files:
        datafile = pd.read_csv(f)
        datafile = prepareDataSet(datafile)
        data = pd.concat([data, datafile])
    return data


def prepareTrainingData(data: pd.DataFrame) -> tuple[np.ndarray, np.ndarray]:
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
        ]
    )
    # Compute the normalisation factors
    scale = StandardScaler()
    scale.fit(input.select_dtypes("number"))
    # Variables to compute the normalisation
    global avg_mean
    avg_mean = avg_mean + scale.mean_
    global avg_sdv
    avg_sdv = avg_sdv + scale.var_
    global events
    events = events + 1
    # Prepare the input feature
    x_cat = OrdinalEncoder().fit_transform(input.select_dtypes("object"))
    x = np.concatenate((x_cat, input), axis=1)
    return x, y


def batchSplit(data: pd.DataFrame, batch_size: int) -> list[pd.DataFrame]:
    """Split the data into batch each containing @batch_size truth particles (the number of corresponding seeds may vary)"""
    """
    @param[in] data: input DataFrame to be cut into batch
    @param[in] batch_size: Number of truth particles per batch
    @return: list of DataFrame, each element correspond to a batch
    """
    batch = []
    pid = data[0][0]
    n_particle = 0
    id_prev = 0
    id = 0
    for index, row, truth in zip(data[0], data[1], data[2]):
        if index != pid:
            pid = index
            n_particle += 1
            if n_particle == batch_size:
                b = data[0][id_prev:id], data[1][id_prev:id], data[2][id_prev:id]
                batch.append(b)
                n_particle = 0
                id_prev = id
        id += 1
    return batch


def computeLoss(
    score_good: torch.Tensor,
    score_duplicate: list[torch.Tensor],
    score_fake: list[torch.Tensor],
    batch_loss: torch.Tensor,
    margin_duplicate: float = 0.3,
    margin_fake: float = 0.9,
) -> torch.Tensor:
    """Compute one loss for each duplicate seed associated with the particle"""
    """
    @param[in] score_good: score return by the model for the good seed associated with this particle
    @param[in] score_duplicate: list of the scores of all duplicate seed associated with this particle
    @param[in] margin_duplicate: Margin used in the computation of the MarginRankingLoss for duplicate seeds
    @param[in] margin_fake: Margin used in the computation of the MarginRankingLoss for fake seeds
    @return: return the updated loss
    """
    # Compute the losses using the MarginRankingLoss with respect to the good seed score
    batch_loss = batch_loss
    if score_duplicate:
        for s in score_duplicate:
            batch_loss += F.relu(s - score_good + margin_duplicate) / (
                len(score_duplicate) + len(score_fake) + 1
            )
    if score_fake:
        for s in score_fake:
            batch_loss += F.relu(s - score_good + margin_fake) / (
                len(score_duplicate) + len(score_fake) + 1
            )
    batch_loss += margin_fake / (len(score_duplicate) + len(score_fake) + 1)

    return batch_loss


def scoringBatch(batch: list[pd.DataFrame], Optimiser=0) -> tuple[int, int, float]:
    """Run the MLP on a batch and compute the corresponding efficiency and loss. If an optimiser is specify train the MLP."""
    """
    @param[in] batch:  list of DataFrame, each element correspond to a batch
    @param[in] Optimiser: Optimiser for the MLP, if one is specify the network will be train on batch.
    @return: array containing the number of particles, the number of particle where the good seed was found and the loss
    """
    # number of particles
    nb_part = 0
    # number of particles associated with a good seed
    nb_good_match = 0
    # number of particles associated with a best seed
    nb_best_match = 0
    # loss for the batch
    loss = 0
    # best seed score for a particle
    max_score = 0
    # is the best score associated with the good seed
    max_match = 1
    # loop over all the batch
    for b_data in batch:
        # ID of the current particle
        pid = b_data[0][0]
        # loss for the current batch
        batch_loss = 0
        # score of the good seed
        score_good = 1
        # score of the duplicate seeds
        score_duplicate = []
        # score of the fake seeds
        score_fake = []

        if Optimiser:
            Optimiser.zero_grad()
        input = torch.tensor(b_data[1], dtype=torch.float32)
        input = input.to(device)
        prediction = duplicateClassifier(input)
        # loop over all the seed in the batch
        for index, pred, truth in zip(b_data[0], prediction, b_data[2]):
            # If we are changing particle update the loss
            if index != pid:
                # Starting a new particles, compute the loss for the previous one
                if max_match == 0 or max_match == 2:
                    nb_good_match += 1
                if max_match == 2:
                    nb_best_match += 1
                batch_loss = computeLoss(
                    score_good,
                    score_duplicate,
                    score_fake,
                    batch_loss,
                    margin_duplicate=0.2,
                    margin_fake=0.4,
                )
                nb_part += 1
                # Reinitialise the variable for the next particle
                pid = index
                score_duplicate = []
                score_fake = []
                score_good = 1
                max_score = 0
                max_match = 1
            # Store seed scores
            if truth == 2:
                score_good = pred
            elif truth == 0:
                score_duplicate.append(pred)
            else:
                score_fake.append(pred)
            # Prepare efficiency computation
            if pred > max_score:
                max_score = pred
                max_match = truth
        # Compute the loss for the last particle when reaching the end of the batch
        if max_match == 0 or max_match == 2:
            nb_good_match += 1
        if max_score == 2:
            nb_best_match += 1
        batch_loss = computeLoss(
            score_good,
            score_duplicate,
            score_fake,
            batch_loss,
            margin_duplicate=0.2,
            margin_fake=0.4,
        )
        nb_part += 1
        # Normalise the loss to the batch size
        batch_loss = batch_loss / len(b_data[0])
        loss += batch_loss.item()
        # Perform the gradient descent if an optimiser was specified
        if Optimiser:
            batch_loss.backward()
            Optimiser.step()
    loss = loss / len(batch)
    return nb_part, nb_good_match, nb_best_match, loss


def train(
    duplicateClassifier: DuplicateClassifier,
    data: tuple[np.ndarray, np.ndarray, np.ndarray],
    epochs: int = 20,
    batch: int = 32,
    validation: float = 0.3,
) -> DuplicateClassifier:
    """Training of the MLP"""
    """
    @param[in] duplicateClassifier: model to be trained.
    @param[in] data: tuple containing three list. Each element of those list correspond to a given seed and represent : the truth particle ID, the seed parameters and the truth.
    @param[in] epochs: number of epoch the model will be trained for.
    @param[in] batch: size of the batch used in the training
    @param[in] validation: Fraction of the batch used in training
    @return: trained model
    """
    # Prepare tensorboard for the training plot
    # use 'tensorboard --logdir=runs' to access the plot afterward
    writer = SummaryWriter()
    opt = torch.optim.Adam(duplicateClassifier.parameters())
    # Split the data in batch
    batch = batchSplit(data, batch)
    val_batch = int(len(batch) * (1 - validation))
    # Loop over all the epoch
    for epoch in range(epochs):
        print("Epoch: ", epoch, " / ", epochs)
        loss = 0.0
        nb_part = 0.0
        nb_good_match = 0.0

        # Loop over all the network over the training batch
        nb_part, nb_good_match, nb_best_match, loss = scoringBatch(
            batch[:val_batch], Optimiser=opt
        )
        print(
            "Loss/train: ",
            loss,
            " Eff/train: ",
            nb_good_match / nb_part,
            " Eff_best/train: ",
            nb_best_match / nb_part,
        )
        writer.add_scalar("Loss/train", loss, epoch)
        writer.add_scalar("Eff/train", nb_good_match / nb_part, epoch)
        writer.add_scalar("Eff_best/train", nb_best_match / nb_part, epoch)

        # If using validation, compute the efficiency and loss over the training batch
        if validation > 0.0:
            nb_part, nb_good_match, nb_best_match, loss = scoringBatch(
                batch[val_batch:]
            )
            writer.add_scalar("Loss/val", loss, epoch)
            writer.add_scalar("Eff/val", nb_good_match / nb_part, epoch)
            writer.add_scalar("Eff_best/train", nb_best_match / nb_part, epoch)
            print(
                "Loss/val: ",
                loss,
                " Eff/val: ",
                nb_good_match / nb_part,
                " Eff_best/val: ",
                nb_best_match / nb_part,
            )

    writer.close()
    return duplicateClassifier


# ==================================================================

# ttbar events used as the training input, here we assume 1000 events are available
CKF_files = sorted(
    glob.glob("odd_output" + "/event000000[0-9][0-9][0-9]-seed_cleaned.csv")
)
data = readDataSet(CKF_files)
# Prepare the data
x_train, y_train = prepareTrainingData(data)

avg_mean = [x / events for x in avg_mean]
avg_sdv = [x / events for x in avg_sdv]

# Create our model and chose the layers sizes
input_dim = np.shape(x_train)[1]
layers_dim = [80, 80, 100, 80, 80]

duplicateClassifier = nn.Sequential(
    Normalise(avg_mean, avg_sdv), DuplicateClassifier(input_dim, layers_dim)
)
duplicateClassifier = duplicateClassifier.to(device)

# Train the model and save it
input = data.index, x_train, y_train
train(duplicateClassifier, input, epochs=30, batch=128, validation=0.3)
duplicateClassifier.eval()
input_test = torch.tensor(x_train, dtype=torch.float32)
torch.save(duplicateClassifier, "seedduplicateClassifier.pt")
torch.onnx.export(
    duplicateClassifier,
    input_test[0:1],
    "seedduplicateClassifier.onnx",
    input_names=["x"],
    output_names=["y"],
    dynamic_axes={"x": {0: "batch_size"}, "y": {0: "batch_size"}},
)

# ==================================================================

# ttbar events for the test, here we assume 40 events are availables
CKF_files_test = sorted(
    glob.glob("odd_output" + "/event000001[0-0][0-9][0-9]-seed_cleaned.csv")
)

test = readDataSet(CKF_files_test)

# Prepare the data
x_test, y_test = prepareTrainingData(test)

# Write the network score to a list
output_predict = []

model = torch.load("seedduplicateClassifier.pt")

x_test = torch.tensor(x_test, dtype=torch.float32)
x_test = x_test.to(device)
for x in x_test:
    output_predict.append(model(x))

# For the first 100 particles print the ID, score and truth
for sample_test, sample_predict, sample_true in zip(
    test.index[0:100], output_predict[0:100], y_test[0:100]
):
    print(sample_test, sample_predict, sample_true)

id = 0
pid = test.index[0]
nb_part = 0
nb_good_match = 0
nb_best_match = 0
max_match = 1
max_score = 0

# Compute the efficiency
for index, pred, truth in zip(test.index, output_predict, y_test):
    if index != pid:
        nb_part += 1
        if max_match == 0 or max_match == 2:
            nb_good_match += 1
        if max_match == 2:
            nb_best_match += 1
        pid = index
        max_match = 1
        max_score = 0

    if pred > max_score:
        max_score = pred
        max_match = truth

print("nb particles: ", nb_part)
print("nb good match: ", nb_good_match)
print("nb best match: ", nb_best_match)
print("Efficiency: ", 100 * nb_good_match / nb_part, " %")
print("Efficiency_best: ", 100 * nb_best_match / nb_part, " %")

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import pandas as pd

import torch.nn as nn
import torch.nn.functional as F
import torch.utils

import ast


def prepareDataSet(data: pd.DataFrame) -> pd.DataFrame:
    """Format the dataset that have been written from the Csv file"""
    """
    @param[in] data: input DataFrame containing 1 event
    @return: Formatted DataFrame
    """
    # Sort by particle ID
    data = data.sort_values("particleId")
    # Set truth particle ID as index
    data = data.set_index("particleId")
    # Transform the hit list from a string to an actual list
    hitsIds = []
    for list in data["Hits_ID"].values:
        hitsIds.append(ast.literal_eval(list))
    data["Hits_ID"] = hitsIds
    # Combine dataset
    return data


class DuplicateClassifier(nn.Module):
    """MLP model used to separate goods seed from duplicate seeds. Return one score per seed the higher one correspond to the good seed."""

    def __init__(self, input_dim, n_layers):
        """Four layer MLP, sigmoid activation for the last layer."""
        super(DuplicateClassifier, self).__init__()
        self.linear1 = nn.Linear(input_dim, n_layers[0])
        self.linear2 = nn.Linear(n_layers[0], n_layers[1])
        self.linear3 = nn.Linear(n_layers[1], n_layers[2])
        self.linear4 = nn.Linear(n_layers[2], n_layers[3])
        self.linear5 = nn.Linear(n_layers[3], n_layers[4])
        self.output = nn.Linear(n_layers[4], 1)
        self.sigmoid = nn.Sigmoid()

    def forward(self, z):
        z = F.relu(self.linear1(z))
        z = F.relu(self.linear2(z))
        z = F.relu(self.linear3(z))
        z = F.relu(self.linear4(z))
        z = F.relu(self.linear5(z))
        return self.sigmoid(self.output(z))


class Normalise(nn.Module):
    """Normalisation of the input before the MLP model."""

    def __init__(self, mean, std):
        super(Normalise, self).__init__()
        self.mean = torch.tensor(mean, dtype=torch.float32)
        self.std = torch.tensor(std, dtype=torch.float32)

    def forward(self, z):
        z = z - self.mean
        z = z / self.std
        return z

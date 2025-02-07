<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Machine learning algorithms

ACTS allows you to replace some components of the tracking chain with machine-learning solutions.
A replacement for the ambiguity solver and a filtering algorithm for seeds are available for now, but when others are implemented, they will be explained here.

## Onnx plugin

To be able to perform neural network (NN) models' inferences in C++, ACTS uses [ONNX Runtime](https://onnxruntime.ai/). An interface to use ONNX Runtime has been implemented as an ACTS plugin; to use it, you will need to compile ACTS with the `ACTS_PLUGIN_ONNX` option. For more details on how to export your model to ONNX, please see the documentation on their [website](https://onnxruntime.ai/docs/).

### OnnxRuntimeBase

The `OnnxRuntimeBase` class implements the inference of a standard MLP via ONNX. The class just requires a link to the `.onnx` file containing the model one wants to use. Please note, that the implementation of the inference in ACTS only works with a single input node and a single output node. The inference can be performed in single entry and batch modes. In single entry mode, the `runONNXInference` method takes a single vector as an entry, each element of the vector corresponding to one of the input features of your network. In batch mode, the input is an `Eigen::Array` with the columns corresponding to the input features and rows to the different batch inputs.

## AmbiguityResolutionMLAlgorithm

The goal of the ambiguity solver is to remove duplicated and fake tracks that remain after the CKF. To perform this cleaning, this algorithm works in three steps:

- Clustering: tracks are clustered together, one cluster ~ one truth particle
- Ranking: tracks in each cluster are scored, the best one is kept
- Cleaning: last pass over all the remaining tracks to remove duplicate and fake (not implemented yet)

### Clustering

The clustering is implemented with the `clusterTracks` function. Its input is a multimap of a pair of track IDs and a vector of measurement IDs. The multimap uses the number of measurements associated with the tracks as a key, which is only a trick to sort the tracks efficiently by the number of measurements. Then, for each track, starting with the one with the most measurements, we check if a cluster shares a hit with the track. If not, we create a new cluster and associate all the hits of the current track with the cluster. If yes, the track is added to that cluster (note that the hits associated with the cluster don’t change here). After looping over all the tracks, each should have been associated with a cluster.

### Ranking

At this step, we have multiple clusters of tracks. We use a NN to compute a score for each track; the closer the score is to 1, the more confident we are that the track is the best (the best one associated with the truth particle). Then, for each cluster, we select the track with the highest score.

### Cleaning

Finally, for the remaining tracks, there might be some fakes and duplicates. It does not seem to be the case with the first tests, so the current implementation stops here. But in the future, if some configuration encounters too many fake/duplicates after the ranking, a simple classification NN could be used to separate the good tracks from the others.

Running the ACTS Greedy Solver after the ML Solver can be helpful in some cases; it will likely remove most of the remaining fakes while being extremely fast and not affecting the reconstruction performances (since both algorithms are orthogonal). The default implementation in the full ODD chain uses this approach.

### How to use

The ML-based Ambiguity solver comes with a pre-trained model to be used with the ODD. Using it is extremely simple; just call the `full_chain_odd.py` with the `--MLSolver` option. If you want to try this solver with another detector, a few more steps are needed. First, you need to train a model. As an input, you will need the multitrajectory output from the CKF (run a full chain up to the CKF, then use the `CSVWriter`). At least 100 ttbar events with 200 PU are needed (1000 would be ideal). You can then run `Examples/Scripts/Python/MLAmbiguityResolution/train_ambiguity_solver.py` on our dataset, 2/3 of the data should be used for the training and 1/3 for the validation. You will receive a `duplicateClassifier.onnx` file as an output, which can now be used as an input for the `AmbiguityResolutionMLAlgorithm` with your detector.

Additionally, two other Python files are present with the training.
- `ambiguity_solver_full_chain.py` performs the same job as the chain we presented here but in Python using CSV files as input.
- `ambiguity_solver_perf.py` can be used to study the performance of the ambiguity solver (with and without ML) and also takes CSV files as input.

## MLSeedFiltering

While the ambiguity solver can significantly improve the cleanliness of the output removing both duplicates and fakes from the final collection, it doesn't help with the speed of the full tracking chain. The MLSeedFiltering aims to use a NN to determine which seed will lead to the best trajectory before performing the track reconstruction. We can select the seed used in track reconstruction based on the NN score. Depending on the full tracking setup, this might help us improve the track reconstruction speed, the track reconstructed quality, or both. In the case of the ODD, a speed improvement of x10 was observed in track reconstruction speed for ttbar, mu=200 G4+Pythia8 events.

It uses the same three steps as the ML ambiguity solver but with seed instead of tracks:

- Clustering: seeds are clustered together, one cluster ~ one truth particle
- Ranking: seeds in each cluster are scored, and the best one is kept
- Cleaning: last pass over all the remaining scores to remove fake

### Clustering

The clustering is implemented with the `dbscanSeedClustering` function. Its input is a vector of vectors containing the seed parameters that will be used in the clustering. For the clustering itself, we used a 4D DBSCAN clustering algorithm, a density-based clustering technique. All the seeds are represented as points in a 4D space using their value of $\phi$, $\eta$, $z_{0}$, $p_{T}$ and are clustered together based on their proximity. The output of the clustering is a vector of vectors of ints; each element corresponds to one cluster with the inner vector listing the ID of all the seeds in that cluster.

### Ranking

At this step, we have multiple clusters of seeds. We use a NN to compute a score for each seed. Due to the loss function used in training, the fake seed (coming from more than one truth particle) tends to have a score close to 0; we can thus remove them by cutting the low-score seed. The `minSeedScore` parameter is used to choose the cutoff value. For each cluster, we can then select the seed with the highest score to be kept.

### How to use

The ML-based Ambiguity solver comes with a pre-trained model to be used with the ODD. Using it is extremely simple; just call the `full_chain_odd.py` with the `--MLSeedFilter` option. If you want to try this solver with another detector, a few more steps are needed. First, you need to train a model. As an input, you will need the seeding and multitrajectory output from the CKF (run a full chain up to the CKF, then use the `CSVWriter` for both the seeding and the CKF). At least 1000 ttbar events with 200 PU are needed. First, we need to match the seed and the corresponding tracks together; for that, we can run `Examples/Scripts/Python/MLAmbiguityResolution/match_good_track-seed.py`. The network can then be trained with: `Examples/Scripts/Python/MLAmbiguityResolution/train_seed_solver.py ` on our dataset, 2/3 of the data should be used for the training and 1/3 for the validation. You will receive a `seedduplicateClassifier.onnx` file as an output, which can now be used as an input for the `AmbiguityResolutionMLAlgorithm` with your detector.

Additionally, another Python files is present with the training: `seed_filter_full_chain.py` performs the same job as the chain we presented here, but in Python, using CSV files in the input, this will plot many helpful distributions that will help you understand how the Filter is performing.

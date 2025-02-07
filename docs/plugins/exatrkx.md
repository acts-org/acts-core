<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

(exatrkxplugin)=
# Exa.TrkX plugin

This plugin contains a track finding module based on Graph Neural Networks (GNNs) which is developed by the [Exa.TrkX](https://exatrkx.github.io/) team. Build instructions and dependencies can be found in the [README](https://github.com/acts-project/acts/blob/main/Plugins/ExaTrkX/README.md) of the plugin.

## Stages

The Exa.TrkX pipeline is a multi-stage GNN-based algorithm. In principle, there are three types of stages:

1) **Graph construction**: This stage constructs the initial graph from the space points. Currently, there is only a metric-learning based approach implemented: A neural network tries to learn a mapping that minimizes the distance between points of the same track in the embedded space. In this embedded space then a graph is built using a fixed nearest-neighbor search.
2) **Edge classification**: In this stage, a graph is taken from the previous stage, and an edge-classification is performed on the edges. This can be done either by a simple feed forward network or by a GNN.
3) **Track building stage**: In this stage, track candidates are built from the edges and the scores of the previous edge classification stage. Currently, there are simple track building algorithms built on top of a *weakly connected components* algorithm available.

A typical pipeline consists e.g. of 4 stages:

```
Graph construction: Metric learning
                |
                v
   Edge classification: Filter
                |
                V
   Edge classification: GNN
                |
                V
 Track building with boost::graph
```

## Implementation

:::{note}
The codebase is currently under refactoring, the documentation will be updated once the code has stabilized.
:::

See [here](https://github.com/acts-project/acts/blob/main/Examples/Scripts/Python/exatrkx.py) for the corresponding python example.

## Resources

* Talk by *Daniel Murnane* at the [Connecting the Dots 2020](https://indico.cern.ch/event/831165/contributions/3717124/attachments/2024241/3385587/GNNs_for_Track_Finding.pdf)
* Talk by *Daniel Murnane* at the [vCHEP 2021](https://indico.cern.ch/event/948465/contributions/4323753/attachments/2246789/3810686/Physics%20and%20Computing%20Performance%20of%20the%20ExaTrkX%20TrackML%20Pipeline.pdf)
* Talk by *Alina Lazar* at the [ACAT 2021](https://indico.cern.ch/event/855454/contributions/4605079/attachments/2357191/4022841/ExaTrkX%20Inference%20-%20ACAT21%20v7.pdf)
* Talk by *Benjamin Huth* at the [ICHEP 2022](https://agenda.infn.it/event/28874/contributions/169199/attachments/94163/128944/slides_benjamin_huth_exatkrkx_acts.pdf)

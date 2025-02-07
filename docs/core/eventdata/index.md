<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Event data

:::{attention}
This section is **incomplete!**
:::

:::{toctree}
:maxdepth: 1
parametrization
tracks
measurements
particle_hypothesis
:::

The various tracking components in ACTS can be assembled into a full
reconstruction chain.  Between these components, data needs to be exchanged in
a well-defined way. This is achieved through the {term}`EDM` (Event Data Model), which
is a set of data types and interfaces representing the content of an event.
Until very recently, ACTS has focused mainly on an *internal* {term}`EDM`, which
is really focused on efficient interchange between components inside the
toolkit, sometimes at the cost of usability for clients. With the main
reconstruction chain becomes more and more mature, however, the focus has
shifted to a more client-oriented {term}`EDM` encapsulating the outputs of tracking.

(edm_chain)=
:::{figure} figures/edm_chain.svg
:width: 100%
Diagram showing the stages and data flow of a track reconstruction chain. The
boxes show {term}`EDM` objects that are passed between the stages.
:::


{numref}`edm_chain` shows an overview of the {term}`EDM` data types and how
they form a data-flow sequence. Measurements coming from the experiment
software are the main inputs of the chain. This data-type is abstracted in ACTS
in a way that allows the details of these measurements to be fully
experiment-specific. See [](#edm_uncalib_meas) for details.

ACTS ships with a clusterization algorithm, which can turn segmented raw
measurements into clusters, the second {term}`EDM` object in the chain,
representing particle intersections with the sensors. For the creation of track
seeds, clusters need to be converted into three-dimensional space-points,
combining information from multiple clusters where needed, e.g. for
one-dimensional silicon strip sensors.  The space-points and seeds are part of
the {term}`EDM` as well, since they are handed over to the track finding
component. This component is responsible for the creation of completed tracks,
which can then optionally be refitted with a precision track fitter.

Both the track finding and the precision track fit produce track objects, which
are the primary output of the tracking chain. More on the track EDM can be
found in [](#edm_tracks).

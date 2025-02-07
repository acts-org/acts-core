<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Glossary

:::{glossary}
EDM
  Event Data Model is a set of classes that can be used to describe the
  contents of an event.

Hole
  A hole is a state on a track where no measurement was found.

Outlier
  A measurement associated with a track, where the measurement is considered
  *far away* from the nominal parameter vector of the track on the associated
  measurement surface. This is often defined as measurement with a $\chi^2$
  larger than a certain threshold value.

SoA
  *Struct of Arrays* is a memory layout in which individual properties of an
  object are stored as contiguous arrays in a structure, where the same index
  is associated with logically connected entries. The opposite is {term}`AoS`.

AoS
  *Array of Structs* is a memory layout where a collection of objects are
  stored in a single array, where each object internally has a member for each
  property.

Seed
  A starting point for the track finding stage. E.g. a triplet of {term}`space
  points<Space point>` that are loosely compatible with a track hypothesis.

Space point
  A three dimensional (possibly approximated) location through which a particle
  will have passed and created masurements. In some cases, like strip
  detectors, space points are [formed from multiple
  measurements](#tracking_sp_formation).
:::

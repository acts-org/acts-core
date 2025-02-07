<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Tracking geometry building

## TrackingGeometry building using a KDTree and a Proto Description

For cylindrical detectors there exist a generic tracking geometry building module,
based on KDTree and a proto description.

This building procedure uses a {struct}`Acts::ProtoDetector` description which provides a
high level description of layers and container volumes, together with some
binning and ordering information.
This proto description is then used to assign surfaces that are provided to the
{class}`Acts::KDTreeTrackingGeometryBuilder` using an internal query to the KD-tree structure.

## Blueprint tree mechanism to build a tracking geometry

:::{todo}
Add description of Blueprint tree and how it can be used to create layerless detector.
:::

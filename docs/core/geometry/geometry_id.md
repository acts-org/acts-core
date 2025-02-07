<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Geometry identifier

The {class}`Acts::GeometryIdentifier` is mainly used for fast identification of the type of
the geometry object (as most of them are either extensions or containers of the
{class}`Acts::Surface` objects) and for the identification of the geometry surfaces after
building, e.g. for the uploading/assigning of material to the surface after
creation. The {class}`Acts::GeometryIdentifier` uses a simple masking procedure for applying an
identification schema.

While it is used in ACTS-internal applications such as material mapping, it is not employed for
`EventData` and `Geometry` identification in an experiment setup. Instead, one should define and use the
`Identifier` class in the latter case.

:::{doxygenclass} Acts::GeometryIdentifier
---
members: kVolumeMask,kBoundaryMask,kLayerMask,kApproachMask,kSensitiveMask,kExtraMask
---
:::

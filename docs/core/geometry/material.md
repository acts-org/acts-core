<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Detector material description

Two types of material description exist, one for a surface based material, one
for a volume based material. They will be dealt with differently in the
extrapolation.

The basic information for any material is:

* the radiation length X0 the nuclear interaction length L0 the atomic weight A
* the atomic charge Z the density of the material

This information is confined together in the {class}`Acts::Material` class.

```{note}
In track reconstruction, only an effective material description is needed, i.e.
non-physical values in regards of the atomic number, the elementary charge or
even the density are allowed, as long as the effective relative radiation
length and $A/Z \times \rho$ ratio can be retrieved.  This enables the compactification
of the material description, as the element composition record does not have to
be kept.
```

Surface based material extends this material information by a representative
thickness; the corresponding object is called {class}`Acts::MaterialSlab`. The
thickness hereby can be arbitrarily chosen in order to regulate the material
budget, it does not have to represent the actual thickness of a detector
element. To attach it to a surface, a dedicated {class}`Acts::ISurfaceMaterial`
class (or it's extensions) is used, which allows to also describe binned
material.

Possible extensions are:

 * {class}`Acts::HomogeneousSurfaceMaterial`, homogeneous material description on a surface
 * {class}`Acts::BinnedSurfaceMaterial`, an arbitrarily binned material description with a
    corresponding {class}`Acts::BinUtility` object

In addition, a dedicated extension exists to allow configuration of the material
mapping process, that is in further described below.

 * {class}`Acts::ProtoSurfaceMaterialT`, only binning description (without material) to be
   used in the material mapping process, which can be specified with a templated binning
   description.

<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

(layerless_geometry)=

# Layerless geometry

## Geometry module rosetta stone

:::{note}
The combination of the original (Gen 1) geometry classes and the new *layerless* modelling (Gen 2, this page) will result in a combined Gen 3 geometry model.
:::

:::{toctree}
building
:::

:::{note}
This module is not considered production-ready yet, and is under the namespace
`Acts::Experimental`.
:::

## Overview

### Geometry objects

The entire geometry setup is based on the following geometry objects

- {class}`Acts::Surface` describing any bound surface object
- {class}`Acts::Experimental::Portal` which holds a surface and adds information about attached volumes
- {class}`Acts::Experimental::DetectorVolume` which is bound by portals and can contain surfaces and other volumes
- {class}`Acts::Experimental::Detector` which is the top level object holding all detector relevant objects

### Memory management, access and const correctness

All geometric objects can only be constructed as `std::shared_ptr<T>` via dedicated `Object::makeShared(...)` factories.
Internally, all constituents are stored as non-const objects and can be accessed as such as long as the geometry is not locked, and the holder object is also a non-const object.

While objects may be stored as `std::shared_ptr<T>` internally, their access during navigation is given either to const references (if guaranteed to be existent) or const raw pointers (if optional or part of a polymorphic container).

### Navigation state and delegates

A struct {struct}`Acts::Experimental::NavigationState` holds the current navigation information through the geometry, which comprises

- the current {class}`Acts::Experimental::DetectorVolume` in associated with the position within the detector, called `currentVolume`
- a list of portal candidates to leave the `currentVolume`
- a list of surface candidates to be tested within the `currentVolume`
- a current position, direction, momentum, charge and magnetic field

Several navigation delegates built upon the {class}`Acts::Delegate` template class are defined and can be adapted and specialized for dedicated detector layouts.
These delegates are called:

- `Acts::Experimental::IInternalNavigation` that is called for updating the information at initialization, within the volume or at a volume switch caused by traversing a portal
- `Acts::Experimental::IExternalNavigation` which allows to find a volume by global position (and is usually only needed at initialization of the navigation) and which is attached to a {class}`Acts::Experimental::Portal` to switch volumes when traversing a portal

## Detailed Description

### The Portal object

Portals are composite objects of a {class}`Acts::Surface` object and additional volume link information which connect a portal to the at least one, but in general multiple volumes it connects. The position and the normal vector of an intersection with a portal are used to determine the next volume and hence the navigation flow through the detector.

When volumes are attached or glued to another, the portal object is usually shared between the volumes, and eventually portals can even be of larger extent than the actual volume they are bounding.

:::{figure} ../figures/DirectPortal.png
:width: 600px
:align: center
Illustration of a shared direct portal between two volumes, the arrows indicate the direction of attachment.
:::

:::{figure} ../figures/SharedPortal.png
:width: 600px
:align: center
Illustration of a shared extended portal between several volumes, the arrows indicate the direction of attachment.
:::

The implementation of a unique, binned or any other volume link can be adapted to the detector geometry by providing a suitable `Acts::Experimental::ExternalNavigationDelegate` delegate.

### The Detector volume object

A detector volume has to contain:

- a list of bounding portal objects (that can be shared with other volumes)
- a navigation state updator as a `Acts::Experimental::InternalNavigationDelegate` delegate, that at minimum is able to provide the portal surfaces for leaving the volume again.
- a unique name string

:::{note}
When constructing a detector volume one has to use a dedicated {class}`Acts::Experimental::DetectorVolumeFactory` that is provided with a portal generator which allows to generate the portals and set the (yet) existing links appropriately.
:::

Additionally, it can contain:

- an optional collection of contained surfaces which can describe sensitive surfaces or passive surfaces (e.g. for material integration)
- an optional collection of contained volumes which describe sub volumes, as e.g. chambers or cells
- a volume material description

In case the volume contains surfaces and/or volumes, an adequate navigation state updator is to be provided that can resolve surface candidates or portal candidates into the sub volumes. E.g.~if the volume contain a layer with sensitive surfaces, a grid can be used to associate an entry/global position with the candidate surfaces further tested in the navigation.

:::{figure} ../figures/EndcapGrid.png
:width: 600px
:align: center
Illustration of a planar module endcap detector with a grid holding the indices to the candidate surfaces.
:::

:::{note}
When building in `Debug` mode the containment of objects inside a `Acts::DetectorVolume` is checked with an `assert(...)` statement.
:::

### The Detector object

The detector object is the holder class of all geometry objects, it has to contain:

- at least one detector volume
- a name string
- a volume finder delegate (as `Acts::Experimental::DetectorVolumeFinder`) that allows to uniquely associate a point in space with a contained volume of the detector.

:::{note}
When the detector is constructed, name duplicates are checked for and if found a `std::exception` is thrown. Similarly, when sensitive surfaces are provided and duplicate `Acts::GeometryIdentifier` objects are found during detector construction a `std::exception` is thrown. The latter can be avoided by using an appropriate (set of) `Acts::GeometryIdGenerator` tool(s) which will guarantee some level of uniqueness.
:::

:::{figure} ../figures/ODD_Detector.png
:width: 600px
:align: center
Illustration (r/z view) of the OpenDataDetector with its sub volumes, portals and contained surfaces.
:::

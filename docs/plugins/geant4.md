<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Geant4 plugin

The Geant4 plugin allows to build {class}`Acts::TrackingGeometry` and {class}`Acts::Experimental::Detector` directly from Geant4 geometry input.
Both rely on the conversion of `G4VPhysicalVolume` into corresponding `Acts` objects.

## Object conversion

### Surface conversion

Converting physical volumes into {class}`Acts::Surface` objects that represent sensitive detector elements, is done via the {class}`Acts::Geant4DetectorSurfaceFactory`.
This helper class allows to select volumes from the Geant4 geometry and convert them either into pairs of {class}`Acts::Geant4DetectorElement` and {class}`Acts::Surface` objects in case of sensitive elements, or simply surfaces objects in the case of passive surfaces.

The selection is hereby done by providing one or more {class}`Acts::IGeant4PhysicalVolumeSelector` objects to the surface factory.

Possible implementations of this type of conversions can be seen in the corresponding unit test `ActsUnitTestGeant4DetectorSurfaceFactory`

```cpp
// Get the box
auto nameSelector =
    std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
        std::vector<std::string>{"yl"}, false);

Acts::Geant4DetectorSurfaceFactory::Cache cache;
Acts::Geant4DetectorSurfaceFactory::Options options;
options.sensitiveSurfaceSelector = nameSelector;

Acts::Geant4DetectorSurfaceFactory factory;
factory.construct(cache, nominal, *cylinderPV, options);

BOOST_CHECK_EQUAL(cache.sensitiveSurfaces.size(), 1u);
BOOST_CHECK_EQUAL(cache.passiveSurfaces.size(), 0u);

auto [ element, surface ] = cache.sensitiveSurfaces.front();
BOOST_CHECK_EQUAL(surface->type(), Acts::Surface::SurfaceType::Cylinder);
```

#### Inspecting surface conversion within python

The `ActsExamples` python bindings allow to conveniently test the conversion of `Geant4` volumes into sensitive and passive surfaces, assuming you have a GDML file called `detector.gdml` where `Geant4PhysVolume` objects can be identified by a certain string, e.g. names containing the flag `Sensitive`, or `Passive`. Also, multiple match strings are allowed. The converted surfaces can then be displayed with `.obj` (part of the Core functionality) or as `.svg` files (if `ACTS_BUILD_PLUGIN_ACTSVG` is switched on)

```python
# import the necessary modules
import acts, acts.examples
from acts.examples import geant4 as acts_g4

# The match criteria
sensitive_matches = [ 'Sensitive' ]
passive_matches = [ 'Passive' ]
[ elements, ssurfaces, psurfaces ] = acts_g4.convertSurfaces('detector.gdml', sensitive_matches, passive_matches)

# Some screen output
print('* Conversion yielded', len(ssurfaces))

# Write them to an obj file
drawContext = acts.GeometryContext()
sensitiveRgb = [ 0, 150, 150 ]
passiveRgb = [ 150, 150, 0]
segments = 64 # how many segments to approximate a full circle
# Draw the sensitive surfaces
acts.examples.writeSurfacesObj(ssurfaces, drawContext, sensitiveRgb, segments, 'detector-sensitives.obj')
# Draw the passive surfaces
acts.examples.writeSurfacesObj(psurfaces, drawContext, passiveRgb, segments, 'detector-passives.obj')
```

## Building a Detector from Geant4 input

In order to build an `Acts::Detector` object from `Geant4` input, the following steps needs to be done
 * a conversion of `Geant4PhysVolume` objects into `Acts::Surface`  and `Acts::DetectorVolume` objects (see before)
 * a build sequence needs to be defined and the converted objects identified

There are several helper methods and tools that can be used, many of them accessible through python bindings. One core component is the selection and assignment of surfaces to dedicated volume. This can be done using e.g. a KDT structure, this can be tested with:

```python
# Create a KDTree from all surfaces binned in z and r
surfacesKdt = acts.KdtSurfaces2D(buildContext, surfaces, [acts.Binning.z, acts.Binning.r])

# Define a query range and select
qrange = acts.Range2D( [-580,580], [0,200])
selected = surfacesKdt.surfaces(qrange)

# Draw, inspect the surfaces
```

Selected surfaces can be put as a layer structure into a volume

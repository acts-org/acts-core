<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

(magnetic-field-core)=
# Magnetic field

The magnetic field component of ACTS provides functionality to describe
arbitrary magnetic field configurations in an experiment. It is implemented in
a generic way and can be extended to connect to an experiment specific upstream
source of field data.

Algorithms which need magnetic field information (e.g.
{class}`Acts::AtlasStepper`, {class}`Acts::EigenStepper`) accept the magnetic
field as an explicit argument.

## Provider interface

All magnetic field implementations inherit and implement the interface {class}`Acts::MagneticFieldProvider`:


It provides a generic interface over different implementations. To speed up
magnetic field lookup, each implementation can have a *cache* object. The cache
object can for example be used to store a local interpolation domain, to speed
up nearby field lookups. The client is expected to pass into lookup calls:

:::{doxygentypedef} Acts::MagneticFieldProvider::Cache
:::

The implementation is then free to use and update this cache instance as
needed. Before a client can issue field lookup calls, it needs to obtain an
initialized instance of this cache object. This can be achieved generically for
all implementations by using {func}`Acts::MagneticFieldProvider::makeCache`.
This function accepts an instance of {class}`Acts::MagneticFieldContext`, see
[](#magnetic-field-context) for details.

The main lookup method of {class}`Acts::MagneticFieldProvider` is

:::{doxygenfunction} Acts::MagneticFieldProvider::getField
:outline:
:::

Aside from the lookup position as a global position vector, it accepts an
instance of the opaque cache object mentioned before. The return value is a
{class}`Acts::Result` object. It either contains the field value at the
requested location, or an {enum}`Acts::MagneticFieldError` in case of a lookup
failure, like an out-of-bounds lookup position.

Below is an example of how a client can interact with an instance of
{class}`Acts::MagneticFieldProvider`.

```cpp
// in event context
auto fieldContext = getExperimentFieldContext();
const Acts::MagneticFieldProvider& fieldProvider = getFieldProvider();
auto cache = fieldProvider.makeCache(fieldContext);

auto lookupResult = fieldProvider.getField(Acts::Vector3{10, 10, 10}, cache);
if(!lookupResult.ok()) {
   throw std::runtime_error{"Field lookup failure"};
}

Acts::Vector3 fieldValue = *lookupResult;
```

## Magnetic field context

:::{doxygenclass} Acts::MagneticFieldContext
:outline:
:::

The magnetic field context is an opaque type which contains experiment specific
event context information. This can be used to supply event dependent data to
the magnetic field instance, in case it is needed to provide correct field
values. The library itself does not make any assumptions on the content of this
context type (it is implemented using `std::any`), but passes a reference
through the call-chain to the field implementation. An experiment specific
field implementation is then expected to performa cast to the concrete type,
and use the contents.

An example use case of the context could be to look up conditions data /
records for the value of the magnetic field at the time of the event.

## Field provider implementations in Core

There are a number of field provider implementations found in core which serve different purposes.

### Constant magnetic field

The simplest implementation is a constant field, which returns the same field
values at every queried location. It is implemented in the
{class}`Acts::ConstantBField` class.

:::{doxygenclass} Acts::ConstantBField
:members: ConstantBField
:::

As seen above, the class is constructed from a three-dimensional field vector,
which is returned unmodified to every call to
{func}`Acts::ConstantBField::getField`.

### Interpolated magnetic field

For more complex magnetic field implementations
{class}`Acts::InterpolatedMagneticField` can be used. The idea here is to calculate
an interpolated value of the magnetic field from a grid of known field values.
In 3D, this means the interpolation is done from the 8 corner points of a *field
cell*. The field cell can be retrieved for any given position. Since during
typical access patterns, e.g. the propagation, subsequent steps are relatively
likely to not cross the field cell boundary, the field cell can be cached.

:::{figure} figures/bfield/field_cell.svg
:width: 300
:align: center
Illustration of the field cell concept. Subsequent steps are clustered in the
same field cell. The field cell only needs to be refetched when the propagation
crosses into the next grid region.
:::

{class}`Acts::InterpolatedMagneticField` extends the
{class}`Acts::MagneticFieldProvider` interface to add a number of additional
methods:

:::{doxygenclass} Acts::InterpolatedMagneticField
:::

This intermediate interface is again implemented by
{class}`Acts::InterpolatedBFieldMap`, which is a template class that depends on
an instance of {class}`Acts::Grid`. Varying configurations are possible,
like a 2D field map that exploits $rz$ symmetry, or a plain 3D grid.

:::{doxygenclass} Acts::InterpolatedBFieldMap
:members: false
:::


The class constructor accepts a single configuration struct that
contains the grid instance, a scale factor and optional conversion function for
the lookup positions and the returned field values.

:::{doxygenstruct} Acts::InterpolatedBFieldMap::Config
:::

Internally, {class}`Acts::InterpolatedBFieldMap` uses a *field interpolation
cell* to speed up lookups. This cell contains the interpolation points so the
grid does not have to be consulted for each lookup. Explicit methods to create
such a field cell are provided, but field cell creation is automatically
handled by {func}`Acts::InterpolatedBFieldMap::makeCache`, opaque to the
client.

Helpers to construct an interpolated field map from text and ROOT file inputs
are provided:

:::{doxygenfunction} Acts::fieldMapRZ
:::

:::{doxygenfunction} Acts::fieldMapXYZ
:::


### Analytical solenoid magnetic field

:::{warning}
The analytical solenoid field is **slow**. See {func}`Acts::solenoidFieldMap`
to speed it up.
:::

ACTS also provides a field provider that calculates the field vectors
analytically for a [solenoid](https://en.wikipedia.org/wiki/Solenoid) field.

:::{figure} figures/bfield/quiver.png
:width: 600
:align: center
Picture of a solenoid field in rz, with arrows indicating the direction of the
field, and their size denoting the strength. The field is almost homogeneous in
the center.
:::

The implementation has configurable solenoid parameters:

:::{doxygenstruct} Acts::SolenoidBField::Config
:::

:::{note}
A configuration of
```cpp
SolenoidBField::Config cfg;
cfg.length = 5.8_m;
cfg.radius = (2.56 + 2.46) * 0.5 * 0.5_m;
cfg.nCoils = 1154;
cfg.bMagCenter = 2_T;
SolenoidBField bField(cfg);
```
roughly corresponds to the solenoid wrapping the Inner Detector in ATLAS.
:::

#### Implementation

The calculation uses two special functions:

- $E_1(k^2)$ is the complete elliptic integral of the 1st kind
- $E_2(k^2)$ is the complete elliptic integral of the 2nd kind

$E_1(k^2)$ and $E_2(k^2)$ are usually indicated as $K(k^2)$ and $E(k^2)$ in literature, respectively:

$$
E_1(k^2) = \int_0^{\pi/2} \left( 1 - k^2 \sin^2{\theta} \right )^{-1/2} \mathop{}\!\mathrm{d}\theta
$$

$$
E_2(k^2) = \int_0^{\pi/2}\sqrt{1 - k^2 \sin^2{\theta}} \mathop{}\!\mathrm{d}\theta
$$

$k^2$ is a function of the point $(r, z)$ and of the radius of the coil $R$

$$
k^2 = \frac{4Rr}{(R+r)^2 + z^2}
$$

Using these, you can evaluate the two components $B_r$ and $B_z$ of the magnetic field:

$$
B_r(r, z) = \frac{\mu_0 I}{4\pi} \frac{kz}{\sqrt{Rr^3}} \left[ \left(\frac{2-k^2}{2-2k^2}\right)E_2(k^2) - E_1(k^2) \right ]
$$

$$
B_z(r,z) = \frac{\mu_0 I}{4\pi} \frac{k}{\sqrt{Rr}} \left[ \left( \frac{(R+r)k^2-2r}{2r(1-k^2)} \right ) E_2(k^2) + E_1(k^2) \right ]
$$

In the implementation the factor of $(\mu_0\cdot I)$ is defined to be a scaling
factor. It is evaluated and defined as the magnetic field in the center of the
coil, i.e. the scale set in {member}`Acts::SolenoidBField::Config::bMagCenter`.

As the evaluation of $E_1(k^2)$ and $E_2(k^2)$ is **slow**. The
{class}`Acts::InterpolatedBFieldMap` easily outperforms
{class}`Acts::SolenoidBField`. A helper is provided that builds a map from the
analytical implementation and is much faster to lookup:

:::{doxygenfunction} Acts::solenoidFieldMap
:::

### Multi-range constant field

The multi-range constant field allows modelling cases where a magnetic field
can be described as multiple (potentially overlapping) regions, each of which
has its own constant magnetic field. This provides more flexibility than the
{class}`Acts::ConstantBField` while providing higher performance than
{class}`Acts::InterpolatedBFieldMap`.

This magnetic field provider is configured using a list of pairs, where each
pair defines a region in three-dimensional space as well as a field vector.
Magnetic field lookup then proceeds by finding the _last_ region in the
user-provided list that contains the requested coordinate and returning the
corresponding field vector.

The implementation uses a simple caching mechanism to store the last matched
region, providing improved performance for consecutive lookups within the same
region. This is thread-safe when each thread uses its own cache instance. The
field configuration itself is immutable after construction.

:::{doxygenclass} Acts::MultiRangeBField
:::

## Full provider interface

:::{doxygenclass} Acts::MagneticFieldProvider
:::

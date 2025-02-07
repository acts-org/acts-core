<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Grid and axis

The `Grid` template class provides a generic binned `Grid` implementation in $N$ dimensions. `Grid` accepts a variadic list of `Axis` types, where the number of axes equals the number of desired dimensions of the `Grid`.

## Axis
`Axis` accepts two template parameters:

```cpp
template <AxisType type,
          AxisBoundaryType bdt = AxisBoundaryType::Open>
class Axis;
```

where
```cpp
enum class AxisBoundaryType { Open, Bound, Closed };
enum class AxisType { Equidistant, Variable };
```

### AxisType

`AxisType` defines whether an axis is fully determined by $x_\text{min}$, $x_\text{max}$ and $N_\text{bins}$, or if there are variable bin boundaries $x_i = \{x_1, \ldots x_N\}$.
The axis boundaries and, if necessary, the bin boundaries are provided in the constructor.


If at all possible, `Equidistant` is preferable, since bin $b$ can then be calculated in constant time for given $x$ as

$$b = \frac{\mathrm{floor}(x - x_\text{min})}{w} + 1$$

where $b \in \{0, 1, \ldots N_\text{bins}\}$. If the type is `Variable`, a search for the correct bin is performed over the bin boundaries.

### AxisBoundaryType

`AxisBoundaryType` steers how out-of-bounds lookups are handled.
There are three options:

![The three different axis boundary types](figures/AxisBoundaryTypes.svg)

- **Bound**: out-of-bounds lookups resolve to the closest valid bin.
- **Open**: out-of-bounds lookups resolve to dedicated underflow and overflow bins.
- **Closed**: out-of-bounds lookups wrap-around to the other side of the axis.

## Grid creation

The types of the axes have to be known at compile-time, since they are provided to the `Grid` as template parameters.
Thus, the number of dimensions $N$ of the `Grid` is also fixed at compile-time.
The axes can be any combination of the aforementioned variations.

```cpp
template <typename T, class... Axes> class Grid;
```

where `T` is the value type that is stored in each bin. Instances of each axis are then given to the `Grid` constructor as a tuple. This Axis instances hold the number of bins and the boundary values, which are **runtime values**.

The `Grid` performs a lookup by recursively visiting each axis and having it perform a lookup in the corresponding component of the input vector. The lookup methods are templated on the input vector type, which only has to support component access. Transforming in and out of the local `Grid` reference frame is not handled by the `Grid`.

## Local vs global bin indices

The underlying data structure of the `Grid` is a one-dimensional `std::vector<T>`, where `T` is the template parameter determining the stored value type. The index of this vector is referred to as the **global bin index**.
The logical index structure, where each global bin is addressed by an $N$-dimensional vector of integers is called **local bin indices**. The two can be converted into one another using

```cpp
using index_t = std::array<size_t, N>;
index_t getLocalBinIndices(size_t bin) const;
size_t getGlobalBinIndex(const index_t& localBins) const;
```

The local bin indices are always defined from 1 to $N_\text{bins}$ for the respective axis. Bins 0 and $N_\text{bins} + 1$ address the underflow and overflow bins, **which are always present, even if `AxisBoundaryType != Open`**.

## Finding neighbors

The `Grid` can determine the number of neighbors around a given bin. This done by first converting the global bin index to local bin indices, and then varying the local bin indices in each dimension. At this stage, each Axis gets to decide, which indices are considered neighbors, which differs depending on `AxisBinningType`. `Open` considers the underflow and overflow bins, while `Bound` does not. `Closed` considers bins on the other side of the axis.
The resulting neighbor combination around bin *B* might look like

| b x b | 1     | 2    | 3     |
|-------|-------|------|-------|
| 1     | x     | 0,+1 | x     |
| 2     | -1, 0 | B    | +1, 0 |
| 3     | x     | 0,-1 | x     |

Here, the corner combinations are still missing (also true for $N>2$). This is then turned into a hypercube which in $N=2$ results in:

| b x b | 1     | 2    | 3     |
|-------|-------|------|-------|
| 1     | -1,+1 | 0,+1 | +1,+1 |
| 2     | -1, 0 | B    | +1, 0 |
| 3     | -1,-1 | 0,-1 | +1,-1 |

These local bin indices are then converted into global bin indices before being returned.

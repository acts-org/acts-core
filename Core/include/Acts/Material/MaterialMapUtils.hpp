// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/InterpolatedMaterialMap.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

// Convenience functions to ease creation of and Acts::InterpolatedMaterialMap
// and to avoid code duplication. Currently implemented for the two most common
// formats: rz and xyz.

namespace Acts {

class Material;

/// Method to setup the MaterialMapper
/// @param [in] materialVectorToGridMapper Function mapping the vector of
/// material to the map of material values
///
/// e.g.: we have small grid with the values: r={2,3}, z ={4,5}, the
/// corresponding indices are i (belonging to r) and j (belonging to z), the
/// globalIndex is M (belonging to the values of the Material) and the map is:
///|   r |    i |    z |    j |   M |
///|----:|:----:|:----:|:----:|:----|
///|   2 |    0 |    4 |    0 |   0 |
///|   2 |    0 |    5 |    1 |   1 |
///|   3 |    1 |    4 |    0 |   2 |
///|   3 |    1 |    5 |    1 |   3 |
///
/// In this case the function would look like:
/// @code
/// [](std::array<std::size_t, 2> binsRZ, std::array<std::size_t, 2> nBinsRZ) {
///    return (binsRZ.at(0) * nBinsRZ.at(1) + binsRZ.at(1));
/// }
/// @endcode
/// @param [in] rPos Values of the grid points in r
/// @note The values do not need to be sorted or unique (this will be done
/// inside the function)
/// @param [in] zPos Values of the grid points in z
/// @note The values do not need to be sorted or unique (this will be done
/// inside the function)
/// @param [in] material The material classification values in r and z for all
/// given grid points stored in a vector
/// @note The function localToGlobalBin determines how the material was
/// stored in the vector in respect to the grid values
/// @param [in] lengthUnit The unit of the grid points
MaterialMapper<
    Grid<Material::ParametersVector, Axis<Acts::AxisType::Equidistant>,
         Axis<Acts::AxisType::Equidistant>>>
materialMapperRZ(
    const std::function<std::size_t(std::array<std::size_t, 2> binsRZ,
                                    std::array<std::size_t, 2> nBinsRZ)>&
        materialVectorToGridMapper,
    std::vector<double> rPos, std::vector<double> zPos,
    const std::vector<Acts::Material>& material,
    double lengthUnit = UnitConstants::mm);

/// Method to setup the MaterialMapper
/// @param [in] materialVectorToGridMapper Function mapping the vector of
/// material to the map of material values
///
/// e.g.: we have small grid with the values: x={2,3}, y={3,4}, z ={4,5}, the
/// corresponding indices are i (belonging to x), j (belonging to y) and k
/// (belonging to z), the globalIndex is M (belonging to the values of the
/// Material) and the map is:
///| x   |    i |    y |    j |    z |    k |   M |
///|----:|:----:|:----:|:----:|:----:|:----:|:----|
///|   2 |    0 |    3 |    0 |    4 |    0 |   0 |
///|   2 |    0 |    3 |    0 |    5 |    1 |   1 |
///|   2 |    0 |    4 |    1 |    4 |    0 |   2 |
///|   2 |    0 |    4 |    1 |    5 |    1 |   3 |
///|   3 |    1 |    3 |    0 |    4 |    0 |   4 |
///|   3 |    1 |    3 |    0 |    5 |    1 |   5 |
///|   3 |    1 |    4 |    1 |    4 |    0 |   6 |
///|   3 |    1 |    4 |    1 |    5 |    1 |   7 |
///
/// In this case the function would look like:
/// @code
/// [](std::array<std::size_t, 3> binsXYZ, std::array<std::size_t, 3> nBinsXYZ)
/// {
///   return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2))
///        + binsXYZ.at(1) * nBinsXYZ.at(2)
///        + binsXYZ.at(2));
/// }
/// @endcode
/// @param[in] xPos Values of the grid points in x
/// @note The values do not need to be sorted or unique (this will be done
/// inside the function)
/// @param[in] yPos Values of the grid points in y
/// @note The values do not need to be sorted or unique (this will be done
/// inside the function)
/// @param[in] zPos Values of the grid points in z
/// @note The values do not need to be sorted or unique (this will be done
/// inside the function)
/// @param [in] material The material classification values in x, y and z for
/// all given grid points stored in a vector
/// @note The function localToGlobalBin determines how the material was
/// stored in the vector in respect to the grid values
/// @param [in] lengthUnit The unit of the grid points
MaterialMapper<
    Grid<Material::ParametersVector, Axis<Acts::AxisType::Equidistant>,
         Axis<Acts::AxisType::Equidistant>, Axis<Acts::AxisType::Equidistant>>>
materialMapperXYZ(
    const std::function<std::size_t(std::array<std::size_t, 3> binsXYZ,
                                    std::array<std::size_t, 3> nBinsXYZ)>&
        materialVectorToGridMapper,
    std::vector<double> xPos, std::vector<double> yPos,
    std::vector<double> zPos, const std::vector<Material>& material,
    double lengthUnit = UnitConstants::mm);

}  // namespace Acts

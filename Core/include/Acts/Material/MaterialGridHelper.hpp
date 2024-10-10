// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/AccumulatedVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {

class MaterialSlab;

using EAxis = Acts::Axis<AxisType::Equidistant>;
using Grid2D = Acts::Grid<Acts::AccumulatedVolumeMaterial, EAxis, EAxis>;
using Grid3D = Acts::Grid<Acts::AccumulatedVolumeMaterial, EAxis, EAxis, EAxis>;
using MaterialGrid2D =
    Acts::Grid<Acts::Material::ParametersVector, EAxis, EAxis>;
using MaterialGrid3D =
    Acts::Grid<Acts::Material::ParametersVector, EAxis, EAxis, EAxis>;

using MaterialGridAxisData = std::tuple<double, double, std::size_t>;

/// @brief Helper method that creates the cache grid for the mapping. This
/// grid allows the collection of material at a the anchor points.
///
/// @param [in] gridAxis1 Axis data
/// @param [in] gridAxis2 Axis data
/// @note The data of the axes is given in the std::array as {minimum value,
/// maximum value, number of bins}
///
/// @return The grid
Grid2D createGrid(MaterialGridAxisData gridAxis1,
                  MaterialGridAxisData gridAxis2);

/// @brief Helper method that creates the cache grid for the mapping. This
/// grid allows the collection of material at a the anchor points.
///
/// @param [in] gridAxis1 Axis data
/// @param [in] gridAxis2 Axis data
/// @param [in] gridAxis3 Axis data
/// @note The data of the axes is given in the std::array as {minimum value,
/// maximum value, number of bins}
///
/// @return The grid
Grid3D createGrid(MaterialGridAxisData gridAxis1,
                  MaterialGridAxisData gridAxis2,
                  MaterialGridAxisData gridAxis3);

/// @brief return a function that return the coordinate corresponding to type of
/// bin
///
/// @param [in] type Type of bin
///
/// @return a coordinate transform function
std::function<double(Acts::Vector3)> globalToLocalFromBin(
    Acts::BinningValue& type);

/// @brief Create a 2DGrid using a BinUtility.
/// Also determine the corresponding global to local transform and grid mapping
/// function
///
/// @param [in] bins BinUtility of the volume to be mapped
/// @param [in] transfoGlobalToLocal Global to local transform to be updated.
///
/// @return the 3D grid
Grid2D createGrid2D(
    const BinUtility& bins,
    std::function<Acts::Vector2(Acts::Vector3)>& transfoGlobalToLocal);

/// @brief Create a 3DGrid using a BinUtility.
/// Also determine the corresponding global to local transform and grid mapping
/// function
///
/// @param [in] bins BinUtility of the volume to be mapped
/// @param [in] transfoGlobalToLocal Global to local transform to be updated.
///
/// @return the 3D grid
Grid3D createGrid3D(
    const BinUtility& bins,
    std::function<Acts::Vector3(Acts::Vector3)>& transfoGlobalToLocal);

/// @brief Average the material collected in a 2D grid and use it to create a 2D material grid
///
/// @param [in] grid The material collecting grid
/// coordinate
///
/// @return The average material grid decomposed into classification numbers
MaterialGrid2D mapMaterialPoints(Grid2D& grid);

/// @brief Average the material collected in a 3D grid and use it to create a 3D material grid
///
/// @param [in] grid The material collecting grid
/// coordinate
///
/// @return The average material grid decomposed into classification numbers
MaterialGrid3D mapMaterialPoints(Grid3D& grid);

}  // namespace Acts

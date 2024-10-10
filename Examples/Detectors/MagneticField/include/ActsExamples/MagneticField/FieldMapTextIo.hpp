// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <string>

namespace ActsExamples {

/// Method to setup the FieldMapper
/// @param localToGlobalBin Function mapping the local bins of r,z to the
/// global
/// bin of the map magnetic field value e.g.: we have small grid with the
/// values: r={2,3}, z ={4,5}, the corresponding indices are i(r) and j(z),
/// the
/// globalIndex is M and the field map is:
///|| r | i || z | j || |B(r,z)| ||  M ||
///  -----------------------------------
///|| 2 | 0 || 4 | 0 ||  2.323   ||  0 ||
///|| 2 | 0 || 5 | 1 ||  2.334   ||  1 ||
///|| 3 | 1 || 4 | 0 ||  2.325   ||  2 ||
///|| 3 | 1 || 5 | 1 ||  2.331   ||  3 ||
///
/// @code
/// In this case the function would look like:
/// [](std::array<std::size_t, 2> binsRZ, std::array<std::size_t, 2> nBinsRZ) {
///    return (binsRZ.at(0) * nBinsRZ.at(1) + binsRZ.at(1));
/// }
/// @endcode
/// @param[in] fieldMapFile Path to file containing field map in txt format
/// @param[in] lengthUnit The unit of the grid points
/// @param[in] BFieldUnit The unit of the magnetic field
/// @note This information is only used as a hint for the required size of
///       the internal vectors. A correct value is not needed, but will help
///       to speed up the field map initialization process.
/// @param[in] firstQuadrant Flag if set to true indicating that only the
/// first
/// quadrant of the grid points and the BField values has been given and
/// that
/// the BFieldMap should be created symmetrically for all quadrants.
/// e.g. we have the grid values r={0,1} with BFieldValues={2,3} on the r
/// axis.
/// If the flag is set to true the r-axis grid values will be set to
/// {-1,0,1}
/// and the BFieldValues will be set to {3,2,3}.
detail::InterpolatedMagneticField2 makeMagneticFieldMapRzFromText(
    const std::function<std::size_t(std::array<std::size_t, 2> binsRZ,
                                    std::array<std::size_t, 2> nBinsRZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, Acts::ActsScalar lengthUnit,
    Acts::ActsScalar BFieldUnit, bool firstQuadrant = false);

/// Method to setup the FieldMapper
/// @param localToGlobalBin Function mapping the local bins of x,y,z to the
/// global bin of the map magnetic field value e.g.: we have small grid with
/// the
/// values: x={2,3}, y={3,4}, z ={4,5}, the corresponding indices are i(x),
/// j(y)
/// and z(k), the globalIndex is M and the field map is:
///|| x | i || y | j || z | k || |B(x,y,z)| ||  M ||
///  --------------------------------------------
///|| 2 | 0 || 3 | 0 || 4 | 0 ||  2.323   ||  0 ||
///|| 2 | 0 || 3 | 0 || 5 | 1 ||  2.334   ||  1 ||
///|| 2 | 0 || 4 | 1 || 4 | 0 ||  2.325   ||  2 ||
///|| 2 | 0 || 4 | 1 || 5 | 1 ||  2.331   ||  3 ||
///|| 3 | 1 || 3 | 0 || 4 | 0 ||  2.323   ||  4 ||
///|| 3 | 1 || 3 | 0 || 5 | 1 ||  2.334   ||  5 ||
///|| 3 | 1 || 4 | 1 || 4 | 0 ||  2.325   ||  6 ||
///|| 3 | 1 || 4 | 1 || 5 | 1 ||  2.331   ||  7 ||
///
/// @code
/// In this case the function would look like:
/// [](std::array<std::size_t, 3> binsXYZ, std::array<std::size_t, 3> nBinsXYZ)
/// {
///   return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2))
///        + binsXYZ.at(1) * nBinsXYZ.at(2)
///        + binsXYZ.at(2));
/// }
/// @endcode
/// @param[in] fieldMapFile Path to file containing field map in txt format
/// @param[in] lengthUnit The unit of the grid points
/// @param[in] BFieldUnit The unit of the magnetic field
/// @note This information is only used as a hint for the required size of
///       the internal vectors. A correct value is not needed, but will help
///       to speed up the field map initialization process.
/// @param[in] firstOctant Flag if set to true indicating that only the
/// first
/// octant of the grid points and the BField values has been given and that
/// the BFieldMap should be created symmetrically for all quadrants.
/// e.g. we have the grid values z={0,1} with BFieldValues={2,3} on the r
/// axis.
/// If the flag is set to true the z-axis grid values will be set to
/// {-1,0,1}
/// and the BFieldValues will be set to {3,2,3}.
detail::InterpolatedMagneticField3 makeMagneticFieldMapXyzFromText(
    const std::function<std::size_t(std::array<std::size_t, 3> binsXYZ,
                                    std::array<std::size_t, 3> nBinsXYZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, Acts::ActsScalar lengthUnit,
    Acts::ActsScalar BFieldUnit, bool firstOctant = false);

}  // namespace ActsExamples

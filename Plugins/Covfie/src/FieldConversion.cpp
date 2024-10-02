// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Covfie/FieldConversion.hpp"

#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace Acts::CovfiePlugin {

/// @brief Creates a strided covfie field that stores the values of the
/// magnetic field in the volume given by min and max using a fixed sample
/// spacing (determined by nPoints).
///
/// @param magneticField The acts magnetic field.
/// @param cache The acts cache.
/// @param nPoints 3D array of containing the number of bins for each axis.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return A strided covfie field.
template <typename magnetic_field_t>
auto newBuilder(const magnetic_field_t& magneticField,
                typename magnetic_field_t::Cache& cache,
                const std::array<std::size_t, 3>& nPoints,
                const Acts::Vector3& min, const Acts::Vector3& max) {
  using Field = covfie::field<BuilderBackend>;

  // Hack to avoid the fact that the domain of ACTS magnetic fields is defined
  // as a half-open interval. Has the potential to introduce very minor
  // floating point errors, but no easy way to fix this right now.
  // TODO: Fix the aforementioned problem.
  std::vector<double> maxima = {
      std::nexttoward(max[0], -std::numeric_limits<double>::infinity()),
      std::nexttoward(max[1], -std::numeric_limits<double>::infinity()),
      std::nexttoward(max[1], -std::numeric_limits<double>::infinity()),
  };

  Field field(covfie::make_parameter_pack(
      Field::backend_t::configuration_t{nPoints[0], nPoints[1], nPoints[2]}));

  Field::view_t view(field);

  std::array<double, 3> sampleSpacing = {
      (max.x() - min.x()) / (nPoints[0] - 1),
      (max.y() - min.y()) / (nPoints[1] - 1),
      (max.z() - min.z()) / (nPoints[2] - 1)};

  for (std::size_t x = 0; x < nPoints[0]; x++) {
    for (std::size_t y = 0; y < nPoints[1]; y++) {
      for (std::size_t z = 0; z < nPoints[2]; z++) {
        Acts::Vector3 position{
            std::min(x * sampleSpacing[0] + min[0], maxima[0]),
            std::min(y * sampleSpacing[1] + min[1], maxima[1]),
            std::min(z * sampleSpacing[2] + min[2], maxima[2])};

        Field::view_t::output_t& p = view.at(x, y, z);
        Result<Vector3> result = magneticField.getField(position, cache);

        if (!result.ok()) {
          throw std::runtime_error("Field lookup failed!");
        }

        Acts::Vector3 rv = *result;
        p[0] = static_cast<float>(rv[0]);
        p[1] = static_cast<float>(rv[1]);
        p[2] = static_cast<float>(rv[2]);
      }
    }
  }

  return field;
}

/// @brief Generate the affine covfie configuration (scaling and rotation)
/// given the size of the field (min and max)
///
/// @param nPoints 3D array of containing the number of bins for each axis.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return The affine field configuration.
template <typename backend_t>
typename backend_t::configuration_t affineConfiguration(
    const std::array<std::size_t, 3>& nPoints, const Acts::Vector3& min,
    const Acts::Vector3& max) {
  auto scaling = covfie::algebra::affine<3>::scaling(
      static_cast<float>((nPoints[0] - 1) / (max[0] - min[0])),
      static_cast<float>((nPoints[1] - 1) / (max[1] - min[1])),
      static_cast<float>((nPoints[2] - 1) / (max[2] - min[2])));

  auto translation = covfie::algebra::affine<3>::translation(
      static_cast<float>(-min[0]), static_cast<float>(-min[1]),
      static_cast<float>(-min[2]));

  return {scaling * translation};
}

/// @brief Uses std::nextafter to generates a clamp backend
/// configuration where arguments min and max hold floating point values.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return The clamp field configuration.
template <typename backend_t>
typename backend_t::configuration_t clampConfigurationFloat(
    const Acts::Vector3& min, const Acts::Vector3& max) {
  return {{std::nextafter(static_cast<float>(min[0]),
                          std::numeric_limits<float>::infinity()),
           std::nextafter(static_cast<float>(min[1]),
                          std::numeric_limits<float>::infinity()),
           std::nextafter(static_cast<float>(min[2]),
                          std::numeric_limits<float>::infinity())},
          {std::nextafter(static_cast<float>(max[0]),
                          -std::numeric_limits<float>::infinity()),
           std::nextafter(static_cast<float>(max[1]),
                          -std::numeric_limits<float>::infinity()),
           std::nextafter(static_cast<float>(max[2]),
                          -std::numeric_limits<float>::infinity())}};
}

/// @brief Creates a covfie field from a generic magnetic field.
/// @param magneticField The generic magnetic field.
/// @param cache The cache.
/// @param nPoints 3D array of containing the number of bins for each axis.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return A clamp affine linear strided covfie field.
template <typename magnetic_field_t>
InterpolatedField covfieFieldLinear(const magnetic_field_t& magneticField,
                                    typename magnetic_field_t::Cache& cache,
                                    const std::array<std::size_t, 3>& nPoints,
                                    const Acts::Vector3& min,
                                    const Acts::Vector3& max) {
  auto builder = newBuilder(magneticField, cache, nPoints, min, max);
  InterpolatedField field(covfie::make_parameter_pack(
      clampConfigurationFloat<InterpolatedField::backend_t>(min, max),
      affineConfiguration<InterpolatedField::backend_t::backend_t>(nPoints, min,
                                                                   max),
      InterpolatedField::backend_t::backend_t::backend_t::configuration_t{},
      builder.backend()));

  return field;
}

/// @brief Creates a covfie field from a magnetic field provider by sampling it.
/// @param magneticField The acts magnetic field provider.
/// @param cache The acts cache.
/// @param nPoints 3D array of containing the number of bins for each axis.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return A clamp affine linear strided covfie field.
InterpolatedField covfieField(const Acts::MagneticFieldProvider& magneticField,
                              Acts::MagneticFieldProvider::Cache& cache,
                              const std::array<std::size_t, 3>& nPoints,
                              const Acts::Vector3& min,
                              const Acts::Vector3& max) {
  return covfieFieldLinear(magneticField, cache, nPoints, min, max);
}

/// @brief Creates a covfie field from an interpolated magnetic field.
/// @param magneticField The acts interpolated magnetic field.
/// @return A clamp affine linear strided covfie field.
InterpolatedField covfieField(
    const Acts::InterpolatedMagneticField& magneticField) {
  Acts::MagneticFieldContext ctx;
  auto cache = magneticField.makeCache(ctx);
  const std::vector<double>& old_min = magneticField.getMin();
  const std::vector<double>& old_max = magneticField.getMax();
  const std::vector<std::size_t>& old_nbins = magneticField.getNBins();
  Acts::Vector3 min{old_min.at(0), old_min.at(1), old_min.at(2)};
  Acts::Vector3 max{old_max.at(0), old_max.at(1), old_max.at(2)};
  std::array<std::size_t, 3> nPoints{old_nbins.at(0), old_nbins.at(1),
                                     old_nbins.at(2)};
  return covfieFieldLinear(magneticField, cache, nPoints, min, max);
}

/// @brief Creates a covfie field from a constant B field.
/// @param magneticField The acts constant magnetic field.
/// @return A constant covfie field.
ConstantField covfieField(const Acts::ConstantBField& magneticField) {
  auto B = magneticField.getField();
  ConstantField field(
      covfie::make_parameter_pack(ConstantField::backend_t::configuration_t{
          static_cast<float>(B[0]), static_cast<float>(B[1]),
          static_cast<float>(B[2])}));
  return field;
}

}  // namespace Acts::CovfiePlugin

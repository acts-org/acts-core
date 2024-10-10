// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldError.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Interpolation.hpp"
#include "Acts/Utilities/Result.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace Acts {

class InterpolatedMagneticField : public MagneticFieldProvider {
 public:
  /// @brief get the number of bins for all axes of the field map
  ///
  /// @return vector returning number of bins for all field map axes
  virtual std::vector<std::size_t> getNBins() const = 0;

  /// @brief get the minimum value of all axes of the field map
  ///
  /// @return vector returning the minima of all field map axes
  virtual std::vector<double> getMin() const = 0;

  /// @brief get the maximum value of all axes of the field map
  ///
  /// @return vector returning the maxima of all field map axes
  virtual std::vector<double> getMax() const = 0;

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @param [in] position global 3D position
  /// @return @c true if position is inside the defined look-up grid,
  ///         otherwise @c false
  virtual bool isInside(const Vector3& position) const = 0;

  /// Get a field value without checking if the lookup position is within the
  /// interpolation domain.
  ///
  /// @param position The lookup position in 3D
  /// @return The field value at @p position
  virtual Vector3 getFieldUnchecked(const Vector3& position) const = 0;
};

/// @ingroup MagneticField
/// @brief interpolate magnetic field value from field values on a given grid
///
/// This class implements a magnetic field service which is initialized by a
/// field map defined by:
/// - a list of field values on a regular grid in some n-Dimensional space,
/// - a transformation of global 3D coordinates onto this n-Dimensional
/// space.
/// - a transformation of local n-Dimensional magnetic field coordinates into
/// global (cartesian) 3D coordinates
///
/// The magnetic field value for a given global position is then determined by:
/// - mapping the position onto the grid,
/// - looking up the magnetic field values on the closest grid points,
/// - doing a linear interpolation of these magnetic field values.
///
/// @tparam grid_t The Grid type which provides the field storage and
/// interpolation
template <typename grid_t>
class InterpolatedBFieldMap : public InterpolatedMagneticField {
 public:
  using Grid = grid_t;
  using FieldType = typename Grid::value_type;
  static constexpr std::size_t DIM_POS = Grid::DIM;

  /// @brief struct representing smallest grid unit in magnetic field grid
  ///
  /// This type encapsulate all required information to perform linear
  /// interpolation of magnetic field values within a confined 3D volume.
  struct FieldCell {
    /// number of corner points defining the confining hyper-box
    static constexpr unsigned int N = 1 << DIM_POS;

   public:
    /// @brief default constructor
    ///
    /// @param [in] lowerLeft   generalized lower-left corner of hyper box
    ///                         (containing the minima of the hyper box along
    ///                         each Dimension)
    /// @param [in] upperRight  generalized upper-right corner of hyper box
    ///                         (containing the maxima of the hyper box along
    ///                         each Dimension)
    /// @param [in] fieldValues field values at the hyper box corners sorted in
    ///                         the canonical order defined in Acts::interpolate
    FieldCell(std::array<double, DIM_POS> lowerLeft,
              std::array<double, DIM_POS> upperRight,
              std::array<Vector3, N> fieldValues)
        : m_lowerLeft(std::move(lowerLeft)),
          m_upperRight(std::move(upperRight)),
          m_fieldValues(std::move(fieldValues)) {}

    /// @brief retrieve field at given position
    ///
    /// @param [in] position global 3D position
    /// @return magnetic field value at the given position
    ///
    /// @pre The given @c position must lie within the current field cell.
    Vector3 getField(const ActsVector<DIM_POS>& position) const {
      // defined in Interpolation.hpp
      return interpolate(position, m_lowerLeft, m_upperRight, m_fieldValues);
    }

    /// @brief check whether given 3D position is inside this field cell
    ///
    /// @param [in] position global 3D position
    /// @return @c true if position is inside the current field cell,
    ///         otherwise @c false
    bool isInside(const ActsVector<DIM_POS>& position) const {
      for (unsigned int i = 0; i < DIM_POS; ++i) {
        if (position[i] < m_lowerLeft[i] || position[i] >= m_upperRight[i]) {
          return false;
        }
      }
      return true;
    }

   private:
    /// generalized lower-left corner of the confining hyper-box
    std::array<double, DIM_POS> m_lowerLeft;

    /// generalized upper-right corner of the confining hyper-box
    std::array<double, DIM_POS> m_upperRight;

    /// @brief magnetic field vectors at the hyper-box corners
    ///
    /// @note These values must be order according to the prescription detailed
    ///       in Acts::interpolate.
    std::array<Vector3, N> m_fieldValues;
  };

  struct Cache {
    /// @brief Constructor with magnetic field context
    Cache(const MagneticFieldContext& /*mctx*/) {}

    std::optional<FieldCell> fieldCell;
    bool initialized = false;
  };

  /// @brief  Config structure for the interpolated B field map
  struct Config {
    /// @brief mapping of global 3D coordinates (cartesian)
    /// onto grid space
    std::function<ActsVector<DIM_POS>(const Vector3&)> transformPos;

    /// @brief calculating the global 3D coordinates
    /// (cartesian) of the magnetic field with the local n dimensional field and
    /// the global 3D position as input
    std::function<Vector3(const FieldType&, const Vector3&)> transformBField;

    /// @brief grid storing magnetic field values
    Grid grid;

    /// @brief global B-field scaling factor
    ///
    /// @note Negative values for @p scale are accepted and will invert the
    ///       direction of the magnetic field.
    double scale = 1.;
  };

  /// @brief default constructor
  ///
  InterpolatedBFieldMap(Config cfg) : m_cfg{std::move(cfg)} {
    typename Grid::index_t minBin{};
    minBin.fill(1);
    m_lowerLeft = m_cfg.grid.lowerLeftBinEdge(minBin);
    m_upperRight = m_cfg.grid.lowerLeftBinEdge(m_cfg.grid.numLocalBins());
  }

  /// @brief retrieve field cell for given position
  ///
  /// @param [in] position global 3D position
  /// @return field cell containing the given global position
  ///
  /// @pre The given @c position must lie within the range of the underlying
  ///      magnetic field map.
  Result<FieldCell> getFieldCell(const Vector3& position) const {
    const auto& gridPosition = m_cfg.transformPos(position);
    const auto& indices = m_cfg.grid.localBinsFromPosition(gridPosition);
    const auto& lowerLeft = m_cfg.grid.lowerLeftBinEdge(indices);
    const auto& upperRight = m_cfg.grid.upperRightBinEdge(indices);

    // loop through all corner points
    constexpr std::size_t nCorners = 1 << DIM_POS;
    std::array<Vector3, nCorners> neighbors;
    const auto& cornerIndices = m_cfg.grid.closestPointsIndices(gridPosition);

    if (!isInsideLocal(gridPosition)) {
      return MagneticFieldError::OutOfBounds;
    }

    std::size_t i = 0;
    for (std::size_t index : cornerIndices) {
      neighbors.at(i++) = m_cfg.transformBField(m_cfg.grid.at(index), position);
    }

    assert(i == nCorners);

    return FieldCell(lowerLeft, upperRight, std::move(neighbors));
  }

  /// @brief get the number of bins for all axes of the field map
  ///
  /// @return vector returning number of bins for all field map axes
  std::vector<std::size_t> getNBins() const final {
    auto nBinsArray = m_cfg.grid.numLocalBins();
    return std::vector<std::size_t>(nBinsArray.begin(), nBinsArray.end());
  }

  /// @brief get the minimum value of all axes of the field map
  ///
  /// @return vector returning the minima of all field map axes
  std::vector<double> getMin() const final {
    return std::vector<double>(m_lowerLeft.begin(), m_lowerLeft.end());
  }

  /// @brief get the maximum value of all axes of the field map
  ///
  /// @return vector returning the maxima of all field map axes
  std::vector<double> getMax() const final {
    return std::vector<double>(m_upperRight.begin(), m_upperRight.end());
  }

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @param [in] position global 3D position
  /// @return @c true if position is inside the defined look-up grid,
  ///         otherwise @c false
  bool isInside(const Vector3& position) const final {
    return isInsideLocal(m_cfg.transformPos(position));
  }

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @param [in] gridPosition local N-D position
  /// @return @c true if position is inside the defined look-up grid,
  ///         otherwise @c false
  bool isInsideLocal(const ActsVector<DIM_POS>& gridPosition) const {
    for (unsigned int i = 0; i < DIM_POS; ++i) {
      if (gridPosition[i] < m_lowerLeft[i] ||
          gridPosition[i] >= m_upperRight[i]) {
        return false;
      }
    }
    return true;
  }

  /// @brief Get a const reference on the underlying grid structure
  ///
  /// @return grid reference
  const Grid& getGrid() const { return m_cfg.grid; }

  /// @copydoc MagneticFieldProvider::makeCache(const MagneticFieldContext&) const
  MagneticFieldProvider::Cache makeCache(
      const MagneticFieldContext& mctx) const final {
    return MagneticFieldProvider::Cache{std::in_place_type<Cache>, mctx};
  }

  /// @brief retrieve field at given position
  ///
  /// @param [in] position global 3D position
  /// @return magnetic field value at the given position
  ///
  /// @pre The given @c position must lie within the range of the underlying
  ///      magnetic field map.
  Result<Vector3> getField(const Vector3& position) const {
    const auto gridPosition = m_cfg.transformPos(position);
    if (!isInsideLocal(gridPosition)) {
      return Result<Vector3>::failure(MagneticFieldError::OutOfBounds);
    }

    return Result<Vector3>::success(
        m_cfg.transformBField(m_cfg.grid.interpolate(gridPosition), position));
  }

  Vector3 getFieldUnchecked(const Vector3& position) const final {
    const auto gridPosition = m_cfg.transformPos(position);
    return m_cfg.transformBField(m_cfg.grid.interpolate(gridPosition),
                                 position);
  }

  /// @copydoc MagneticFieldProvider::getField(const Vector3&,MagneticFieldProvider::Cache&) const
  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const final {
    Cache& lcache = cache.as<Cache>();
    const auto gridPosition = m_cfg.transformPos(position);
    if (!lcache.fieldCell || !(*lcache.fieldCell).isInside(gridPosition)) {
      auto res = getFieldCell(position);
      if (!res.ok()) {
        return Result<Vector3>::failure(res.error());
      }
      lcache.fieldCell = *res;
    }
    return Result<Vector3>::success((*lcache.fieldCell).getField(gridPosition));
  }

  /// @copydoc MagneticFieldProvider::getFieldGradient(const Vector3&,ActsMatrix<3,3>&,MagneticFieldProvider::Cache&) const
  ///
  /// @note currently the derivative is not calculated
  /// @note Cache is not used currently
  /// @todo return derivative
  Result<Vector3> getFieldGradient(
      const Vector3& position, ActsMatrix<3, 3>& derivative,
      MagneticFieldProvider::Cache& cache) const final {
    (void)derivative;
    return getField(position, cache);
  }

 private:
  Config m_cfg;

  typename Grid::point_t m_lowerLeft;
  typename Grid::point_t m_upperRight;
};

}  // namespace Acts

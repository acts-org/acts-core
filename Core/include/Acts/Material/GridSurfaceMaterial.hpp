// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/GridAccessHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <ostream>
#include <stdexcept>
#include <vector>

namespace Acts {

/// @brief  This is an accessor for cases where the material is directly stored
/// in the grid, it simply forwards the grid entry in const and non-const way.
struct GridMaterialAccessor {
  /// @brief  Direct const access to the material slap sorted in the grid
  /// @tparam grid_type the type of the grid, also defines the point type
  /// @param grid the grid
  /// @param point the lookup point (already casted from global, or filled from local)
  ///
  /// @return the material slab from the grid bin associated to the lookup point
  template <typename grid_type>
  inline const MaterialSlab& slab(
      grid_type& grid, const typename grid_type::point_t& point) const {
    return grid.atPosition(point);
  }

  /// @brief Scale the material (by scaling the thickness)
  ///
  /// @param grid the grid (ignored)
  /// @param scale the amount of the scaling
  ///
  /// @note this is not particularly fast
  template <typename grid_type>
  void scale(grid_type& grid, ActsScalar scale) {
    // Loop through the grid bins, get the indices and scale the material
    for (std::size_t ib = 0; ib < grid.size(); ++ib) {
      grid.at(ib).scaleThickness(scale);
    }
  }
};

/// @brief  This is an accessor for cases where the material is filled in a vector
/// and then indexed by the grid
struct IndexedMaterialAccessor {
  /// @brief The internal storage of the material
  std::vector<MaterialSlab> material;
  /// @brief  Direct const access to the material slap sorted in the grid
  /// @tparam grid_type the type of the grid, also defines the point type
  /// @param grid the grid
  /// @param point the lookup point (already casted from global, or filled from local)
  ///
  /// @return the material slab from the grid bin associated to the lookup point
  template <typename grid_type>
  inline const MaterialSlab& slab(
      const grid_type& grid, const typename grid_type::point_t& point) const {
    auto index = grid.atPosition(point);
    return material[index];
  }

  /// @brief Scale the material (by scaling the thickness)
  ///
  /// @param scale the amount of the scaling
  template <typename grid_type>
  void scale(grid_type& /*grid*/, ActsScalar scale) {
    for (auto& m : material) {
      m.scaleThickness(scale);
    }
  }
};

/// @brief  This is an accessor for cases where the material is filled in a global
/// material vector that is accessed from the different material grids.
struct GloballyIndexedMaterialAccessor {
  /// @brief The internal storage of the material
  std::shared_ptr<std::vector<MaterialSlab>> globalMaterial = nullptr;

  /// Indicate if you have entries bins across different grids, e.g. by
  /// running a compression/clustering algorithm.
  ///
  /// It is the responsibility of the user to set this flag correctly.
  bool sharedEntries = false;

  /// @brief  Direct const access to the material slap sorted in the grid
  ///
  /// @tparam grid_type the type of the grid, also defines the point type
  ///
  /// @param grid the grid holding the indices into the global material vector
  /// @param point the lookup point (already casted from global, or filled from local)
  ///
  /// @return the material slab from the grid bin associated to the lookup point
  template <typename grid_type>
  inline const MaterialSlab& slab(
      const grid_type& grid, const typename grid_type::point_t& point) const {
    auto index = grid.atPosition(point);
    return (*globalMaterial)[index];
  }

  /// @brief Scale the material (by scaling the thickness)
  ///
  /// @param grid the grid holding the indices into the global material vector
  /// @param scale the amount of the scaling
  ///
  /// @note this will scale only the bins touched by this grid, however,
  /// if there are shared bins, then it will throw an exception as the
  /// outcome is unpredictable.
  ///
  template <typename grid_type>
  void scale(grid_type& grid, ActsScalar scale) {
    if (sharedEntries) {
      throw std::invalid_argument(
          "GloballyIndexedMaterialAccessor: shared entry scaling is not "
          "supported.");
    }
    // Loop through the grid bins, get the indices and scale the material
    for (std::size_t ib = 0; ib < grid.size(); ++ib) {
      auto index = grid.at(ib);
      (*globalMaterial)[index].scaleThickness(scale);
    }
  }
};

/// @brief GridSurfaceMaterialT
///
/// It extends the @c ISurfaceMaterial base class and allows to create
/// material maps associated to a grid structure
///
/// @tparam grid_type is the type of the grid used here
/// @tparam material_accessor_type is the type of the accessor to the material
///
/// It is templated on the material type and a slab accessor type in order
/// to allow it to be used in the material recording as well.
template <typename grid_t, typename material_accessor_t = GridMaterialAccessor>
class GridSurfaceMaterialT : public ISurfaceMaterial {
 public:
  // Definition of bound (on surface) to grid local representation delegate
  using BoundToGridLocalDelegate =
      OwningDelegate<typename grid_t::point_t(const Vector2&),
                     GridAccess::IBoundToGridLocal>;

  // Definition of global to grid local representation delegate
  using GlobalToGridLocalDelegate =
      OwningDelegate<typename grid_t::point_t(const Vector3&),
                     GridAccess::IGlobalToGridLocal>;

  /// Broadcast grid type
  using grid_type = grid_t;

  /// Broadcast material accessor type
  using material_accessor_type = material_accessor_t;

  /// @brief Constructor for indexed surface material
  ///
  /// @param grid the index grid steering the access to the material vector
  /// @param materialAccessor the material accessor: from grid, from indexed vector
  /// @param boundToGridLocal the delegation from bound to grid local frame
  /// @param globalToGridLocal the delegation from global into grid local frame
  GridSurfaceMaterialT(grid_type&& grid,
                       material_accessor_type&& materialAccessor,
                       BoundToGridLocalDelegate boundToGridLocal,
                       GlobalToGridLocalDelegate globalToGridLocal)
      : m_grid(std::move(grid)),
        m_materialAccessor(std::move(materialAccessor)),
        m_globalToGridLocal(std::move(globalToGridLocal)),
        m_boundToGridLocal(std::move(boundToGridLocal)) {
    if (!m_globalToGridLocal.connected()) {
      throw std::invalid_argument(
          "GridSurfaceMaterialT: GlobalToGridLocalDelegate is not connected.");
    }
    if (!m_boundToGridLocal.connected()) {
      throw std::invalid_argument(
          "GridSurfaceMaterialT: BoundToGridLocalDelegate is not connected.");
    }
  }

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector2&) const
  const MaterialSlab& materialSlab(const Vector2& lp) const final {
    return m_materialAccessor.slab(m_grid, m_boundToGridLocal(lp));
  }

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector3&) const
  const MaterialSlab& materialSlab(const Vector3& gp) const final {
    return m_materialAccessor.slab(m_grid, m_globalToGridLocal(gp));
  }

  /// Scale operator
  ///
  /// @param scale is the scale factor applied
  ISurfaceMaterial& operator*=(ActsScalar scale) final {
    m_materialAccessor.scale(m_grid, scale);
    return (*this);
  }

  /// Output Method for std::ostream, to be overloaded by child classes
  std::ostream& toStream(std::ostream& sl) const final {
    sl << "GridSurfaceMaterial - material access via accessor.";
    return sl;
  }

  /// @brief Accessor to the grid
  const grid_type& grid() const { return m_grid; }

  /// @brief Accessor to the material accessor
  const material_accessor_type& materialAccessor() const {
    return m_materialAccessor;
  }

  /// @brief Accessor to the bound to grid local delegate
  const BoundToGridLocalDelegate& boundToGridLocal() const {
    return m_boundToGridLocal;
  }

  /// @brief Accessor to the global to grid local delegate
  const GlobalToGridLocalDelegate& globalToGridLocal() const {
    return m_globalToGridLocal;
  }

 private:
  /// @brief The grid
  grid_type m_grid;

  /// @brief The stored material accessor
  material_accessor_type m_materialAccessor;

  /// The global to grid local delegate
  GlobalToGridLocalDelegate m_globalToGridLocal;

  /// The bound to grid local delegate
  BoundToGridLocalDelegate m_boundToGridLocal;
};

// Indexed Surface material
template <typename grid_type>
using IndexedSurfaceMaterial =
    GridSurfaceMaterialT<grid_type, IndexedMaterialAccessor>;

// Globally Indexed Surface material
template <typename grid_type>
using GloballyIndexedSurfaceMaterial =
    GridSurfaceMaterialT<grid_type, GloballyIndexedMaterialAccessor>;

// Grid Surface material
template <typename grid_type>
using GridSurfaceMaterial = GridSurfaceMaterialT<grid_type>;

}  // namespace Acts

// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <iosfwd>

namespace Acts {

/// @class ProtoSurfaceMaterial
///
/// @brief proxy to SurfaceMaterial hand over BinUtility
///
/// The ProtoSurfaceMaterial class acts as a proxy to the SurfaceMaterial
/// to mark the layers and surfaces on which the material should be mapped on
/// at construction time of the geometry and to hand over the granularity of
/// of the material map with the bin Utility.

class ProtoSurfaceMaterial : public ISurfaceMaterial {
 public:
  /// Constructor without BinUtility - homogenous material
  ProtoSurfaceMaterial() = default;

  /// Constructor with BinUtility - multidimensional material
  ///
  /// @param binUtility a BinUtility determining the granularity
  ///        and binning of the material on the surface/layer
  /// @param mappingType is the type of surface mapping associated to the surface
  ProtoSurfaceMaterial(const BinUtility& binUtility,
                       MappingType mappingType = MappingType::Default);

  /// Copy constuctor
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterial(const ProtoSurfaceMaterial& smproxy) = default;

  /// Copy move constuctor
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterial(ProtoSurfaceMaterial&& smproxy) = default;

  /// Destructor
  ~ProtoSurfaceMaterial() override = default;

  /// Assignment operator
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterial& operator=(const ProtoSurfaceMaterial& smproxy) =
      default;

  /// Assigment move operator
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterial& operator=(ProtoSurfaceMaterial&& smproxy) = default;

  /// Scale operator
  ///
  /// @param scale The value to scale this material by
  ProtoSurfaceMaterial& operator*=(double scale) final;

  /// Return the BinUtility
  const BinUtility& binUtility() const;

  /// Return method for full material description of the Surface - from local
  /// coordinates
  ///
  /// @param lp is local positioning vector
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(const Vector2& lp) const final;

  /// Return method for full material description of the Surface - from the
  /// global coordinates
  ///
  /// @param gp is the global positioning vector
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(const Vector3& gp) const final;

  /// Direct access via bins to the MaterialSlab
  ///
  /// @param ib0 indicates the first bin
  /// @param ib1 indicates the seconf bin
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(size_t ib0, size_t ib1) const final;

  /// Output Method for std::ostream, to be overloaded by child classes
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// two dimensional BinUtility determining
  /// the granularity and binning of the
  /// material on the surface/layer
  BinUtility m_binUtility;

  /// Dummy material properties
  MaterialSlab m_materialSlab;
};
}  // namespace Acts

inline const Acts::MaterialSlab& Acts::ProtoSurfaceMaterial::materialSlab(
    const Vector2& /*lp*/) const {
  return (m_materialSlab);
}

inline const Acts::MaterialSlab& Acts::ProtoSurfaceMaterial::materialSlab(
    const Vector3& /*gp*/) const {
  return (m_materialSlab);
}

inline const Acts::MaterialSlab& Acts::ProtoSurfaceMaterial::materialSlab(
    size_t /*ib0*/, size_t /*ib1*/) const {
  return (m_materialSlab);
}

inline const Acts::BinUtility& Acts::ProtoSurfaceMaterial::binUtility() const {
  return m_binUtility;
}

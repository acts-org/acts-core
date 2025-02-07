// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <iosfwd>

namespace Acts {

/// @class ProtoVolumeMaterial
///
/// @brief proxy to VolumeMaterial hand over BinUtility
///
/// The ProtoVolumeMaterial class acts as a proxy to the VolumeMaterial
/// to mark the volume on which the material should be mapped on
/// at construction time of the geometry and to hand over the granularity of
/// of the material map with the bin Utility.

class ProtoVolumeMaterial : public IVolumeMaterial {
 public:
  /// Constructor without BinUtility - homogeneous material
  ProtoVolumeMaterial() = default;

  /// Constructor with BinUtility - multidimensional material
  ///
  /// @param binUtility a BinUtility determining the granularity
  ///        and binning of the material on the volume
  ProtoVolumeMaterial(const BinUtility& binUtility);

  /// Copy constructor
  ///
  /// @param vmproxy The source proxy
  ProtoVolumeMaterial(const ProtoVolumeMaterial& vmproxy) = default;

  /// Copy move constructor
  ///
  /// @param vmproxy The source proxy
  ProtoVolumeMaterial(ProtoVolumeMaterial&& vmproxy) = default;

  /// Destructor
  ///
  ~ProtoVolumeMaterial() override = default;

  /// Return the BinUtility
  const BinUtility& binUtility() const;

  /// Assignment operator
  ///
  /// @param vmproxy The source proxy
  ProtoVolumeMaterial& operator=(const ProtoVolumeMaterial& vmproxy) = default;

  /// Return the material
  const Material material(const Vector3& /*position*/) const final;

  /// Output Method for std::ostream
  ///
  /// @param sl The outoput stream
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  BinUtility m_binUtility;
  Material m_material;
};

inline const Acts::Material Acts::ProtoVolumeMaterial::material(
    const Acts::Vector3& /*position*/) const {
  return m_material;
}

inline const Acts::BinUtility& Acts::ProtoVolumeMaterial::binUtility() const {
  return m_binUtility;
}

}  // namespace Acts

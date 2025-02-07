// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <iosfwd>

namespace Acts {

/// @class ProtoSurfaceMaterial
///
/// @brief proxy to SurfaceMaterial hand over BinUtility or other suitable
/// binning description
///
/// The ProtoSurfaceMaterial class acts as a proxy to the SurfaceMaterial
/// to mark the layers and surfaces on which the material should be mapped on
/// at construction time of the geometry and to hand over the granularity of
/// of the material map with the bin Utility.
template <typename BinningType>
class ProtoSurfaceMaterialT : public ISurfaceMaterial {
 public:
  /// Constructor without binningType - homogeneous material
  ProtoSurfaceMaterialT() = default;

  /// Constructor with BinningType
  /// @param binning a binning description for the material map binning
  /// @param mappingType is the type of surface mapping associated to the surface
  ProtoSurfaceMaterialT(const BinningType& binning,
                        MappingType mappingType = MappingType::Default)
      : ISurfaceMaterial(1., mappingType), m_binning(binning) {}

  /// Copy constructor
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterialT(const ProtoSurfaceMaterialT<BinningType>& smproxy) =
      default;

  /// Copy move constructor
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterialT(ProtoSurfaceMaterialT<BinningType>&& smproxy) = default;

  /// Destructor
  ~ProtoSurfaceMaterialT() override = default;

  /// Assignment operator
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterialT<BinningType>& operator=(
      const ProtoSurfaceMaterialT<BinningType>& smproxy) = default;

  /// Assignment move operator
  ///
  /// @param smproxy The source proxy
  ProtoSurfaceMaterialT<BinningType>& operator=(
      ProtoSurfaceMaterialT<BinningType>&& smproxy) = default;

  /// Scale operation - dummy implementation
  ///
  ProtoSurfaceMaterialT<BinningType>& scale(double /*factor*/) final {
    return (*this);
  }

  /// Return the BinUtility
  const BinningType& binning() const { return (m_binning); }

  /// Return method for full material description of the Surface - from local
  /// coordinates
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(const Vector2& /*lp*/) const final {
    return (m_materialSlab);
  }

  /// Return method for full material description of the Surface - from the
  /// global coordinates
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(const Vector3& /*gp*/) const final {
    return (m_materialSlab);
  }

  /// Output Method for std::ostream, to be overloaded by child classes
  ///
  /// @param sl is the output stream
  std::ostream& toStream(std::ostream& sl) const final {
    sl << "Acts::ProtoSurfaceMaterial : " << std::endl;
    sl << m_binning.toString() << std::endl;
    return sl;
  }

 private:
  /// A binning description
  BinningType m_binning;

  /// Dummy material properties
  MaterialSlab m_materialSlab;
};

using ProtoSurfaceMaterial = ProtoSurfaceMaterialT<Acts::BinUtility>;

using ProtoGridSurfaceMaterial =
    ProtoSurfaceMaterialT<Acts::Experimental::BinningDescription>;

}  // namespace Acts

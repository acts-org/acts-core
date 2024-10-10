// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/Material.hpp"

namespace Acts {

class Material;

/// @class IVolumeMaterial
///
/// Virtual base class of volume material description
//
/// Material associated with a Volume (homogeneous, binned, interpolated)
class IVolumeMaterial {
 public:
  /// Virtual Destructor
  virtual ~IVolumeMaterial() = default;

  /// Access to actual material
  ///
  /// @param position is the request position for the material call
  /// @todo interface to change including 'cell'
  virtual const Material material(const Vector3& position) const = 0;

  /// @brief output stream operator
  ///
  /// Prints information about this object to the output stream using the
  /// virtual IVolumeeMaterial::toStream method
  ///
  /// @return modified output stream object
  friend std::ostream& operator<<(std::ostream& out,
                                  const IVolumeMaterial& vm) {
    vm.toStream(out);
    return out;
  }

  /// Output Method for std::ostream, to be overloaded by child classes
  virtual std::ostream& toStream(std::ostream& sl) const = 0;
};

}  // namespace Acts

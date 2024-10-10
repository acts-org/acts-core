// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Set the Geometry Context PLUGIN
#ifdef ACTS_CORE_GEOMETRYCONTEXT_PLUGIN
#include ACTS_CORE_GEOMETRYCONTEXT_PLUGIN
#else

#include "Acts/Utilities/detail/ContextType.hpp"

#include <ostream>

namespace Acts {

/// @brief This is the central definition of the Acts
/// payload object regarding detector geometry status (e.g. alignment)
///
/// It is propagated through the code to allow for event/thread
/// dependent geometry changes

class GeometryContext : public ContextType {
 public:
  /// Inherit all constructors
  using ContextType::ContextType;
  using ContextType::operator=;
};

/// Helper struct that stores an object and a context, and will print it to
/// an outstream.
/// This allows you to write
/// ```cpp
/// std::cout << surface->toStream(geoContext) << std::endl;
/// ```
template <typename T>
struct GeometryContextOstreamWrapper {
  GeometryContextOstreamWrapper(const T& object, const GeometryContext& gctx)
      : m_object(object), m_gctx(gctx) {}

  GeometryContextOstreamWrapper(const GeometryContextOstreamWrapper&) = delete;
  GeometryContextOstreamWrapper(GeometryContextOstreamWrapper&&) = delete;
  GeometryContextOstreamWrapper& operator=(
      const GeometryContextOstreamWrapper&) = delete;
  GeometryContextOstreamWrapper& operator=(GeometryContextOstreamWrapper&&) =
      delete;

  friend std::ostream& operator<<(
      std::ostream& os, const GeometryContextOstreamWrapper& osWrapper) {
    osWrapper.toStream(os);
    return os;
  }

 private:
  void toStream(std::ostream& os) const { m_object.toStreamImpl(m_gctx, os); }

  const T& m_object;
  const GeometryContext& m_gctx;
};

}  // namespace Acts

#endif

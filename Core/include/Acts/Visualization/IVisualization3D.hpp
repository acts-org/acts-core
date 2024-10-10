// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace Acts {

/// Partially abstract base class which provides an interface to visualization
/// helper classes. It provides a number of methods that all the helpers need to
/// conform to.
class IVisualization3D {
 public:
  using FaceType = std::vector<std::size_t>;

  static constexpr Color s_defaultColor = {120, 120, 120};

  /// Draw a vertex at a given location and a color.
  /// @param vtx The vertex position
  /// @param color The color
  ///
  virtual void vertex(const Vector3& vtx, Color color = s_defaultColor) = 0;

  /// Draw a face that connects a list of vertices.
  /// @note Depending on the helper implementation, out of plane vertices might
  /// be handled differently.
  /// @param vtxs The vertices that make up the face
  /// @param color The color of the face
  ///
  virtual void face(const std::vector<Vector3>& vtxs,
                    Color color = s_defaultColor) = 0;

  /// Draw a faces that connects a list of vertices - expert only
  ///
  /// @note Depending on the helper implementation, out of plane vertices might
  /// be handled differently.
  /// @param vtxs The vertices that make up the faceS
  /// @param faces The face presections (i.e. connecting vertices)
  /// @param color The color of the face
  ///
  virtual void faces(const std::vector<Vector3>& vtxs,
                     const std::vector<FaceType>& faces,
                     Color color = s_defaultColor) = 0;

  /// Draw a line from a vertex to another
  /// @param a The start vertex
  /// @param b The end vertex
  /// @param color The color of the line
  ///
  virtual void line(const Vector3& a, const Vector3& b,
                    Color color = s_defaultColor) = 0;

  /// Write the content of the helper to an outstream.
  /// @param os The output stream for file
  virtual void write(std::ostream& os) const = 0;

  /// Write the content of the helper to an outstream.
  /// @param path is the file system path for writing the file
  virtual void write(const std::filesystem::path& path) const = 0;

  /// Remove all contents of this helper
  ///
  virtual void clear() = 0;

  virtual ~IVisualization3D() = default;

  /// Start a new object context
  /// @param name The name of the object
  virtual void object(const std::string& name) = 0;
};

/// Overload of the << operator to facilitate writing to streams.
/// @param os The output stream
/// @param hlp The helper instance
inline std::ostream& operator<<(std::ostream& os, const IVisualization3D& hlp) {
  hlp.write(os);
  return os;
}
}  // namespace Acts

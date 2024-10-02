// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Acts {

/// This helper produces output in the OBJ format. Note that colors are not
/// supported in this implementation.
///
template <typename T = double>
class ObjVisualization3D : public IVisualization3D {
 public:
  static_assert(std::is_same_v<T, double> || std::is_same_v<T, float>,
                "Use either double or float");

  /// Stored value type, should be double or float
  using ValueType = T;

  /// Type of a vertex based on the value type
  using VertexType = Eigen::Matrix<ValueType, 3, 1>;

  /// Type of a line
  using LineType = std::pair<std::size_t, std::size_t>;

  /// Constructor that allows to set scalor and precision
  /// @param prec The output precision with std::setprecision
  /// @param scale An (optional) scaling for the writing out
  ObjVisualization3D(unsigned int prec = 4, double scale = 1.)
      : m_outputPrecision(prec), m_outputScalor(scale) {}

  /// @copydoc Acts::IVisualization3D::vertex()
  void vertex(const Vector3& vtx, Color color = s_defaultColor) final;

  /// @copydoc Acts::IVisualization3D::line()
  void line(const Vector3& a, const Vector3& b,
            Color color = s_defaultColor) final;

  /// @copydoc Acts::IVisualization3D::face()
  void face(const std::vector<Vector3>& vtxs,
            Color color = s_defaultColor) final;

  /// @copydoc Acts::IVisualization3D::faces()
  void faces(const std::vector<Vector3>& vtxs,
             const std::vector<FaceType>& faces,
             Color color = s_defaultColor) final;

  /// @copydoc Acts::IVisualization3D::write(const std::filesystem::path&) const
  void write(const std::filesystem::path& path) const final;

  /// @copydoc Acts::IVisualization3D::write(std::ostream&) const
  void write(std::ostream& os) const final;

  /// Write the object and the material file
  /// @param os the output stream for the object
  /// @param mos the output stream for the auxiliary material file
  void write(std::ostream& os, std::ostream& mos) const;

  ///  @copydoc Acts::IVisualization3D::clear()
  void clear() final;

 private:
  /// The output parameters
  unsigned int m_outputPrecision = 4;
  double m_outputScalor = 1.;
  /// The object data to be written
  std::vector<VertexType> m_vertices;
  std::vector<FaceType> m_faces;
  std::vector<LineType> m_lines;
  /// Map of colors to be written at given index position
  std::map<std::size_t, Color> m_lineColors;
  std::map<std::size_t, Color> m_vertexColors;
  std::map<std::size_t, Color> m_faceColors;
};

#ifndef DOXYGEN
#include "detail/ObjVisualization3D.ipp"
#endif

}  // namespace Acts

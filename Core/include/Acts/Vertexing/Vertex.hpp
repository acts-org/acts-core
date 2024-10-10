// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Vertexing/TrackAtVertex.hpp"

namespace Acts {

/// @class Vertex
/// @brief Class for storing vertex objects
class Vertex {
 public:
  /// @brief Default constructor
  Vertex() = default;

  /// @brief Construct for vertex at given 3d-position, sets covariance to zero
  ///
  /// @param position Vertex position
  Vertex(const Vector3& position);

  /// @brief Construct for vertex at given 4d-position, sets covariance to zero
  ///
  /// @param position Vertex position
  Vertex(const Vector4& position);

  /// @brief Vertex constructor
  ///
  /// @param position Vertex position
  /// @param covariance Position covariance matrix
  /// @param tracks Vector of tracks associated with the vertex
  Vertex(const Vector3& position, const SquareMatrix3& covariance,
         std::vector<TrackAtVertex> tracks);

  /// @brief Vertex constructor
  ///
  /// @param position Full vertex position
  /// @param covariance 4x4 covariance matrix
  /// @param tracks Vector of tracks associated with the vertex
  Vertex(const Vector4& position, const SquareMatrix4& covariance,
         std::vector<TrackAtVertex> tracks);

  /// @return Returns 3-position
  Vector3 position() const;

  /// @return Returns time
  ActsScalar time() const;

  /// @return Returns 4-position
  const Vector4& fullPosition() const;
  Vector4& fullPosition();

  /// @return Returns 4D position of the vertex seed
  const Vector4& fullSeedPosition() const;
  Vector4& fullSeedPosition();

  /// @return Returns position covariance
  SquareMatrix3 covariance() const;

  /// @return Returns 4x4 covariance
  const SquareMatrix4& fullCovariance() const;
  SquareMatrix4& fullCovariance();

  /// @return Returns vector of tracks associated with the vertex
  const std::vector<TrackAtVertex>& tracks() const;

  /// @return Returns pair of (chi2, numberDoF)
  std::pair<double, double> fitQuality() const;

  /// @brief Set position
  ///
  /// @param position Vertex position
  void setPosition(const Vector3& position);

  /// @brief Set position and time
  ///
  /// @param fullPosition Vertex position and time
  void setFullPosition(const Vector4& fullPosition);

  /// @brief Sets time
  ///
  /// @param time The time
  void setTime(ActsScalar time);

  /// @brief Sets 3x3 covariance
  ///
  /// @param covariance Position covariance matrix
  void setCovariance(const SquareMatrix3& covariance);

  /// @brief Sets 4x4 covariance
  ///
  /// @param covariance The 4x4 covariance matrix
  void setFullCovariance(const SquareMatrix4& covariance);

  /// @param tracks Vector of tracks at vertex
  void setTracksAtVertex(std::vector<TrackAtVertex> tracks);

  /// @param chiSquared Chi2 of fit
  /// @param numberDoF Number of degrees of freedom
  void setFitQuality(double chiSquared, double numberDoF);

  /// @param fitQuality pair of (chi2, numberDoF)
  void setFitQuality(std::pair<double, double> fitQuality);

 private:
  Vector4 m_position = Vector4::Zero();
  Vector4 m_seedPosition = Vector4::Zero();
  SquareMatrix4 m_covariance = SquareMatrix4::Zero();
  std::vector<TrackAtVertex> m_tracksAtVertex;
  double m_chiSquared = 0.;  // chi2 of the fit
  double m_numberDoF = 0.;   // number of degrees of freedom
};

}  // namespace Acts

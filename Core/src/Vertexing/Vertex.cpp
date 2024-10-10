// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/Vertex.hpp"

namespace Acts {

Vertex::Vertex(const Vector3& position) {
  m_position.head<3>() = position;
  m_seedPosition.head<3>() = position;
}

Vertex::Vertex(const Vector4& position)
    : m_position(position), m_seedPosition(position) {}

Vertex::Vertex(const Vector3& position, const SquareMatrix3& covariance,
               std::vector<TrackAtVertex> tracks)
    : m_tracksAtVertex(std::move(tracks)) {
  m_position.head<3>() = position;
  m_seedPosition.head<3>() = position;
  m_covariance.block<3, 3>(ePos0, ePos0) = covariance;
}

Vertex::Vertex(const Vector4& position, const SquareMatrix4& covariance,
               std::vector<TrackAtVertex> tracks)
    : m_position(position),
      m_seedPosition(position),
      m_covariance(covariance),
      m_tracksAtVertex(std::move(tracks)) {}

Vector3 Vertex::position() const {
  return VectorHelpers::position(m_position);
}

ActsScalar Vertex::time() const {
  return m_position[eTime];
}

const Vector4& Vertex::fullPosition() const {
  return m_position;
}

Vector4& Vertex::fullPosition() {
  return m_position;
}

const Vector4& Vertex::fullSeedPosition() const {
  return m_seedPosition;
}

Vector4& Vertex::fullSeedPosition() {
  return m_seedPosition;
}

SquareMatrix3 Vertex::covariance() const {
  return m_covariance.block<3, 3>(ePos0, ePos0);
}

const SquareMatrix4& Vertex::fullCovariance() const {
  return m_covariance;
}

SquareMatrix4& Vertex::fullCovariance() {
  return m_covariance;
}

const std::vector<TrackAtVertex>& Vertex::tracks() const {
  return m_tracksAtVertex;
}

std::pair<double, double> Vertex::fitQuality() const {
  return std::pair<double, double>(m_chiSquared, m_numberDoF);
}

void Vertex::setPosition(const Vector3& position) {
  m_position.head<3>() = position;
}

void Vertex::setFullPosition(const Vector4& fullPosition) {
  m_position = fullPosition;
}

void Vertex::setTime(ActsScalar time) {
  m_position[eTime] = time;
}

void Vertex::setCovariance(const SquareMatrix3& covariance) {
  m_covariance.setZero();
  m_covariance.block<3, 3>(ePos0, ePos0) = covariance;
}

void Vertex::setFullCovariance(const SquareMatrix4& covariance) {
  m_covariance = covariance;
}

void Vertex::setTracksAtVertex(std::vector<TrackAtVertex> tracks) {
  m_tracksAtVertex = std::move(tracks);
}

void Vertex::setFitQuality(double chiSquared, double numberDoF) {
  m_chiSquared = chiSquared;
  m_numberDoF = numberDoF;
}

void Vertex::setFitQuality(std::pair<double, double> fitQuality) {
  m_chiSquared = fitQuality.first;
  m_numberDoF = fitQuality.second;
}

}  // namespace Acts

// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/EventData/Measurement.hpp"

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"

namespace ActsExamples {

MeasurementContainer::MeasurementContainer() = default;

std::size_t MeasurementContainer::size() const {
  return m_entries.size();
}

void MeasurementContainer::reserve(std::size_t size) {
  m_geometryIds.reserve(size);
  m_subspaceIndices.reserve(size * 2);
  m_parameters.reserve(size * 2);
  m_covariances.reserve(size * 2 * 2);
}

std::size_t MeasurementContainer::addMeasurement(
    std::uint8_t size, Acts::GeometryIdentifier geometryId) {
  m_entries.push_back({m_subspaceIndices.size(), m_parameters.size(),
                       m_covariances.size(), size});
  m_geometryIds.emplace_back(geometryId);
  m_subspaceIndices.resize(m_subspaceIndices.size() + size);
  m_parameters.resize(m_parameters.size() + size);
  m_covariances.resize(m_covariances.size() + size * size);

  std::size_t index = m_entries.size() - 1;
  IndexSourceLink sourceLink(geometryId, index);
  m_orderedIndices.emplace_hint(m_orderedIndices.end(), sourceLink);

  return index;
}

MeasurementContainer::VariableProxy MeasurementContainer::at(
    std::size_t index) {
  return VariableProxy{*this, index};
}

MeasurementContainer::ConstVariableProxy MeasurementContainer::at(
    std::size_t index) const {
  return ConstVariableProxy{*this, index};
}

MeasurementContainer::VariableProxy MeasurementContainer::getMeasurement(
    std::size_t index) {
  return VariableProxy{*this, index};
}

MeasurementContainer::ConstVariableProxy MeasurementContainer::getMeasurement(
    std::size_t index) const {
  return ConstVariableProxy{*this, index};
}

MeasurementContainer::VariableProxy MeasurementContainer::makeMeasurement(
    std::uint8_t size, Acts::GeometryIdentifier geometryId) {
  return getMeasurement(addMeasurement(size, geometryId));
}

const MeasurementContainer::OrderedIndices&
MeasurementContainer::orderedIndices() const {
  return m_orderedIndices;
}

MeasurementContainer::iterator MeasurementContainer::begin() {
  return iterator{*this, 0};
}

MeasurementContainer::iterator MeasurementContainer::end() {
  return iterator{*this, m_entries.size()};
}

MeasurementContainer::const_iterator MeasurementContainer::begin() const {
  return const_iterator{*this, 0};
}

MeasurementContainer::const_iterator MeasurementContainer::end() const {
  return const_iterator{*this, m_entries.size()};
}

MeasurementContainer::const_iterator MeasurementContainer::cbegin() const {
  return const_iterator{*this, 0};
}

MeasurementContainer::const_iterator MeasurementContainer::cend() const {
  return const_iterator{*this, m_entries.size()};
}

}  // namespace ActsExamples

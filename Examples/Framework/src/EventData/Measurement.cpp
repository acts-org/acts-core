// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/EventData/Measurement.hpp"

namespace ActsExamples {

MeasurementContainer::MeasurementContainer() = default;

std::size_t MeasurementContainer::size() const {
  return m_entries.size();
}

void MeasurementContainer::reserve(std::size_t size) {
  m_sourceLinks.reserve(size);
  m_subspaceIndices.reserve(size * 2);
  m_parameters.reserve(size * 2);
  m_covariances.reserve(size * 2 * 2);
}

std::size_t MeasurementContainer::addMeasurement(std::uint8_t size) {
  m_entries.push_back({m_subspaceIndices.size(), m_parameters.size(),
                       m_covariances.size(), size});
  m_sourceLinks.emplace_back();
  m_subspaceIndices.resize(m_subspaceIndices.size() + size);
  m_parameters.resize(m_parameters.size() + size);
  m_covariances.resize(m_covariances.size() + size * size);
  return m_entries.size() - 1;
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
    std::uint8_t size) {
  return getMeasurement(addMeasurement(size));
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

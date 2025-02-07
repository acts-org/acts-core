// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Framework/DataHandle.hpp"

namespace ActsExamples {

void WriteDataHandleBase::initialize(const std::string& key) {
  if (key.empty()) {
    throw std::invalid_argument{"Write handle '" + fullName() +
                                "' cannot receive empty key"};
  }
  m_key = key;
}

bool WriteDataHandleBase::isCompatible(const DataHandleBase& other) const {
  return dynamic_cast<const ReadDataHandleBase*>(&other) != nullptr &&
         typeInfo() == other.typeInfo();
}

void ReadDataHandleBase::initialize(const std::string& key) {
  if (key.empty()) {
    throw std::invalid_argument{"Read handle '" + fullName() +
                                "' cannot receive empty key"};
  }
  m_key = key;
}

bool ReadDataHandleBase::isCompatible(const DataHandleBase& other) const {
  return dynamic_cast<const WriteDataHandleBase*>(&other) != nullptr &&
         typeInfo() == other.typeInfo();
}

}  // namespace ActsExamples

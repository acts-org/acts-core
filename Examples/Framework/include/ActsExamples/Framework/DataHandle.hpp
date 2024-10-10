// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/ThrowAssert.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <iostream>
#include <stdexcept>
#include <typeinfo>

namespace ActsExamples {

class DataHandleBase {
 protected:
  virtual ~DataHandleBase() = default;

  DataHandleBase(SequenceElement* parent, const std::string& name)
      : m_parent(parent), m_name(name) {}

  // We can't change addresses after construction
  DataHandleBase(const DataHandleBase&) = delete;
  DataHandleBase(DataHandleBase&&) = default;

 public:
  const std::string& key() const { return m_key.value(); }

  virtual const std::type_info& typeInfo() const = 0;

  bool isInitialized() const { return m_key.has_value(); }

  const std::string& name() const { return m_name; }

  void maybeInitialize(const std::string& key) {
    if (!key.empty()) {
      m_key = key;
    }
  }

  virtual bool isCompatible(const DataHandleBase& other) const = 0;

  std::string fullName() const { return m_parent->name() + "." + name(); }

 protected:
  SequenceElement* m_parent{nullptr};
  std::string m_name;
  std::optional<std::string> m_key{};
};

template <typename T>
class ReadDataHandle;

template <typename T>
class WriteDataHandle final : public DataHandleBase {
 public:
  WriteDataHandle(SequenceElement* parent, const std::string& name)
      : DataHandleBase{parent, name} {
    m_parent->registerWriteHandle(*this);
  }

  void operator()(const AlgorithmContext& ctx, T&& value) const {
    (*this)(ctx.eventStore, std::move(value));
  }

  void operator()(WhiteBoard& wb, T&& value) const {
    if (!isInitialized()) {
      throw std::runtime_error{"WriteDataHandle '" + fullName() +
                               "' not initialized"};
    }
    wb.add(m_key.value(), std::move(value));
  }

  void initialize(const std::string& key) {
    if (key.empty()) {
      throw std::invalid_argument{"Write handle '" + fullName() +
                                  "' cannot receive empty key"};
    }
    m_key = key;
  }

  bool isCompatible(const DataHandleBase& other) const override {
    return dynamic_cast<const ReadDataHandle<T>*>(&other) != nullptr;
  }

  const std::type_info& typeInfo() const override { return typeid(T); };
};

template <typename T>
class ReadDataHandle final : public DataHandleBase {
 public:
  ReadDataHandle(SequenceElement* parent, const std::string& name)
      : DataHandleBase{parent, name} {
    m_parent->registerReadHandle(*this);
  }

  void initialize(const std::string& key) {
    if (key.empty()) {
      throw std::invalid_argument{"Read handle '" + fullName() +
                                  "' cannot receive empty key"};
    }
    m_key = key;
  }

  const T& operator()(const AlgorithmContext& ctx) const {
    return (*this)(ctx.eventStore);
  }

  const T& operator()(const WhiteBoard& wb) const {
    if (!isInitialized()) {
      throw std::runtime_error{"ReadDataHandle '" + fullName() +
                               "' not initialized"};
    }
    return wb.get<T>(m_key.value());
  }

  bool isCompatible(const DataHandleBase& other) const override {
    return dynamic_cast<const WriteDataHandle<T>*>(&other) != nullptr;
  }

  const std::type_info& typeInfo() const override { return typeid(T); };
};

}  // namespace ActsExamples

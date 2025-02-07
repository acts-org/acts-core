// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/Framework/SequenceElement.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

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

class WriteDataHandleBase : public DataHandleBase {
 protected:
  WriteDataHandleBase(SequenceElement* parent, const std::string& name)
      : DataHandleBase{parent, name} {}

 public:
  void initialize(const std::string& key);

  bool isCompatible(const DataHandleBase& other) const final;
};

class ReadDataHandleBase : public DataHandleBase {
 protected:
  ReadDataHandleBase(SequenceElement* parent, const std::string& name)
      : DataHandleBase{parent, name} {}

 public:
  void initialize(const std::string& key);

  bool isCompatible(const DataHandleBase& other) const final;
};

template <typename T>
class WriteDataHandle final : public WriteDataHandleBase {
 public:
  WriteDataHandle(SequenceElement* parent, const std::string& name)
      : WriteDataHandleBase{parent, name} {
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

  const std::type_info& typeInfo() const override { return typeid(T); };
};

template <typename T>
class ReadDataHandle final : public ReadDataHandleBase {
 public:
  ReadDataHandle(SequenceElement* parent, const std::string& name)
      : ReadDataHandleBase{parent, name} {
    m_parent->registerReadHandle(*this);
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

  const std::type_info& typeInfo() const override { return typeid(T); };
};

}  // namespace ActsExamples

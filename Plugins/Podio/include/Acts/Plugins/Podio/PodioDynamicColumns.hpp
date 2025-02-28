// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>
#include <string>
#include <string_view>

#include <podio/Frame.h>
#include <podio/UserDataCollection.h>

namespace Acts::podio_detail {

struct ConstDynamicColumnBase {
  explicit ConstDynamicColumnBase(std::string_view name) : m_name{name} {}

  virtual ~ConstDynamicColumnBase() = default;

  virtual std::any get(std::size_t i) const = 0;

  virtual std::size_t size() const = 0;

 protected:
  std::string m_name;
};

template <typename T>
struct ConstDynamicColumn : public ConstDynamicColumnBase {
  ConstDynamicColumn(std::string_view name,
                     const podio::UserDataCollection<T>& collection)
      : ConstDynamicColumnBase(name), m_collection{collection} {}

  std::any get(std::size_t i) const override {
    return &m_collection.vec().at(i);
  }
  std::size_t size() const override { return m_collection.size(); }

  const podio::UserDataCollection<T>& m_collection;
};

struct DynamicColumnBase : public ConstDynamicColumnBase {
  explicit DynamicColumnBase(std::string_view name)
      : ConstDynamicColumnBase{name} {}

  virtual std::any get(std::size_t i) = 0;
  std::any get(std::size_t i) const override = 0;

  virtual void add() = 0;
  virtual void clear() = 0;
  virtual void erase(std::size_t i) = 0;
  virtual void copyFrom(std::size_t dstIdx, const DynamicColumnBase& src,
                        std::size_t srcIdx) = 0;
  virtual void copyFrom(std::size_t dstIdx, const std::any& srcPtr) = 0;

  virtual std::unique_ptr<DynamicColumnBase> clone(
      bool empty = false) const = 0;

  virtual std::unique_ptr<ConstDynamicColumnBase> asConst() const = 0;

  virtual void releaseInto(podio::Frame& frame, const std::string& prefix) = 0;
};

template <typename T>
struct DynamicColumn : public DynamicColumnBase {
  explicit DynamicColumn(std::string_view name,
                         podio::UserDataCollection<T> collection = {})
      : DynamicColumnBase(name), m_collection{std::move(collection)} {}

  std::any get(std::size_t i) override { return &m_collection.vec().at(i); }

  std::any get(std::size_t i) const override {
    return &m_collection.vec().at(i);
  }

  void add() override { m_collection.vec().emplace_back(); }
  void clear() override { m_collection.clear(); }
  void erase(std::size_t i) override {
    m_collection.vec().erase(m_collection.vec().begin() + i);
  }
  std::size_t size() const override { return m_collection.size(); }

  std::unique_ptr<DynamicColumnBase> clone(bool empty) const override {
    if (empty) {
      return std::make_unique<DynamicColumn<T>>(m_name);
    }
    podio::UserDataCollection<T> copy;
    copy.vec().reserve(m_collection.size());
    for (const T& v : m_collection) {
      copy.push_back(v);
    }
    return std::make_unique<DynamicColumn<T>>(m_name, std::move(copy));
  }

  std::unique_ptr<ConstDynamicColumnBase> asConst() const override {
    return std::make_unique<ConstDynamicColumn<T>>(m_name, m_collection);
  }

  void copyFrom(std::size_t dstIdx, const DynamicColumnBase& src,
                std::size_t srcIdx) override {
    const auto* other = dynamic_cast<const DynamicColumn<T>*>(&src);
    assert(other != nullptr &&
           "Source column is not of same type as destination");
    m_collection.vec().at(dstIdx) = other->m_collection.vec().at(srcIdx);
  }

  void copyFrom(std::size_t dstIdx, const std::any& srcPtr) override {
    const auto* other = std::any_cast<const T*>(srcPtr);
    assert(other != nullptr &&
           "Source column is not of same type as destination");
    m_collection.vec().at(dstIdx) = *other;
  }

  void releaseInto(podio::Frame& frame, const std::string& prefix) override {
    frame.put(std::move(m_collection), prefix + m_name);
  }

  podio::UserDataCollection<T> m_collection;
};

}  // namespace Acts::podio_detail

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <utility>

namespace Acts::detail {

/// Internal holder type for referencing a backend without ownership
template <typename T>
struct RefHolder {
  T* ptr;

  explicit RefHolder(T* _ptr) : ptr{_ptr} {}
  explicit RefHolder(T& ref) : ptr{&ref} {}

  const T& operator*() const { return *ptr; }
  T& operator*() { return *ptr; }

  const T* operator->() const { return ptr; }
  T* operator->() { return ptr; }
};

/// Internal holder type for referencing a backend without ownership that is
/// const
template <typename T>
struct ConstRefHolder {
  const T* ptr;

  explicit ConstRefHolder(const T* _ptr) : ptr{_ptr} {}
  explicit ConstRefHolder(const T& ref) : ptr{&ref} {}

  const T& operator*() const { return *ptr; }

  const T* operator->() const { return ptr; }
};

/// Internal holder type holding a backend container by value
template <typename T>
struct ValueHolder {
  T val;

  // Let's be clear with the user that we take the ownership
  // Only require rvalues and avoid hidden copies
  ValueHolder(T& _val) = delete;
  // @FIXME: Ideally we want this to be explicit, but cannot be explicit,
  // because using an explicit constructor and a deduction guide leads to
  // a SEGFAULT in GCC11 (an up?). Re-evaluate down the line
  /* explicit */ ValueHolder(T&& _val) : val{std::move(_val)} {}

  // Does it make sense to allow copy operations?

  const T& operator*() const { return val; }
  T& operator*() { return val; }

  const T* operator->() const { return &val; }
  T* operator->() { return &val; }
};

}  // namespace Acts::detail

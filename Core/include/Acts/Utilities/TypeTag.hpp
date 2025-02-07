// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace Acts {

template <typename T>
struct TypeTag {};
template <typename T>
constexpr TypeTag<T> Type;

}  // namespace Acts

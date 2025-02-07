// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <type_traits>

namespace Acts {
template <bool C, typename T>
using const_if_t = std::conditional_t<C, const T, T>;
}  // namespace Acts

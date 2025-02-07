// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <type_traits>
namespace Acts {
template <typename stepper_t, typename navigator_t>
struct SupportsBoundParameters : public std::false_type {};

template <typename stepper_t, typename navigator_t>
constexpr bool SupportsBoundParameters_v =
    SupportsBoundParameters<stepper_t, navigator_t>::value;
}  // namespace Acts

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Covfie/FieldConversion.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"

#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {

namespace {
template <typename field_t, typename scalar_type>
void declareCovfieField(py::module& m, const std::string& fieldName) {
  using view_t = typename field_t::view_t;
  m.def("toView",
        [](const field_t& field) { return typename field_t::view_t(field); });
  py::class_<field_t, std::shared_ptr<field_t>>(m, fieldName.c_str());
  py::class_<view_t, std::shared_ptr<view_t>>(
      m, (fieldName + std::string("View")).c_str())
      .def("at", &view_t::template at<scalar_type, scalar_type, scalar_type>);
}
}  // namespace

void addCovfie(Context& ctx) {
  auto main = ctx.get("main");
  auto m = main.def_submodule("covfie", "Submodule for covfie conversion");

  py::class_<covfie::array::array<float, 3ul>,
             std::shared_ptr<covfie::array::array<float, 3ul>>>(m,
                                                                "ArrayFloat3")
      .def("at", [](const covfie::array::array<float, 3ul>& self,
                    std::size_t i) { return self[i]; });

  declareCovfieField<Acts::CovfiePlugin::ConstantField, float>(
      m, "CovfieConstantField");
  declareCovfieField<Acts::CovfiePlugin::InterpolatedField, float>(
      m, "CovfieAffineLinearStridedField");

  m.def("makeCovfieField",
        py::overload_cast<const Acts::InterpolatedMagneticField&>(
            &Acts::CovfiePlugin::covfieField));
  m.def("makeCovfieField", py::overload_cast<const Acts::ConstantBField&>(
                               &Acts::CovfiePlugin::covfieField));
  m.def("makeCovfieField",
        py::overload_cast<const Acts::MagneticFieldProvider&,
                          Acts::MagneticFieldProvider::Cache&,
                          const std::array<std::size_t, 3>&,
                          const Acts::Vector3&, const Acts::Vector3&>(
            &Acts::CovfiePlugin::covfieField));
}

}  // namespace Acts::Python

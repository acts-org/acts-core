// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Covfie/FieldConversion.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"

#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {

namespace {
template <typename field_t>
void declareCovfieField(py::module& m, const std::string& fieldName) {
  using view_t = typename field_t::view_t;
  m.def("toView",
        [](const field_t& field) { return typename field_t::view_t(field); });
  py::class_<field_t, std::shared_ptr<field_t>>(m, fieldName.c_str());
  py::class_<view_t, std::shared_ptr<view_t>>(
      m, (fieldName + std::string("View")).c_str())
      .def("at", &view_t::template at<float, float, float>);
}
}  // namespace

void addCovfie(Context& ctx) {
  auto main = ctx.get("main");
  auto m = main.def_submodule("covfie", "Submodule for covfie conversion");

  declareCovfieField<Acts::CovfiePlugin::ConstantField>(m,
                                                        "CovfieConstantField");
  declareCovfieField<Acts::CovfiePlugin::InterpolatedField>(
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

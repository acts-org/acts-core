// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>
#include <unordered_map>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <pybind11/pybind11.h>

namespace Acts {
namespace Concepts {
template <typename T>
concept has_write_method =
    requires(T& t, const ActsExamples::AlgorithmContext& ctx) {
      { t.write(ctx) } -> std::same_as<ActsExamples::ProcessCode>;
    };
}  // namespace Concepts

namespace Python {

struct Context {
  std::unordered_map<std::string, pybind11::module_> modules;

  pybind11::module_& get(const std::string& name) { return modules.at(name); }

  template <typename... Args>
  auto get(Args&&... args)
    requires(sizeof...(Args) >= 2)
  {
    return std::make_tuple((modules.at(args))...);
  }
};

template <typename T, typename Ur, typename Ut>
void pythonRangeProperty(T& obj, const std::string& name, Ur Ut::*begin,
                         Ur Ut::*end) {
  obj.def_property(
      name.c_str(), [=](Ut& self) { return std::pair{self.*begin, self.*end}; },
      [=](Ut& self, std::pair<Ur, Ur> p) {
        self.*begin = p.first;
        self.*end = p.second;
      });
}

inline void patchClassesWithConfig(pybind11::module_& m) {
  pybind11::module::import("acts._adapter").attr("_patch_config")(m);
}

template <typename T>
void patchKwargsConstructor(T& c) {
  pybind11::module::import("acts._adapter").attr("_patchKwargsConstructor")(c);
}
}  // namespace Python
}  // namespace Acts

#define ACTS_PYTHON_MEMBER(name) \
  _binding_instance.def_readwrite(#name, &_struct_type::name)

#define ACTS_PYTHON_STRUCT_BEGIN(obj, cls)          \
  {                                                 \
    [[maybe_unused]] auto& _binding_instance = obj; \
    using _struct_type = cls;                       \
    do {                                            \
    } while (0)

#define ACTS_PYTHON_STRUCT_END() \
  }                              \
  do {                           \
  } while (0)

/// This macro is needed to use the BOOST_PP_SEQ_FOR_EACH loop macro
#define ACTS_PYTHON_MEMBER_LOOP(r, data, elem) ACTS_PYTHON_MEMBER(elem);

/// A macro that uses Boost.Preprocessor to create the python binding for and
/// algorithm and the additional config struct.
#define ACTS_PYTHON_DECLARE_ALGORITHM(algorithm, mod, name, ...)              \
  do {                                                                        \
    using Alg = algorithm;                                                    \
    using Config = Alg::Config;                                               \
    auto alg =                                                                \
        py::class_<Alg, ActsExamples::IAlgorithm, std::shared_ptr<Alg>>(mod,  \
                                                                        name) \
            .def(py::init<const Config&, Acts::Logging::Level>(),             \
                 py::arg("config"), py::arg("level"))                         \
            .def_property_readonly("config", &Alg::config);                   \
                                                                              \
    auto c = py::class_<Config>(alg, "Config").def(py::init<>());             \
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);                                      \
    BOOST_PP_SEQ_FOR_EACH(ACTS_PYTHON_MEMBER_LOOP, _,                         \
                          BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))              \
    ACTS_PYTHON_STRUCT_END();                                                 \
  } while (0)

/// Similar as above for writers
#define ACTS_PYTHON_DECLARE_WRITER(writer, mod, name, ...)                  \
  do {                                                                      \
    using Writer = writer;                                                  \
    using Config = Writer::Config;                                          \
    auto w =                                                                \
        py::class_<Writer, ActsExamples::IWriter, std::shared_ptr<Writer>>( \
            mod, name)                                                      \
            .def(py::init<const Config&, Acts::Logging::Level>(),           \
                 py::arg("config"), py::arg("level"))                       \
            .def_property_readonly("config", &Writer::config);              \
                                                                            \
    constexpr bool has_write_method =                                       \
        Acts::Concepts::has_write_method<Writer>;                           \
                                                                            \
    if constexpr (has_write_method) {                                       \
      w.def("write", &Writer::write);                                       \
    }                                                                       \
                                                                            \
    auto c = py::class_<Config>(w, "Config").def(py::init<>());             \
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);                                    \
    BOOST_PP_SEQ_FOR_EACH(ACTS_PYTHON_MEMBER_LOOP, _,                       \
                          BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))            \
    ACTS_PYTHON_STRUCT_END();                                               \
  } while (0)

/// Similar as above for readers
#define ACTS_PYTHON_DECLARE_READER(reader, mod, name, ...)                  \
  do {                                                                      \
    using Reader = reader;                                                  \
    using Config = Reader::Config;                                          \
    auto r =                                                                \
        py::class_<Reader, ActsExamples::IReader, std::shared_ptr<Reader>>( \
            mod, name)                                                      \
            .def(py::init<const Config&, Acts::Logging::Level>(),           \
                 py::arg("config"), py::arg("level"))                       \
            .def_property_readonly("config", &Reader::config);              \
                                                                            \
    auto c = py::class_<Config>(r, "Config").def(py::init<>());             \
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);                                    \
    BOOST_PP_SEQ_FOR_EACH(ACTS_PYTHON_MEMBER_LOOP, _,                       \
                          BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))            \
    ACTS_PYTHON_STRUCT_END();                                               \
  } while (0)

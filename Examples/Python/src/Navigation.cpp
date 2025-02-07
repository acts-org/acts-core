// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/SurfaceArrayNavigationPolicy.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/TypeTag.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include <boost/core/demangle.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {

struct AnyNavigationPolicyFactory : public Acts::NavigationPolicyFactory {
  virtual std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<TryAllNavigationPolicy> /*type*/) = 0;

  virtual std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<SurfaceArrayNavigationPolicy> /*type*/,
      SurfaceArrayNavigationPolicy::Config config) = 0;

  virtual std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<TryAllNavigationPolicy> /*type*/,
      TryAllNavigationPolicy::Config config) = 0;
};

template <typename Factory = detail::NavigationPolicyFactoryImpl<>,
          typename... Policies>
struct NavigationPolicyFactoryT : public AnyNavigationPolicyFactory {
  explicit NavigationPolicyFactoryT(Factory impl)
    requires(sizeof...(Policies) > 0)
      : m_impl(std::move(impl)) {}

  NavigationPolicyFactoryT()
    requires(sizeof...(Policies) == 0)
      : m_impl{} {}

  std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<TryAllNavigationPolicy> /*type*/) override {
    return add<TryAllNavigationPolicy>();
  }

  std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<SurfaceArrayNavigationPolicy> /*type*/,
      SurfaceArrayNavigationPolicy::Config config) override {
    return add<SurfaceArrayNavigationPolicy>(std::move(config));
  }

  std::unique_ptr<AnyNavigationPolicyFactory> add(
      TypeTag<TryAllNavigationPolicy> /*type*/,
      TryAllNavigationPolicy::Config config) override {
    return add<TryAllNavigationPolicy>(config);
  }

  std::unique_ptr<INavigationPolicy> build(
      const GeometryContext& gctx, const TrackingVolume& volume,
      const Logger& logger) const override {
    if constexpr (sizeof...(Policies) > 0) {
      return m_impl.build(gctx, volume, logger);
    } else {
      throw std::runtime_error("No policies added to the factory");
    }
  }

 private:
  template <typename T, typename... Args>
  std::unique_ptr<AnyNavigationPolicyFactory> add(Args&&... args) {
    if constexpr (!(std::is_same_v<T, Policies> || ...)) {
      auto impl =
          std::move(m_impl).template add<T>(std::forward<Args>(args)...);
      return std::make_unique<
          NavigationPolicyFactoryT<decltype(impl), Policies..., T>>(
          std::move(impl));
    } else {
      throw std::invalid_argument("Policy already added to the factory");
    }
  }

  Factory m_impl;
};

class NavigationPolicyFactory : public Acts::NavigationPolicyFactory {
 public:
  // This overload is for all the navigation policies that don't have extra
  // arguments
  NavigationPolicyFactory& addNoArguments(const py::object& cls) {
    auto m = py::module_::import("acts");
    if (py::object o = m.attr("TryAllNavigationPolicy"); cls.is(o)) {
      m_impl = m_impl->add(Type<TryAllNavigationPolicy>);
    }
    // Add other policies here
    return *this;
  }

  NavigationPolicyFactory& addSurfaceArray(
      const py::object& /*cls*/,
      const SurfaceArrayNavigationPolicy::Config& config) {
    m_impl = m_impl->add(Type<SurfaceArrayNavigationPolicy>, config);
    return *this;
  }

  NavigationPolicyFactory& addTryAll(
      const py::object& /*cls*/, const TryAllNavigationPolicy::Config& config) {
    m_impl = m_impl->add(Type<TryAllNavigationPolicy>, config);
    return *this;
  }

  std::unique_ptr<INavigationPolicy> build(
      const GeometryContext& gctx, const TrackingVolume& volume,
      const Logger& logger) const override {
    return m_impl->build(gctx, volume, logger);
  }

 private:
  std::unique_ptr<AnyNavigationPolicyFactory> m_impl =
      std::make_unique<NavigationPolicyFactoryT<>>();
};

namespace Test {
class DetectorElementStub : public DetectorElementBase {
 public:
  DetectorElementStub() : DetectorElementBase() {}

  const Transform3& transform(const GeometryContext&) const override {
    return m_transform;
  }

  /// Return surface representation - const return pattern
  const Surface& surface() const override {
    throw std::runtime_error("Not implemented");
  }

  /// Non-const return pattern
  Surface& surface() override { throw std::runtime_error("Not implemented"); }

  /// Returns the thickness of the module
  /// @return double that indicates the thickness of the module
  double thickness() const override { return 0; }

 private:
  Transform3 m_transform;
};

}  // namespace Test

void addNavigation(Context& ctx) {
  auto m = ctx.get("main");

  py::class_<Acts::NavigationPolicyFactory,
             std::shared_ptr<Acts::NavigationPolicyFactory>>(
      m, "_NavigationPolicyFactory");

  {
    auto tryAll =
        py::class_<TryAllNavigationPolicy>(m, "TryAllNavigationPolicy");
    using Config = TryAllNavigationPolicy::Config;
    auto c = py::class_<Config>(tryAll, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(portals);
    ACTS_PYTHON_MEMBER(sensitives);
    ACTS_PYTHON_STRUCT_END();
  }

  py::class_<NavigationPolicyFactory, Acts::NavigationPolicyFactory,
             std::shared_ptr<NavigationPolicyFactory>>(
      m, "NavigationPolicyFactory")
      // only to mirror the C++ API
      .def_static("make", []() { return NavigationPolicyFactory{}; })
      .def("add", &NavigationPolicyFactory::addNoArguments)
      .def("add", &NavigationPolicyFactory::addSurfaceArray)
      .def("add", &NavigationPolicyFactory::addTryAll)
      .def("_buildTest", [](NavigationPolicyFactory& self) {
        auto vol1 = std::make_shared<TrackingVolume>(
            Transform3::Identity(),
            std::make_shared<CylinderVolumeBounds>(30, 40, 100));
        vol1->setVolumeName("TestVolume");

        auto detElem = std::make_unique<Test::DetectorElementStub>();

        auto surface = Surface::makeShared<CylinderSurface>(
            Transform3::Identity(), std::make_shared<CylinderBounds>(30, 40));
        surface->assignDetectorElement(*detElem);

        vol1->addSurface(std::move(surface));

        std::unique_ptr<INavigationPolicy> result =
            self.build(GeometryContext{}, *vol1,
                       *getDefaultLogger("Test", Logging::VERBOSE));
      });

  {
    auto saPolicy = py::class_<SurfaceArrayNavigationPolicy>(
        m, "SurfaceArrayNavigationPolicy");

    using LayerType = SurfaceArrayNavigationPolicy::LayerType;
    py::enum_<LayerType>(saPolicy, "LayerType")
        .value("Cylinder", LayerType::Cylinder)
        .value("Disc", LayerType::Disc)
        .value("Plane", LayerType::Plane);

    using Config = SurfaceArrayNavigationPolicy::Config;
    auto c = py::class_<Config>(saPolicy, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(layerType);
    ACTS_PYTHON_MEMBER(bins);
    ACTS_PYTHON_STRUCT_END();
  }
}

}  // namespace Acts::Python

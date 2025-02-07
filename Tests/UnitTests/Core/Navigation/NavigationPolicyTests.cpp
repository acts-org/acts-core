// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/MultiNavigationPolicy.hpp"
#include "Acts/Navigation/NavigationDelegate.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"

using namespace Acts;
using namespace Acts::UnitLiterals;

BOOST_AUTO_TEST_SUITE(NavigationPolicyTests)

GeometryContext gctx;
auto logger = getDefaultLogger("NavigationPolicyTests", Logging::VERBOSE);

struct APolicy : public INavigationPolicy {
  APolicy(const GeometryContext& /*gctx*/, const TrackingVolume& /*volume*/,
          const Logger& /*logger*/) {}

  void initializeCandidates(const NavigationArguments& /*unused*/,
                            AppendOnlyNavigationStream& /*unused*/,
                            const Logger& /*unused*/) const {
    const_cast<APolicy*>(this)->executed = true;
  }

  void connect(NavigationDelegate& delegate) const override {
    connectDefault<APolicy>(delegate);
  }

  bool executed = false;
};

struct BPolicy : public INavigationPolicy {
  struct Config {
    int value;
  };

  BPolicy(const GeometryContext& /*gctx*/, const TrackingVolume& /*volume*/,
          const Logger& /*logger*/, Config config)
      : m_config(config) {}

  void connect(NavigationDelegate& delegate) const override {
    connectDefault<BPolicy>(delegate);
  }

  void initializeCandidates(const NavigationArguments& /*unused*/,
                            AppendOnlyNavigationStream& /*unused*/,
                            const Logger& /*unused*/) const {
    const_cast<BPolicy*>(this)->executed = true;
    const_cast<BPolicy*>(this)->value = m_config.value;
  }

  bool executed = false;
  int value = 0;

  Config m_config;
};

BOOST_AUTO_TEST_CASE(DirectTest) {
  TrackingVolume volume{
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(250_mm, 400_mm, 310_mm),
      "PixelLayer3"};

  MultiNavigationPolicy policy{APolicy{gctx, volume, *logger},
                               BPolicy{gctx, volume, *logger, {.value = 4242}}};

  NavigationDelegate delegate;
  policy.connect(delegate);

  NavigationStream main;
  AppendOnlyNavigationStream stream{main};
  delegate(NavigationArguments{.position = Vector3::Zero(),
                               .direction = Vector3::Zero()},
           stream, *logger);

  BOOST_CHECK(std::get<APolicy>(policy.policies()).executed);
  BOOST_CHECK(std::get<BPolicy>(policy.policies()).executed);
  BOOST_CHECK_EQUAL(std::get<BPolicy>(policy.policies()).value, 4242);
}

BOOST_AUTO_TEST_CASE(FactoryTest) {
  TrackingVolume volume{
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(250_mm, 400_mm, 310_mm),
      "PixelLayer3"};

  BPolicy::Config config{.value = 42};

  std::function<std::unique_ptr<INavigationPolicy>(
      const GeometryContext&, const TrackingVolume&, const Logger&)>
      factory = NavigationPolicyFactory::make()
                    .add<APolicy>()         // no arguments
                    .add<BPolicy>(config);  // config struct as argument

  auto policyBase = factory(gctx, volume, *logger);
  auto policyBase2 = factory(gctx, volume, *logger);

  auto& policy =
      dynamic_cast<MultiNavigationPolicy<APolicy, BPolicy>&>(*policyBase);

  NavigationDelegate delegate;
  policy.connect(delegate);

  NavigationStream main;
  AppendOnlyNavigationStream stream{main};
  delegate(NavigationArguments{.position = Vector3::Zero(),
                               .direction = Vector3::Zero()},
           stream, *logger);

  BOOST_CHECK(std::get<APolicy>(policy.policies()).executed);
  BOOST_CHECK(std::get<BPolicy>(policy.policies()).executed);
  BOOST_CHECK_EQUAL(std::get<BPolicy>(policy.policies()).value, 42);

  auto& policy2 =
      dynamic_cast<MultiNavigationPolicy<APolicy, BPolicy>&>(*policyBase2);

  NavigationDelegate delegate2;
  policyBase2->connect(delegate2);

  delegate2(NavigationArguments{.position = Vector3::Zero(),
                                .direction = Vector3::Zero()},
            stream, *logger);

  BOOST_CHECK(std::get<APolicy>(policy2.policies()).executed);
  BOOST_CHECK(std::get<BPolicy>(policy2.policies()).executed);
  BOOST_CHECK_EQUAL(std::get<BPolicy>(policy2.policies()).value, 42);
}

BOOST_AUTO_TEST_CASE(AsUniquePtrTest) {
  TrackingVolume volume{
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(250_mm, 400_mm, 310_mm),
      "PixelLayer3"};

  std::unique_ptr<NavigationPolicyFactory> factory =
      NavigationPolicyFactory::make().add<APolicy>().asUniquePtr();

  auto policyBase = factory->build(gctx, volume, *logger);
  auto& policy = dynamic_cast<MultiNavigationPolicy<APolicy>&>(*policyBase);

  NavigationDelegate delegate;
  policyBase->connect(delegate);

  NavigationStream main;
  AppendOnlyNavigationStream stream{main};
  delegate(NavigationArguments{.position = Vector3::Zero(),
                               .direction = Vector3::Zero()},
           stream, *logger);

  BOOST_CHECK(std::get<APolicy>(policy.policies()).executed);
}

struct CPolicy : public INavigationPolicy {};

template <typename T>
struct CPolicySpecialized : public CPolicy {
  struct Config {
    T value;
  };

  CPolicySpecialized(const TrackingVolume& /*volume*/, Config config)
      : m_config(config) {}

  void connect(NavigationDelegate& delegate) const override {
    connectDefault<CPolicySpecialized<T>>(delegate);
  }

  void initializeCandidates(const NavigationArguments& /*unused*/,
                            AppendOnlyNavigationStream& /*stream*/,
                            const Logger& /*logger*/) const {
    auto* self = const_cast<CPolicySpecialized<int>*>(this);
    self->executed = true;
    self->value = m_config.value;
  }

  bool executed = false;
  int value = 0;

  Config m_config;
};

struct IsolatedConfig {
  int value;
};

auto makeCPolicy(const GeometryContext& /*gctx*/, const TrackingVolume& volume,
                 const Logger& /*logger*/, IsolatedConfig config) {
  // I can do arbitrary stuff here
  CPolicySpecialized<int>::Config config2{.value = config.value};
  return CPolicySpecialized<int>(volume, config2);
}

BOOST_AUTO_TEST_CASE(IsolatedFactory) {
  TrackingVolume volume{
      Transform3::Identity(),
      std::make_shared<CylinderVolumeBounds>(250_mm, 400_mm, 310_mm),
      "PixelLayer3"};

  IsolatedConfig config{.value = 44};
  auto factory =
      NavigationPolicyFactory::make().add<APolicy>().add(makeCPolicy, config);

  auto factory2 =
      NavigationPolicyFactory::make().add(makeCPolicy, config).add<APolicy>();

  auto policyBase = factory(gctx, volume, *logger);
  auto& policy =
      dynamic_cast<MultiNavigationPolicy<APolicy, CPolicySpecialized<int>>&>(
          *policyBase);

  NavigationDelegate delegate;
  policyBase->connect(delegate);

  NavigationStream main;
  AppendOnlyNavigationStream stream{main};
  delegate(NavigationArguments{.position = Vector3::Zero(),
                               .direction = Vector3::Zero()},
           stream, *logger);

  BOOST_CHECK(std::get<APolicy>(policy.policies()).executed);
  BOOST_CHECK(std::get<CPolicySpecialized<int>>(policy.policies()).executed);
  BOOST_CHECK_EQUAL(std::get<CPolicySpecialized<int>>(policy.policies()).value,
                    44);
}

BOOST_AUTO_TEST_SUITE_END()

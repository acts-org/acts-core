import pytest

import acts

import acts.examples


def test_navigation_policy_factory():

    policy = (
        acts.NavigationPolicyFactory.make()
        .add(acts.TryAllPortalNavigationPolicy)
        .add(acts.TryAllSurfaceNavigationPolicy)
        .add(
            acts.SurfaceArrayNavigationPolicy,
            acts.SurfaceArrayNavigationPolicy.Config(
                layerType=acts.SurfaceArrayNavigationPolicy.LayerType.Plane,
                bins=(10, 10),
            ),
        )
    )

    policy._buildTest()

    policy = (
        acts.NavigationPolicyFactory.make()
        .add(acts.TryAllPortalNavigationPolicy)
        .add(acts.TryAllSurfaceNavigationPolicy)
    )

    policy._buildTest()


def test_navigation_policy_factory_build_empty():
    policy = acts.NavigationPolicyFactory.make()

    with pytest.raises(RuntimeError):
        policy._buildTest()


def test_navigation_policy_factory_add_multiple():
    with pytest.raises(ValueError):
        (
            acts.NavigationPolicyFactory.make()
            .add(acts.TryAllPortalNavigationPolicy)
            .add(acts.TryAllSurfaceNavigationPolicy)
            .add(acts.TryAllPortalNavigationPolicy)
        )

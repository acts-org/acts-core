// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"

#include <fstream>

namespace Acts::Experimental {
class IInternalStructureBuilder {};
}  // namespace Acts::Experimental

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(BlueprintTest) {
  std::vector<Acts::ActsScalar> bValues = {0., 10., 100.};

  // Create  root node
  std::vector<Acts::BinningValue> binning = {Acts::BinningValue::binR};
  auto root = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "detector", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      bValues, binning);
  // Check the root node
  BOOST_CHECK(root->isRoot());
  BOOST_CHECK_EQUAL(root->parent, nullptr);
  BOOST_CHECK(root->children.empty());
  BOOST_CHECK_EQUAL(root->name, "detector");

  auto leaf0 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "volume_0", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      bValues);
  BOOST_CHECK(leaf0->isLeaf());

  auto branch = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "container_0", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      bValues, binning);

  auto leaf1 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "volume_1", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      bValues);

  auto leaf2 = std::make_unique<Acts::Experimental::Blueprint::Node>(
      "volume_2", Acts::Transform3::Identity(), Acts::VolumeBounds::eOther,
      bValues,
      std::make_shared<Acts::Experimental::IInternalStructureBuilder>());

  // Keep around the pointers of the branch & leaves
  auto* leaf0Ptr = leaf0.get();
  auto* leaf1Ptr = leaf1.get();
  auto* leaf2Ptr = leaf2.get();
  auto* branchPtr = branch.get();

  branch->add(std::move(leaf1));
  branch->add(std::move(leaf2));

  // Branch has two children
  BOOST_CHECK_EQUAL(branch->children.size(), 2u);

  // Parent of the leaves is the branch
  BOOST_CHECK_EQUAL(leaf1Ptr->parent, branchPtr);
  BOOST_CHECK_EQUAL(leaf2Ptr->parent, branchPtr);

  root->add(std::move(branch));

  // Root stays root
  BOOST_CHECK(root->isRoot());
  // Parent of the branch is the root
  BOOST_CHECK_EQUAL(branchPtr->parent, root.get());

  root->add(std::move(leaf0));

  // Parent of the leaf is the root
  BOOST_CHECK_EQUAL(leaf0Ptr->parent, root.get());

  std::ofstream fs("blueprint.dot");
  Acts::Experimental::detail::BlueprintDrawer::dotStream(fs, *root);
  fs.close();
}

BOOST_AUTO_TEST_SUITE_END()

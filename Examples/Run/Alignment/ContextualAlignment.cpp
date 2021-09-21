// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/AlignedDetector.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"

#include "DetectorAlignment.hpp"

int main(int argc, char* argv[]) {
  // 1. The aligned transform updater
  ActsAlignment::AlignedTransformUpdater alignedTransformUpdater =
      [](Acts::DetectorElementBase* detElement,
         const Acts::GeometryContext& gctx,
         const Acts::Transform3& aTransform) {
        auto* alignedDetElement = dynamic_cast<
            ActsExamples::Contextual::InternallyAlignedDetectorElement*>(
            detElement);
        assert(alignedDetElement != nullptr && "Got wrong detector element");
        auto alignContext =
            gctx.get<ActsExamples::Contextual::
                         InternallyAlignedDetectorElement::ContextType>();
        if (alignedDetElement) {
          alignedDetElement->addAlignedTransform(aTransform, alignContext.iov);
          return true;
        }
        return false;
      };

  // 2. Selector for the detector elements to be aligned
  // @todo: allow different levels of alignment
  auto alignedDetElementsGetter =
      [](const std::shared_ptr<ActsExamples::IBaseDetector>& detector,
         const std::vector<Acts::GeometryIdentifier>& geometrySelection)
      -> std::vector<Acts::DetectorElementBase*> {
    std::vector<Acts::DetectorElementBase*> dets;
    auto* alignedDetector =
        dynamic_cast<ActsExamples::Contextual::AlignedDetector*>(
            detector.get());
    for (auto& lstore : alignedDetector->detectorStore()) {
      for (auto& ldet : lstore) {
        // get the detetor surface
        const auto& surface = &ldet->surface();
        auto geoID = surface->geometryId();
        auto it = std::find(geometrySelection.begin(), geometrySelection.end(),
                            geoID);
        if (it != geometrySelection.end()) {
          dets.push_back(ldet.get());
        }
      }
    }
    return dets;
  };

  return runDetectorAlignment(
      argc, argv, std::make_shared<ActsExamples::Contextual::AlignedDetector>(),
      alignedTransformUpdater, alignedDetElementsGetter);
}

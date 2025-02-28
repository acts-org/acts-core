# Task: Fix Non-explicit Constructors

This file tracks the progress of making constructors explicit across the ACTS codebase to prevent unintended implicit conversions. The task was initiated based on clang-tidy warnings about single-argument constructors that should be marked explicit.

The output of the clang-tidy run is in `clang_tidy.txt`, for reference.
The build command is `cmake --build build`. The compilation must be checked after each change is made using this command.

Some constructors cannot be made explicit due to existing code that relies on implicit conversions. In this case, the necessary changes to other files need to be made, until the compilation is successful.

Every time an item is completed, the corresponding todo item should be marked as such.

Procedure:
- Identify the next item in the todo list.
- Make the change to the code.
- Compile the code using `cmake --build build`. The compilation MUST be checked and MUST be successful before moving on.
- If the compilation was successful, mark the item as completed.


Some extra notes:
- NEVER update more than one item at once, also NEVER mark more than one item as completed at once.
- ONLY EVER skip an item and make it implicit again, if there is an existing comment explaining why it cannot be explicit.
- Don't prompt for feedback if there were no errors or you were able to fix compilation errors.


---

# Non-explicit Constructor Fixes Required

## Core/Utilities
- [x] `Acts::ValueHolder` constructor in `Holders.hpp:55` (skipped due to possible SEGFAULT)

## Core/Vertexing
- [x] `Acts::AdaptiveMultiVertexFinder` constructor in `AdaptiveMultiVertexFinder.hpp:172`
- [x] `Acts::NumericalTrackLinearizer::Config` constructor in `NumericalTrackLinearizer.hpp:73`
- [x] `Acts::NumericalTrackLinearizer` constructor in `NumericalTrackLinearizer.hpp:93`
- [x] `Acts::GaussianTrackDensity::Config` constructor in `GaussianTrackDensity.hpp:62`
- [x] `Acts::GaussianTrackDensity::State` constructor in `GaussianTrackDensity.hpp:87`
- [x] `Acts::GaussianTrackDensity` constructor in `GaussianTrackDensity.hpp:93`
- [x] `Acts::GaussianTrackDensity::GaussianTrackDensityStore` constructor in `GaussianTrackDensity.hpp:178`
- [x] `Acts::FullBilloirVertexFitter` constructor in `FullBilloirVertexFitter.hpp:64`
- [x] `Acts::AdaptiveMultiVertexFitter::Config` constructor in `AdaptiveMultiVertexFitter.hpp:101`
- [x] `Acts::AdaptiveMultiVertexFitter` constructor in `AdaptiveMultiVertexFitter.hpp:148`
- [x] `Acts::SingleSeedVertexFinder` constructor in `SingleSeedVertexFinder.hpp:102`
- [x] `Acts::IterativeVertexFinder` constructor in `IterativeVertexFinder.hpp:138`
- [x] `Acts::HelicalTrackLinearizer` constructor in `HelicalTrackLinearizer.hpp:63`
- [x] `Acts::ZScanVertexFinder::Config` constructor in `ZScanVertexFinder.hpp:40`
- [x] `Acts::ZScanVertexFinder` constructor in `ZScanVertexFinder.hpp:75`
- [x] `Acts::GaussianGridTrackDensity::Config` constructor in `GaussianGridTrackDensity.hpp:41`
- [x] `Acts::GaussianGridTrackDensity` constructor in `GaussianGridTrackDensity.hpp:82`
- [x] `Acts::BilloirTrack` constructor in `FullBilloirVertexFitter.cpp:24`

## Core/EventData
- [x] `Acts::SinglyChargedParticleHypothesis` constructor in `ParticleHypothesis.hpp:35`
- [x] `Acts::NeutralParticleHypothesis` constructor in `ParticleHypothesis.hpp:78`
- [x] `Acts::NonNeutralChargedParticleHypothesis` constructor in `ParticleHypothesis.hpp:110`
- [x] `Acts::ParticleHypothesis` constructor in `ParticleHypothesis.hpp:157`
- [x] `Acts::TrackStateRange` constructor in `MultiTrajectory.hpp:111`
- [x] `Acts::ProxyAccessorBase` constructors in `ProxyAccessor.hpp:73,77`
- [x] `Acts::VectorTrackContainer` constructor in `VectorTrackContainer.hpp:207`
- [x] `Acts::ConstVectorTrackContainer` constructors in `VectorTrackContainer.hpp:285,291`
- [x] `Acts::GenericParticleHypothesis` constructors in `GenericParticleHypothesis.hpp:48,60`
- [x] `Acts::CorrectedFreeToBoundTransformer` constructor in `CorrectedTransformationFreeToBound.hpp:75`
- [x] `Acts::DynamicKeyIterator` constructor in `DynamicKeyIterator.hpp:29`
- [x] `Acts::GenericFreeTrackParameters` constructor in `GenericFreeTrackParameters.hpp:114`
- [x] `Acts::TrackStateType` constructor in `TrackStateType.hpp:49`
- [x] `Acts::ConstTrackStateType` constructor in `TrackStateType.hpp:117`
- [x] `Acts::TrackProxy` constructor in `TrackProxy.hpp:117` (skipped: this is a copy constructor from mutable to const, where implicit conversion is expected and safe)
- [x] `Acts::ConstVectorMultiTrajectory` constructors in `VectorMultiTrajectory.hpp:572,575`
- [x] `Acts::TransitiveConstPointer` constructors in `TrackStateProxy.hpp:45,48`
- [x] `Acts::TrackStateProxy` constructor in `TrackStateProxy.hpp:220`

## Core/Detector
- [x] `Acts::GeometryIdGenerator` constructor in `GeometryIdGenerator.hpp:73`
- [x] `Acts::ChainedGeometryIdGenerator` constructor in `GeometryIdGenerator.hpp:145`
- [x] `Acts::GeometryIdMapper` constructor in `GeometryIdMapper.hpp:55`
- [x] `Acts::Portal` constructor in `Portal.hpp:50`
- [x] `Acts::MultiWireStructureBuilder` constructor in `MultiWireStructureBuilder.hpp:54`
- [x] `Acts::DetectorVolumeBuilder` constructor in `DetectorVolumeBuilder.hpp:57`
- [x] `Acts::VolumeStructureBuilder` constructor in `VolumeStructureBuilder.hpp:56`
- [x] `Acts::IndexedRootVolumeFinderBuilder` constructor in `IndexedRootVolumeFinderBuilder.hpp:29`
- [x] `Acts::CylindricalContainerBuilder` constructors in `CylindricalContainerBuilder.hpp:70,89`
- [x] `Acts::DetectorVolume::ObjectStore` constructor in `DetectorVolume.hpp:87`
- [x] `Acts::DetectorBuilder` constructor in `DetectorBuilder.hpp:53`
- [x] `Acts::LayerStructureBuilder::SurfacesHolder` constructor in `LayerStructureBuilder.hpp:61`
- [x] `Acts::LayerStructureBuilder` constructor in `LayerStructureBuilder.hpp:105`
- [x] `Acts::CuboidalContainerBuilder` constructors in `CuboidalContainerBuilder.hpp:68,87`
- [x] `Acts::MultiWireInternalStructureBuilder` constructor in `MultiWireStructureBuilder.cpp:52`

## Core/TrackFinding
- [x] `Acts::GbtsConnector` constructor in `GbtsConnector.hpp:40`
- [x] `Acts::TrackSelector::EtaBinnedConfig` constructors in `TrackSelector.hpp:162,168,175`
- [x] `Acts::TrackSelector` constructors in `TrackSelector.hpp:220,224`
- [x] `Acts::CombinatorialKalmanFilter` constructor in `CombinatorialKalmanFilter.hpp:288`

## Core/Surfaces
- [x] `Acts::CylinderBounds` constructor in `CylinderBounds.hpp:79`
- [x] `Acts::LineBounds` constructor in `LineBounds.hpp:39`
- [x] `Acts::DiscSurface` constructor in `DiscSurface.hpp:85`
- [x] `Acts::SingleElementLookup` constructors in `SurfaceArray.hpp:360,365`
- [x] `Acts::SurfaceArray` constructor in `SurfaceArray.hpp:446`
- [x] `Acts::AnnulusBounds` constructor in `AnnulusBounds.hpp:67`
- [x] `Acts::TrapezoidBounds` constructor in `TrapezoidBounds.hpp:52`
- [x] `Acts::ConvexPolygonBounds` constructors in `ConvexPolygonBounds.hpp:86,91,96,145`
- [x] `Acts::StrawSurface` constructor in `StrawSurface.hpp:52`
- [x] `Acts::DiscTrapezoidBounds` constructor in `DiscTrapezoidBounds.hpp:58`
- [x] `Acts::RadialBounds` constructor in `RadialBounds.hpp:54`
- [x] `Acts::LineSurface` constructor in `LineSurface.hpp:58`
- [x] `Acts::EllipseBounds` constructor in `EllipseBounds.hpp:64`
- [x] `Acts::RectangleBounds` constructor in `RectangleBounds.hpp:51`
- [x] `Acts::DiamondBounds` constructor in `DiamondBounds.hpp:61`
- [x] `Acts::PlaneSurface` constructor in `PlaneSurface.hpp:69`
- [x] `Acts::ConeBounds` constructor in `ConeBounds.hpp:72`
- [x] `Acts::PerigeeSurface` constructors in `PerigeeSurface.hpp:37,42`

## Core/TrackFitting
- [x] `Acts::KalmanFitter` constructor in `KalmanFitter.hpp:271`
- [x] `Acts::Gx2Fitter` constructor in `GlobalChiSquareFitter.hpp:691`

## Core/Geometry
- [x] `Acts::GlueVolumesDescriptor` constructor in `GlueVolumesDescriptor.hpp:41`
- [x] `Acts::SurfaceBinningMatcher` constructor in `SurfaceBinningMatcher.hpp:28`
- [x] `Acts::TrapezoidVolumeBounds` constructor in `TrapezoidVolumeBounds.hpp:87`
- [x] `Acts::ProtoLayerHelper` constructor in `ProtoLayerHelper.hpp:40`
- [x] `Acts::KDTreeTrackingGeometryBuilder` constructor in `KDTreeTrackingGeometryBuilder.hpp:67`
- [x] `Acts::CuboidVolumeBuilder` constructor in `CuboidVolumeBuilder.hpp:127`
- [x] `Acts::SurfaceArrayCreator` constructors in `SurfaceArrayCreator.hpp:98,105`
- [x] `Acts::PassiveLayerBuilder` constructor in `PassiveLayerBuilder.hpp:57`
- [x] `Acts::ConeVolumeBounds` constructor in `ConeVolumeBounds.hpp:81`
- [x] `Acts::TrackingVolumeArrayCreator` constructor in `TrackingVolumeArrayCreator.hpp:38`
- [x] `Acts::Layer` constructor in `Layer.hpp:221`
- [ ] `Acts::CylinderVolumeHelper` constructor in `CylinderVolumeHelper.hpp:59`
- [ ] `Acts::TrackingGeometry` constructor in `TrackingGeometry.hpp:51`
- [ ] `Acts::GenericApproachDescriptor` constructor in `GenericApproachDescriptor.hpp:38`
- [ ] `Acts::GeometryHierarchyMap` constructor in `GeometryHierarchyMap.hpp:72`
- [ ] `Acts::GenericCuboidVolumeBounds` constructors in `GenericCuboidVolumeBounds.hpp:42,48`
- [ ] `Acts::CylinderVolumeBuilder` constructor in `CylinderVolumeBuilder.hpp:512`
- [ ] `Acts::LayerCreator` constructor in `LayerCreator.hpp:62`
- [ ] `Acts::TrackingVolume` constructor in `TrackingVolume.hpp:156`
- [ ] `Acts::TrackingGeometryBuilder` constructor in `TrackingGeometryBuilder.hpp:62`

## Core/MagneticField
- [ ] `Acts::ConstantBField::Cache` constructor in `ConstantBField.hpp:26`
- [ ] `Acts::InterpolatedBFieldMap::Cache` constructor in `InterpolatedBFieldMap.hpp:148`
- [ ] `Acts::InterpolatedBFieldMap` constructor in `InterpolatedBFieldMap.hpp:177`
- [ ] `Acts::NullBField::Cache` constructor in `NullBField.hpp:23`
- [ ] `Acts::SolenoidBField::Cache` constructor in `SolenoidBField.hpp:73`
- [ ] `Acts::SolenoidBField` constructor in `SolenoidBField.hpp:93`

## Core/Material
- [x] `Acts::ISurfaceMaterial` constructor in `ISurfaceMaterial.hpp:38`
- [x] `Acts::MaterialComposition` constructor in `MaterialComposition.hpp:101`
- [x] `Acts::ProtoSurfaceMaterialT` constructor in `ProtoSurfaceMaterial.hpp:39`
- [x] `Acts::BinnedSurfaceMaterialAccumulater` constructor in `BinnedSurfaceMaterialAccumulater.hpp:48`
- [ ] `Acts::InteractionVolume` constructors in `MaterialInteraction.hpp:35,39`
- [ ] `Acts::HomogeneousVolumeMaterial` constructor in `HomogeneousVolumeMaterial.hpp:28`
- [ ] `Acts::HomogeneousSurfaceMaterial` constructor in `HomogeneousSurfaceMaterial.hpp:33`
- [ ] `Acts::MaterialMapper` constructor in `MaterialMapper.hpp:63`
- [ ] `Acts::MaterialValidater` constructor in `MaterialValidater.hpp:38`
- [ ] `Acts::AccumulatedSurfaceMaterial` constructors in `AccumulatedSurfaceMaterial.hpp:40,52`
- [ ] `Acts::ProtoVolumeMaterial` constructor in `ProtoVolumeMaterial.hpp:38`
- [ ] `Acts::PropagatorMaterialAssigner` constructor in `PropagatorMaterialAssigner.hpp:101`
- [ ] `Acts::InterpolatedMaterialMap` constructor in `InterpolatedMaterialMap.hpp:250`
- [ ] `Acts::IntersectionMaterialAssigner` constructor in `IntersectionMaterialAssigner.hpp:50`

## Core/Clusterization
- [ ] `Acts::TimedConnect` constructor in `TimedClusterization.hpp:28`

## Conversion Operators to Fix
- [ ] `Acts::CorrectedTransformationFreeToBound::operator bool()` in `CorrectedTransformationFreeToBound.hpp:53`
- [ ] `Acts::MaterialComposition::operator bool()` in `MaterialComposition.hpp:128`

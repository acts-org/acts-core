# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        AccumulatedMaterialSlab.cpp
        AccumulatedSurfaceMaterial.cpp
        AccumulatedVolumeMaterial.cpp
        AverageMaterials.cpp
        BinnedSurfaceMaterial.cpp
        BinnedSurfaceMaterialAccumulater.cpp
        HomogeneousSurfaceMaterial.cpp
        HomogeneousVolumeMaterial.cpp
        Interactions.cpp
        IntersectionMaterialAssigner.cpp
        Material.cpp
        MaterialGridHelper.cpp
        MaterialInteractionAssignment.cpp
        MaterialMapUtils.cpp
        MaterialMapper.cpp
        MaterialSlab.cpp
        MaterialValidater.cpp
        ProtoVolumeMaterial.cpp
        SurfaceMaterialMapper.cpp
        VolumeMaterialMapper.cpp
)

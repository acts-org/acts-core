# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        EigenStepperError.cpp
        MultiStepperError.cpp
        SympyStepper.cpp
        PropagatorError.cpp
        StraightLineStepper.cpp
        detail/PointwiseMaterialInteraction.cpp
        detail/CovarianceEngine.cpp
        detail/JacobianEngine.cpp
        detail/SympyCovarianceEngine.cpp
        detail/SympyJacobianEngine.cpp
)

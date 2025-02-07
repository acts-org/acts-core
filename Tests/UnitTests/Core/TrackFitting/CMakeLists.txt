# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_unittest(GainMatrixSmoother GainMatrixSmootherTests.cpp)
add_unittest(GainMatrixUpdater GainMatrixUpdaterTests.cpp)
add_unittest(KalmanFitter KalmanFitterTests.cpp)
add_unittest(Gsf GsfTests.cpp)
add_unittest(GsfComponentMerging GsfComponentMergingTests.cpp)
add_unittest(GsfMixtureReduction GsfMixtureReductionTests.cpp)
add_unittest(Gx2f Gx2fTests.cpp)
add_unittest(MbfSmoother MbfSmootherTests.cpp)

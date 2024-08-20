target_sources(
    ActsCore
    PRIVATE
        KalmanFitterError.cpp
        GainMatrixUpdater.cpp
        GainMatrixSmoother.cpp
        GlobalChiSquareFitterError.cpp
        GsfError.cpp
        GsfUtils.cpp
        BetheHeitlerApprox.cpp
        GsfMixtureReduction.cpp
        GlobalChiSquareFitter.cpp
        MbfSmoother.cpp
)

foreach(DIM RANGE 1 6)
    set(dim_file ${CMAKE_CURRENT_BINARY_DIR}/GainMatrixUpdaterImpl${DIM}.cpp)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/GainMatrixUpdaterImpl.cpp.in
        ${dim_file}
        @ONLY
    )

    set_source_files_properties(
        ${dim_file}
        PROPERTIES
            COMPILE_DEFINITIONS ACTS_GAIN_MATRIX_UPDATER_INSTANTIATE=${DIM}
    )
    target_sources(ActsCore PRIVATE ${dim_file})
endforeach()

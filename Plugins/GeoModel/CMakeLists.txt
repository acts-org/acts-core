include(FetchContent)

add_library(
    ActsPluginGeoModel
    SHARED
    src/GeoModelBlueprintCreater.cpp
    src/GeoModelConversionError.cpp
    src/GeoModelToDetectorVolume.cpp
    src/GeoModelReader.cpp
    src/GeoModelDetectorElement.cpp
    src/GeoModelDetectorObjectFactory.cpp
    src/GeoModelMaterialConverter.cpp
    src/detail/GeoBoxConverter.cpp
    src/detail/GeoTrdConverter.cpp
    src/detail/GeoTubeConverter.cpp
    src/detail/GeoPolygonConverter.cpp
    src/detail/GeoSubtractionConverter.cpp
    src/detail/GeoShiftConverter.cpp
    src/detail/GeoIntersectionAnnulusConverter.cpp
    src/detail/GeoModelBinningHelper.cpp
    src/detail/GeoModelExtentHelper.cpp
    src/detail/GeoUnionDoubleTrdConverter.cpp
    src/GeoModelDetectorElementITk.cpp
)
target_include_directories(
    ActsPluginGeoModel
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
    ActsPluginGeoModel
    PUBLIC
        ActsCore
        GeoModelCore::GeoModelKernel
        GeoModelIO::GeoModelDBManager
        GeoModelIO::GeoModelRead
)

install(
    TARGETS ActsPluginGeoModel
    EXPORT ActsPluginGeoModelTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

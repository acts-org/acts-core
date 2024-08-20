set(library_sources
    src/TGeoCylinderDiscSplitter.cpp
    src/TGeoDetectorElement.cpp
    src/TGeoLayerBuilder.cpp
    src/TGeoParser.cpp
    src/TGeoPrimitivesHelper.cpp
    src/TGeoSurfaceConverter.cpp
)

# Internal system of units change, This behavior affected versions up to v6-25-01
# see: https://root.cern.ch/doc/v626/classTGeoMaterial.html
if(${ROOT_VERSION} VERSION_GREATER "6.25.01")
    list(APPEND library_sources src/TGeoMaterialConverter.cpp)
else()
    message(
        INFO
        " Skipping TGeoMaterialConverter due to ROOT version ${ROOT_VERSION}"
    )
endif()

add_library(ActsPluginTGeo SHARED ${library_sources})

target_include_directories(
    ActsPluginTGeo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ActsPluginTGeo PUBLIC ActsCore ROOT::Geom)

install(
    TARGETS ActsPluginTGeo
    EXPORT ActsPluginTGeoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

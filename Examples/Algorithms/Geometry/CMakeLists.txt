add_library(ActsExamplesGeometry SHARED src/VolumeAssociationTest.cpp)

target_include_directories(
    ActsExamplesGeometry
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesGeometry
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesGeometry
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

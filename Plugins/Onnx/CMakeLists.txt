add_library(
    ActsPluginOnnx
    SHARED
    # header files
    include/Acts/Plugins/Onnx/OnnxRuntimeBase.hpp
    include/Acts/Plugins/Onnx/MLTrackClassifier.hpp
    include/Acts/Plugins/Onnx/AmbiguityTrackClassifier.hpp
    include/Acts/Plugins/Onnx/SeedClassifier.hpp
    # source files
    src/OnnxRuntimeBase.cpp
    src/MLTrackClassifier.cpp
)

target_include_directories(
    ActsPluginOnnx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ActsPluginOnnx PUBLIC ActsCore OnnxRuntime)

install(
    TARGETS ActsPluginOnnx
    EXPORT ActsPluginOnnxTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

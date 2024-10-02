add_library(ActsPluginPodio SHARED src/PodioUtil.cpp)

target_include_directories(
    ActsPluginPodio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ActsPluginPodio PUBLIC ActsCore)

# target_link_libraries(ActsPluginEDM4hep PUBLIC EDM4HEP::edm4hep)

# message(STATUS "IO HANDLERS: ${PODIO_IO_HANDLERS}")

podio_generate_datamodel(
  ActsPodioEdm
  ${CMAKE_CURRENT_LIST_DIR}/edm.yml
  headers
  sources
  IO_BACKEND_HANDLERS ${PODIO_IO_HANDLERS}
)

podio_add_datamodel_core_lib(ActsPodioEdm "${headers}" "${sources}")

target_link_libraries(
    ActsPluginPodio
    PUBLIC ActsPodioEdm ROOT::Core podio::podio podio::podioRootIO
)

target_compile_options(ActsPodioEdm PRIVATE "-Wno-old-style-cast")

podio_add_root_io_dict(ActsPodioEdmDict ActsPodioEdm "${headers}" src/selection.xml)
add_library(Acts::ActsPodioEdmDict ALIAS ActsPodioEdmDict)

target_compile_options(ActsPodioEdmDict PRIVATE "-Wno-old-style-cast")

install(
    TARGETS ActsPodioEdm
    EXPORT ActsPodioEdmTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ActsPodioEdm
)

install(
    DIRECTORY ActsPodioEdm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ActsPodioEdm
)

set(install_package_config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/Acts")
install(EXPORT ActsPodioEdmTargets DESTINATION ${install_package_config_dir})

install(
    TARGETS ActsPluginPodio
    EXPORT ActsPluginPodioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/Acts DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
    TARGETS ActsPodioEdmDict
    EXPORT ActsPodioEdmTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
    PUBLIC_HEADER
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ActsPodioEdm"
        COMPONENT dev
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ActsPodioEdmDictDict.rootmap"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT dev
)

if(${ROOT_VERSION} GREATER 6)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/libActsPodioEdmDict_rdict.pcm"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT dev
    )
endif()

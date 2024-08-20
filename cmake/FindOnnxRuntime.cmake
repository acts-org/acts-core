# Find the ONNX Runtime include directory and library.
#
# This module defines the `onnxruntime` imported target that encodes all
# necessary information in its target properties.

find_library(
    OnnxRuntime_LIBRARY
    NAMES onnxruntime
    PATHS ${onnxruntime_DIR}
    PATH_SUFFIXES lib lib32 lib64
    DOC "The ONNXRuntime library"
)

if(NOT OnnxRuntime_LIBRARY)
    message(FATAL_ERROR "onnxruntime library not found")
else()
    message(STATUS "Found OnnxRuntime library at ${OnnxRuntime_LIBRARY}")
endif()

find_path(
    OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS ${onnxruntime_DIR}
    PATH_SUFFIXES include include/onnxruntime include/onnxruntime/core/session
    DOC "The ONNXRuntime include directory"
)

if(NOT OnnxRuntime_INCLUDE_DIR)
    message(FATAL_ERROR "onnxruntime includes not found")
else()
    file(READ ${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h ver)
    string(REGEX MATCH "ORT_API_VERSION ([0-9]*)" _ ${ver})
    set(OnnxRuntime_API_VERSION ${CMAKE_MATCH_1})
    message(
        STATUS
        "Found OnnxRuntime includes at ${OnnxRuntime_INCLUDE_DIR} (API version: ${OnnxRuntime_API_VERSION})"
    )
endif()

string(
    REPLACE
    "."
    ";"
    OnnxRuntime_MIN_VERSION_LIST
    ${_acts_onnxruntime_version}
)
list(GET OnnxRuntime_MIN_VERSION_LIST 1 OnnxRuntime_MIN_API_VERSION)
if("${OnnxRuntime_API_VERSION}" LESS ${OnnxRuntime_MIN_API_VERSION})
    message(
        FATAL_ERROR
        "OnnxRuntime API version ${OnnxRuntime_MIN_API_VERSION} or greater required"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
)

add_library(OnnxRuntime SHARED IMPORTED)
set_property(
    TARGET OnnxRuntime
    PROPERTY IMPORTED_LOCATION ${OnnxRuntime_LIBRARY}
)
set_property(
    TARGET OnnxRuntime
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OnnxRuntime_INCLUDE_DIR}
)
set_property(
    TARGET OnnxRuntime
    PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${OnnxRuntime_INCLUDE_DIR}
)

mark_as_advanced(OnnxRuntime_FOUND OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARY)
